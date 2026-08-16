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
      // Re-entrant guard: a NESTED printMenu call for the SAME tag (e.g.
      // Menu::Part::printItem's own pad-mode handling re-entering the whole
      // out.printMenu(...) chain from within an already-open <item>, to
      // reuse the exact Menu/Title/Body/Item wrapping a real submenu gets —
      // see item.h/menu.h) must NOT re-wrap; only the OUTERMOST call (the
      // real top-level print pass, ViewPrinter=FmtPrinter<Fmt::View>) should
      // ever emit this tag. `active` is a real per-instance member (not a
      // stack local), so it correctly reflects "already inside" across the
      // WHOLE print pass regardless of call depth — `out` is one single,
      // persistent object for the pass's own duration.
      bool active{false};
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        if(active) return Base::printMenu(i,ctx);
        active=true;
        Base::template fmtStart<tag>(ctx);
        bool r=Base::printMenu(i,ctx);
        Base::template fmtStop<tag>(ctx);
        active=false;
        return r;
      }
    };
  };

  using ViewPrinter=FmtPrinter<Fmt::View>;
  using AccelPrinter=FmtPrinter<Fmt::Accel>;
  using LabelPrinter=FmtPrinter<Fmt::Label>;
  using FieldPrinter=FmtPrinter<Fmt::Field>;
  using UnitPrinter=FmtPrinter<Fmt::Unit>;
  // using EditModePrinter=FmtPrinter<Fmt::EditMode>;
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

