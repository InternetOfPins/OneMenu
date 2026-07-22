#pragma once

#include "oneMenu/menu/out.h"

namespace oneMenu {

  /// @brief will call fmtStart(tag) + Base::print + fmtStop(tag)
  /// @tparam tag 
  template<Fmt tag>
  struct FmtPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using Base::fmtStart;
      using Base::fmtStop;
      using This=Part<O>;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        Base::template fmtStart<tag>(ctx);
        bool r=Base::printMenu(i,ctx);
        Base::template fmtStop<tag>(ctx);
        return r;
      }
    };
  };

  using ViewPrinter=FmtPrinter<Fmt::View>;
  using AccelPrinter=FmtPrinter<Fmt::Accel>;
  using LabelPrinter=FmtPrinter<Fmt::Label>;
  using FieldPrinter=FmtPrinter<Fmt::Field>;
  using UnitPrinter=FmtPrinter<Fmt::Unit>;
  using EditModePrinter=FmtPrinter<Fmt::EditMode>;
  using TextEditCursorPrinter=FmtPrinter<Fmt::EditCursor>;

  /// @brief groups some printer parts to form menu
  /// @tparam ...OO the body parts
  template<typename... OO>
  struct MenuPrinter : aPrinter {
    template<typename O>
    struct Part:Chain<OO...>::template Part<O> {
      using IsPrinter=std::true_type;
      using Base=typename Chain<OO...>::template Part<O>;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::obj;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        ctx.idx=0;
        Base::template fmtStart<Fmt::Menu>(ctx);
        // Choice: a constant marker, emitted right after <menu> opens (attr
        // still open) so a web client (XmlFmt) can tell "this level is a
        // Choose field's own inner body — sibling items are selectable
        // options" apart from an ordinary nested submenu. Gated on BOTH the
        // active format (IsXmlFmt, via the fully-assembled Out reached
        // through CRTP obj() — MenuPrinter itself has no direct Out&, unlike
        // item.h's printItem) and the menu instance i's own composition
        // (IsChoiceBody, spliced into ChooseFieldDef's Menu<T,B,IsChoiceBody,
        // OO...> only, menu.h) — every other Menu<> is unaffected.
        if constexpr(hapi::query<IsXmlFmt,typename std::decay_t<decltype(obj())>::Types>
                  && hapi::query<hapi::SameAs<IsChoiceBody>,typename I::Types>) {
          Base::template fmtStart<Fmt::Choice>(ctx);
          Base::template fmtStop<Fmt::Choice>(ctx);
        }
        // i.printTo(Base::obj());
        bool r=Base::printMenu(i,ctx);
        Base::template fmtStop<Fmt::Menu>(ctx);
        return r;
      }
    };
  };

}; // namespace oneMenu (reopened below)

// Traverse specialization: MenuPrinter<OO...> is not a hapi::Chain, so its pack is
// otherwise opaque to hapi::query/Exists — needed so tags nested inside it (e.g.
// ScrollBodyPrinter's aScrollBody, buried via MenuPrinter<TitlePrinter,ScrollBodyPrinter,...>)
// are still found by a flat query<Q, SomeOutDef::Types> walk.
namespace hapi {
  template<typename Op, typename... OO>
  struct Traverse<Op, oneMenu::MenuPrinter<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };
}

namespace oneMenu {