// hapi::Traverse only auto-recurses a real Chain<OO...> — oneMenu::MenuPrinter<OO...>
// (above) is a distinct wrapping struct, opaque to any hapi::TagIs/query scan unless
// it gets this same treatment explicitly. Without it, a flat query<Q, SomeOutDef::Types>
// walk can never see a tag (e.g. ScrollBodyPrinter's aScrollBody) buried inside
// MenuPrinter<TitlePrinter,ScrollBodyPrinter,ItemsPrinter>'s own template args
// (ScrollPrinter, below) — confirmed via a real SIGSEGV: nav.h's printTo() gates
// allocating a real tops[] array on exactly that check. This specialization alone
// wasn't sufficient on its own, though — nav.h's own check was ALSO passing the bare
// Out (an opaque OutDef<Chain<...>> wrapper) instead of Out::Types, so Traverse never
// even got as far as this specialization; see nav.h's printTo() for that half of the
// fix, plus TagIs<...>::Check<...>::value needing hapi::query<...> instead (Check<> on
// a multi-element chain yields a Chain-of-results tree, not a plain bool).
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

  /// @brief Starts body printing by redirecting to the item; chains to Base::printMenu.
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
      using HasScrollBody=std::true_type;
      using Base=typename BodyPrinter::Part<P>;
      using Base::lockMode;
      using Base::getPos;
      using Base::free;
      using Base::setPos;

      // Was: "did the LAST item the walk visited (ci-1) overflow" — blamed
      // whichever item happened to be positioned last for ANY cumulative
      // overflow, even when an EARLIER item in the window was the real space
      // hog. Concretely: window [multiline, opt1, opt2] slightly overflows
      // the area because multiline (3 lines) is tall — while opt1 was
      // selected this was accepted (opt1 isn't ci-1, so the old check never
      // fired), but the instant selection moved to opt2 (now == ci-1), the
      // SAME window got blamed on opt2 and triggered an unnecessary scroll —
      // found on real ST7789 hardware, selecting the last visible item
      // scrolled even though that item had visibly ample room below it.
      // Track whether the SELECTED item specifically (not whatever's last)
      // stayed within bounds — set from printItem below, right after
      // whichever item is ctx.sel() actually prints.
      bool m_selFits{false};

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
          m_selFits=false;
          // measure body only — fmtStop<Footer> emits nl() which would corrupt free().y
          Base::template fmtStart<Fmt::Body>(ctx);
          i.printBody(Base::obj(),ctx);
          Base::template fmtStop<Fmt::Body>(ctx);
          Sz ci=ctx.idx;
          ctx.idx=0;
          // Bound by the last item: top can never usefully scroll past it. Without this,
          // a page that always measures as "the selected item didn't fully fit" never
          // breaks on its own — e.g. a FullScreen item deliberately consumes the *entire*
          // remaining page. Once top reaches the last item there's nowhere further to
          // scroll, so stop regardless — this is also the exactly-correct landing spot
          // for a FullScreen page (top==sel).
          if((ctx.sel(ctx.pAt)<ci&&m_selFits)||ctx.top()>=i.size()-1) break;
          setPos(Pos{x,y});
          ctx.top(ctx.top()+1);//--scroll down
          om=LockMode::None;//scroll => full redraw
        };
        lockMode(om);
        setPos({x,y});
        // No explicit "did a scroll happen" bookkeeping needed — same shape as
        // ANSIFmt::fmtStart<Fmt::View>'s unconditional clear() call: fillRect is already
        // Gate-gated (out.h, if(unlocked())), so this is a no-op unless om ended up None,
        // which is exactly "a scroll happened" here (Changed/Sync already returned above,
        // and entering-as-None is handled by Fmt::View's own clear — this one only needs
        // to additionally cover the body region for the scroll case Fmt::View can't see,
        // see below). Full-screen Base::clear() is NOT safe here, though: TitlePrinter
        // already ran, further out in the chain, before ScrollBodyPrinter ever got
        // control — if this pass entered as Update (locked) and title's own content
        // didn't change, its real pixels were never touched this frame, and nothing
        // downstream re-prints it once we're here. Body-region only instead: (x,y) is
        // exactly the top-left of the body (captured above, right after Title finished)
        // — fillRect from there down to the bottom of the declared area, using whatever
        // color is currently set (the last item measured above — setColors() isn't
        // Gate-guarded, so it took real effect even during the Measure-mode walk).
        Base::fillRect(x, y, Base::width()-x, Base::height()-y);
        // fillRect (ansiOut.h) walks row-by-row via its own internal setPos(x,row) calls,
        // ending parked at the last cleared row — that never reaches the outer Cursor<>'s
        // own m_at (fillRect isn't one of the methods Cursor intercepts), so Cursor still
        // logically believes it's sitting at (x,y) while the real device cursor is actually
        // at the bottom of the just-cleared region. The real item walk below prints
        // sequentially via plain nl() chaining with no per-item reposition (that's only
        // ItemPrinter's Update-mode force-unlock path) — without this, item 0 lands wherever
        // fillRect's last row left the real cursor instead of its own row. Re-sync now, same
        // idiom as the setPos({x,y}) above it and ItemPrinter's own setPos(getPos()).
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
        bool isSel=ctx.idx==ctx.sel(ctx.pAt);
        // attempted, not r: Base::printItem()'s return is "did this item's content
        // change" (ItemBodyPrinter ORs in i.changed()), not "did it fit" — ANDing
        // m_selFits against r made it false almost unconditionally, found via a
        // native trace showing r=false on every single item every single pass.
        bool attempted=Base::free().y>0;
        bool r=attempted?Base::printItem(i,ctx):false;
        // Right after THIS item (not whatever prints last) — did it, specifically, stay
        // in bounds? free() may legitimately be tight/negative afterward if a LATER
        // item also gets drawn and overflows; that's not this item's fault.
        if(isSel) m_selFits=attempted&&Base::free().y>=0;
        return r;
      }
    };
  };

  /// @brief Like ScrollBodyPrinter but without the scroll-search: top is always exactly ctx.sel().
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
        // LiquidBox<x,y,cellW> (item.h) carries its own on-screen box — bridge it into
        // ctx here, the one place that sees both the concrete item type I (to query the
        // tag) and calls fmtStart<Item> before printItem() runs (GfxColorFmt, which
        // paints the highlight, only ever sees Ctx&, never the item itself). Explicit
        // else so no stale box leaks from a previous LiquidBox item onto a plain one.
        if constexpr(hapi::FromTypes<IsLiquidBox>::template Apply<I>::value) {
          ctx.hasBox=true;
          ctx.boxPos=I::liquidBoxPos();
          ctx.boxSize=i.liquidBoxSize();
        } else ctx.hasBox=false;
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

  /// @brief Triggers the item's own enabled/disabled state print, unconditionally for every item.
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

  /// @brief Printer that injects a static data value as an item prefix.
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
      // Must thread through BOTH halves printItem() below already does —
      // Base::print (the wrapped OO... content, e.g. AsLabel<Text>'s own
      // Text) AND O::print (the NEXT sibling in the SAME ItemDef fold,
      // e.g. AsEditMode<>'s own "O" is whatever component comes after it,
      // such as StaticText<&title>) — calling only one silently drops the
      // other. Found 2026-07-22: every real menu title in this codebase
      // happens to be a bare StaticText<&...> preceded by a zero-arg marker
      // (AsEditMode<>), so .print() (called only by TitlePrinter, for a
      // Menu's own title) had only ever been exercised through the "O"
      // half — a first attempt that called ONLY Base::print (to fix
      // dateField's own AsLabel<Text> title, the first REAL title with
      // wrapped content instead of a plain sibling) broke every OTHER
      // title's own O::print reach instead. Both halves are needed.
      template<typename Out> void print(Out& out) const { Base::print(out); O::print(out); }
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

  /// @brief Just the item's own content — no index number, no enabled-state fmt, no nav
  /// cursor marker. For a printer stack meant to show exactly one item's content standing
  /// alone (e.g. SelectedPrinter, below) rather than a row inside a visible list —
  /// index/cursor chrome only makes sense when sibling rows are also on screen to compare
  /// against.
  using PlainItemsPrinter=ItemPrinter<ItemBodyPrinter>;

  // Full printers: title + body + footer
  using FullPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,BodyPrinter,ItemsPrinter>
  >;
  using ScrollPrinter=Chain<
    ViewPrinter,
    MenuPrinter<
      TitlePrinter,
      ScrollBodyPrinter,
      ItemsPrinter
    >
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
  // PlainItemsPrinter, not ItemsPrinter: this shows exactly one item standing alone (no
  // sibling rows ever visible at once), so an index number / nav cursor marker would be
  // noise, not signal — zero other consumers today (only neurMenu's `footer` device), so
  // safe to keep chrome-less rather than adding yet another printer-stack alias for it.
  using SelectedPrinter=Chain<
    ViewPrinter,
    MenuPrinter<SelectBodyPrinter,PlainItemsPrinter>
  >;
};//oneMenu

// ItemPrinter<OO...> and AsFmt<Fmt tag,OO...> (above) are both the same
// Chain<OO...>::Part<O>-wrapping shape as MenuPrinter — opaque to hapi::query/Traverse
// without their own specialization, exactly like MenuPrinter was before its own fix
// (see that specialization's own comment, above, for the real SIGSEGV this class of
// bug already caused once). Neither has a confirmed live trigger today (no real tag
// happens to be nested inside either one yet) — fixed prophylactically anyway, same
// mechanical pattern, cheap. ItemPrinter is the closer of the two: it's the direct
// one-level-nested sibling of MenuPrinter inside every stock printer stack above
// (FullPrinter, ScrollPrinter, NoTitlePrinter, ...).
namespace hapi {
  template<typename Op, typename... OO>
  struct Traverse<Op, oneMenu::ItemPrinter<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };

  // AsFmt<tag,OO...>::Part<O> is built from Chain<OO...,PartEnd>::Part<O> (PartEnd is
  // AsFmt's own inert terminal sentinel, see its definition above) — mirrored here for
  // structural faithfulness, though PartEnd itself carries no tag so its presence
  // never changes a query's outcome either way.
  template<typename Op, oneMenu::Fmt tag, typename... OO>
  struct Traverse<Op, oneMenu::AsFmt<tag,OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...,
                                                   typename Traverse<Op, typename oneMenu::AsFmt<tag,OO...>::PartEnd>::Beta>;
  };
}