  /// @brief print the title + format
  /// @brief printer layer that emits the menu title wrapped in fmtStart/fmtStop(Title)
  struct TitlePrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::Title>(ctx);
        i.print(O::obj(),ctx);
        O::template fmtStop<Fmt::Title>(ctx);
#ifdef MENU_DEBUG_FULLSCREEN
        if(O::lockMode()!=LockMode::Changed&&O::lockMode()!=LockMode::Sync) {
          Pos p=O::getPos(); printf("[TitlePrinter] after title: pos=(%d,%d) lockMode=%d\n",(int)p.x,(int)p.y,(int)O::lockMode());
        }
#endif
        return O::printMenu(i,ctx)||i.changed();
      }
    };
  };

  /// @brief start body printing process by redirecting to the item.
  /// Chains to Base::printMenu so any other post-body printers in the same
  /// MenuPrinter<...> pack are reached through the normal fmt pipeline.
  struct BodyPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using Base::obj;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        Base::template fmtStart<Fmt::Body>(ctx);
        bool r=i.printBody(O::obj(),ctx);
        Base::template fmtStop<Fmt::Body>(ctx);
        return Base::printMenu(i,ctx)||r;
      }
    };
  };

  /// @brief print scroll menu body
  struct ScrollBodyPrinter : aPrinter, aScrollBody {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "ScrollBodyPrinter: Cursor must be placed below ScrollBodyPrinter — scroll measurement needs tracked position");
      return true;
    }
    template<typename P>
    struct Part:BodyPrinter::Part<P> {
      using IsPrinter=std::true_type;
      using Base=typename BodyPrinter::Part<P>;
      using Base::lockMode;
      using Base::getPos;
      using Base::free;
      using Base::setPos;

      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        if(i.size()==0) return false;
        LockMode om=lockMode();
        // Changed/Sync are read-only traversals — skip scroll state changes entirely
        if(om==LockMode::Changed||om==LockMode::Sync) return Base::printMenu(i,ctx);
        Sz x=Base::getPos().x;
        Sz y=Base::getPos().y;
#ifdef MENU_DEBUG_FULLSCREEN
        printf("[ScrollBodyPrinter] entry anchor: pos=(%d,%d) lockMode=%d top=%d sel=%d\n",(int)x,(int)y,(int)om,(int)ctx.top(),(int)ctx.sel(ctx.pAt));
#endif
        if(ctx.sel(ctx.pAt)<0) ctx.path.data[(int)ctx.pAt]=0;
        else if(ctx.sel(ctx.pAt)>=i.size()) ctx.path.data[(int)ctx.pAt]=i.size()-1;
        if(ctx.sel(ctx.pAt)<ctx.top()) {
          ctx.top(ctx.sel());//--scroll up
          om=LockMode::None;//scroll => full redraw
        } else for(;;) {
          lockMode(LockMode::Measure);
          // measure body only — fmtStop<Footer> emits nl() which would corrupt free().y
          Base::template fmtStart<Fmt::Body>(ctx);
          i.printBody(Base::obj(),ctx);
          Base::template fmtStop<Fmt::Body>(ctx);
          Sz f=free().y;
          Sz ci=ctx.idx;
          ctx.idx=0;
          // Bound by the last item: top can never usefully scroll past it. Without this,
          // a page that always measures as "the selected item didn't fully fit" (f<0 at
          // ci-1) never breaks on its own — e.g. a FullScreen item deliberately consumes
          // the *entire* remaining page, so free().y reads negative (fmtStop<Body> adds
          // its own nl() on top of FullScreen's own padding) for every top, permanently
          // vetoing the "fits" check below and spinning forever. Once top reaches the
          // last item there's nowhere further to scroll, so stop regardless — this is
          // also the exactly-correct landing spot for a FullScreen page (top==sel).
          if((ctx.sel(ctx.pAt)<ci&&(!(ctx.sel(ctx.pAt)==(ci-1)&&f<0)))||ctx.top()>=i.size()-1) break;
          setPos(Pos{x,y});
          ctx.top(ctx.top()+1);//--scroll down
          om=LockMode::None;//scroll => full redraw
        };
        lockMode(om);
        setPos({x,y});
        bool r=Base::printMenu(i,ctx);
        return r;
      }
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        if(ctx.idx<ctx.top()) {
          ctx.idx++;
          return false;
        }
        // In Changed/Sync traversal Gate already gates output — bypass free().y area check
        // so visible items' changed()/sync() is actually reached.
        LockMode m=Base::lockMode();
        if(m==LockMode::Changed||m==LockMode::Sync) return Base::printItem(i,ctx);
        return Base::free().y>0?Base::printItem(i,ctx):false;
      }
    };
  };

  /// @brief like ScrollBodyPrinter, but skips the scroll-search entirely — top is always
  /// exactly ctx.sel(). There's no window to search for: a FullScreen item (item.h)
  /// always consumes the whole page, so "increasing top eventually fits more items"
  /// (ScrollBodyPrinter::printMenu's own search invariant) can never become true here;
  /// forcing FullScreen through that search risks an infinite loop and a stale
  /// position, so this case gets its own dedicated path instead of patching a function
  /// built for a different case.
  /// Deliberately reuses ScrollBodyPrinter::Part's own printItem unchanged (its
  /// skip-before-top / stop-when-full gating applies equally here — only the *search*
  /// for top is skipped) by inheriting from it and overriding only printMenu, then
  /// calling straight through to BodyPrinter::Part::printMenu (ScrollBodyPrinter's own
  /// Base) instead of ScrollBodyPrinter::Part::printMenu — skipping its search loop.
  struct SelectBodyPrinter : aPrinter, aScrollBody {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "SelectBodyPrinter: Cursor must be placed below SelectBodyPrinter — scroll measurement needs tracked position");
      return true;
    }
    template<typename P>
    struct Part:ScrollBodyPrinter::template Part<P> {
      using Base=typename ScrollBodyPrinter::template Part<P>;
      using Super=typename Base::Base;// BodyPrinter::Part<P> — bypasses Base's own scroll-search
      using Base::lockMode;

      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        if(i.size()==0) return false;
        LockMode om=lockMode();
        // Changed/Sync are read-only traversals — mirrors ScrollBodyPrinter's own guard
        if(om!=LockMode::Changed&&om!=LockMode::Sync) {
          Sz sel=ctx.sel(ctx.pAt);
          if(sel<0) sel=0; else if(sel>=i.size()) sel=(Sz)i.size()-1;
          ctx.path.data[(int)ctx.pAt]=sel;
          ctx.top(sel);// the whole fix: top IS sel, always — nothing to search for
        }
        return Super::printMenu(i,ctx);
      }
    };
  };

  /// @brief groups some printer parts to form a item body, will be formatted as a item
  /// also checks LockMode and act appropriately
  template<typename... OO>
  struct ItemPrinter : aPrinter {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "ItemPrinter: Cursor must be placed below ItemPrinter — partial-update repositioning needs tracked position");
      return true;
    }
    template<typename O>
    struct Part:Chain<OO...>::template Part<O> {
      using IsPrinter=std::true_type;
      using Base=typename Chain<OO...>::template Part<O>;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::lockMode;
      using Base::setPos;
      using Base::getPos;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        LockMode om=lockMode();
        if(om==LockMode::Update
          &&(i.changed()||(ctx.prev!=ctx.sel()&&(ctx.idx==ctx.prev||ctx.idx==ctx.sel()))
            // ctx.prev/ctx.sel() are relative to ctx.at (this recursion depth) — a selection
            // move *inside* an already-open pad (e.g. SelectBehave/RecallNavPos browsing
            // choices via padOpen(), not a fresh Menu::open() level push) never changes the
            // *top-level* selection those compare, so it never trips this far. TreeNav::changed()
            // already sees it correctly (m_prevSel!=sel() at the true, current m_level), which is
            // why doOutput() redraws at all — but nothing here told this specific collapsed row
            // (RecallNavPos::printItem picks ctx.path.last() live while ctx.after()>1) that it's
            // the one that needs to actually reach the device. ctx is true only for the item that
            // owns the currently open path, so this only affects that one row.
            ||(ctx&&ctx.after()>1))
        ) {
          lockMode(LockMode::None);
          // Body-level setPos(x,y) that opens this pass was itself Gate-suppressed (it ran
          // while still locked at Update) — logical Cursor::m_at reset to the body origin, but
          // the real device cursor was left wherever the last actually-unlocked write parked
          // it (e.g. bottom of the previous full page). Items are otherwise written by plain
          // sequential nl() chaining, no per-item absolute reposition — so a lone force-unlocked
          // item would draw its content at that stale real position instead of its own row.
          // Re-send position now, while genuinely unlocked, to resync real-to-logical before
          // drawing (same idiom as RecallNavPos::printItem's out.setPos(out.getPos())).
          setPos(getPos());
        }
        ctx.enabled =i.enabled();
        Base::template fmtStart<Fmt::Item>(ctx);
        bool r=Base::printItem(i,ctx);
        Base::template fmtStop<Fmt::Item>(ctx);
        ctx.idx++;
        if(lockMode()==LockMode::Sync) i.sync();
        else if(om==LockMode::Update&&lockMode()==LockMode::None) lockMode(LockMode::Update);
        return r;
      }
    };
  };

  /// @brief print the item
  /// @brief printer leaf that delegates printItem to the item itself, then signals changed()
  struct ItemBodyPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        i.printItem(Base::obj(),ctx);
        return Base::printItem(i,ctx)||i.changed();
      }
    };
  };

  /// @brief triggers the edit index accel number print
  struct IndexPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using This=Part<O>;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::Index>(ctx);
        O::template fmtStop<Fmt::Index>(ctx);
        return O::printItem(i,ctx);
      }
    };
  };

  /// @brief triggers the item's own enabled/disabled state print — unlike
  /// NavCursorPrinter's '@'/'-'/' ' (which only distinguishes enabled from
  /// disabled when the item is ALSO the focused one), this fires for every
  /// item unconditionally, same shape as IndexPrinter — a universal no-op
  /// for any format that doesn't specifically handle Fmt::Enabled (out.h's
  /// own base default), real output only from XmlFmt today (web clients:
  /// render a disabled item as non-clickable instead of guessing from
  /// nav-focus state alone).
  struct EnabledPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using This=Part<O>;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::Enabled>(ctx);
        O::template fmtStop<Fmt::Enabled>(ctx);
        return O::printItem(i,ctx);
      }
    };
  };

  /// @brief triggers the navigation cursor print
  struct NavCursorPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using This=Part<O>;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::NavCursor>(ctx);
        O::template fmtStop<Fmt::NavCursor>(ctx);
        bool r=O::printItem(i,ctx);
        return r;
      }
    };
  };

  /// @brief allow inclusion of data on the printers queue as a item part
  /// @tparam Data: included static data
  /// @brief printer that injects a static data value as an item prefix
  template<typename Data>
  struct StaticDataPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::put(Data::get());
        return O::printItem(i,ctx);
      }
    };
  };

  #ifdef MENU_DEBUG
    /// @brief debug printer part, printing path
    struct PathPrinter {
      template<typename O>
      struct Part:O {
        using IsPrinter=std::true_type;
        using Base=O;
        template<typename I>
        bool printItem(I& i,Ctx& ctx) {
          bool r=O::printItem(i,ctx);
          return r;
        }
      };
    };
  #endif

  //format printers to use with item description ----------------
  template<Fmt tag,typename... OO>
  struct AsFmt {
    struct PartEnd {
      template<typename O>
      struct Part:O {
        using Base=O;
        using Base::Base;
        template<typename Out> static void print(Out&) noexcept {}
        template<typename Out> static void printItem(Out&,Ctx&) noexcept {}
      };
    };
    template<typename O>
    struct Part:Chain<OO...,PartEnd>::template Part<O> {
      using Base=typename Chain<OO...,PartEnd>::template Part<O>;
      using Base::Base;
      template<typename Out> void print(Out& out) const { O::print(out); }
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        out.template fmtStart<tag>(ctx);
        // Only wrap in a Data/CDATA section when there's real content to
        // print (OO... non-empty) — a zero-arg marker like AsEditMode<>
        // wraps nothing (Base::printItem is PartEnd's own no-op), and
        // XmlFmt's Fmt::Data::fmtStart calls closeAttr() unconditionally,
        // which wrongly force-closes an attribute-style tag's still-open
        // value (e.g. mode="...) if invoked here regardless. Found
        // 2026-07-22 rendering a real NumField (AsEditMode<>) over XmlFmt.
        if constexpr(sizeof...(OO)>0) {
          out.template fmtStart<Fmt::Data>(ctx);
          Base::printItem(out,ctx);
          out.template fmtStop<Fmt::Data>(ctx);
        } else {
          Base::printItem(out,ctx);
        }
        out.template fmtStop<tag>(ctx);
        O::printItem(out,ctx);
      }
    };
  };

  template<typename... OO> using AsView=AsFmt<Fmt::View,OO...>;
  template<typename... OO> using AsTitle=AsFmt<Fmt::Title,OO...>;
  template<typename... OO> using AsMenu=AsFmt<Fmt::Menu,OO...>;
  template<typename... OO> using AsBody=AsFmt<Fmt::Body,OO...>;
  template<typename... OO> using AsItem=AsFmt<Fmt::Item,OO...>;
  template<typename... OO> using AsIndex=AsFmt<Fmt::Index,OO...>;
  template<typename... OO> using AsAccel=AsFmt<Fmt::Accel,OO...>;
  template<typename... OO> using AsNavCursor=AsFmt<Fmt::NavCursor,OO...>;
  template<typename... OO> using AsField=AsFmt<Fmt::Field,OO...>;
  template<typename... OO> using AsLabel=AsFmt<Fmt::Label,OO...>;
  template<typename... OO> using AsEditMode=AsFmt<Fmt::EditMode,OO...>;
  template<typename... OO> using AsEditCursor=AsFmt<Fmt::EditCursor,OO...>;
  template<typename... OO> using AsData=AsFmt<Fmt::Data,OO...>;
  template<typename... OO> using AsUnit=AsFmt<Fmt::Unit,OO...>;
    

  using ItemsPrinter=ItemPrinter<IndexPrinter,EnabledPrinter,NavCursorPrinter,ItemBodyPrinter>;

  // Full printers: title + body + footer
  using FullPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,BodyPrinter,ItemsPrinter>
  >;
  using ScrollPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,ScrollBodyPrinter,ItemsPrinter>
  >;

  // No-title variants: body + footer only — default for small-display devices
  using NoTitlePrinter=Chain<
    ViewPrinter,
    MenuPrinter<BodyPrinter,ItemsPrinter>
  >;
  using NoTitleScrollPrinter=Chain<
    ViewPrinter,
    MenuPrinter<ScrollBodyPrinter,ItemsPrinter>
  >;

  // Single-item-per-page variants: always shows exactly ctx.sel(), nothing else — the
  // dedicated FullScreen printer (see SelectBodyPrinter above), not ScrollPrinter/
  // NoTitleScrollPrinter's general multi-item scroll-search.
  using SelectPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,SelectBodyPrinter,ItemsPrinter>
  >;
  using NoTitleSelectPrinter=Chain<
    ViewPrinter,
    MenuPrinter<SelectBodyPrinter,ItemsPrinter>
  >;
};//oneMenu