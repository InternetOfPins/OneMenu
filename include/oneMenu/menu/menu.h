/**
 * @file menu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief
*/

#pragma once

#include "oneMenu/menu/item.h"
#include "oneMenu/menu/body/staticBody.h"  // StaticBody must be complete for detail::find below

namespace oneMenu {
  template<typename T,typename B,typename... OO> struct Menu; // forward — needed by detail::find below

  // recursive structural search: descends Menu -> body -> nested items/menus until Q's
  // own immediate chain is found. A miss is a compile error (dead end has no overload) —
  // mirrors hapi::FindFirst's philosophy. Declared here, defined below (needs Menu complete).
  namespace detail {
    template<typename Q, typename O, typename... OO> decltype(auto) find(StaticBody<O,OO...>&);
    template<typename Q, typename O, typename... OO> decltype(auto) find(const StaticBody<O,OO...>&);
    template<typename Q, typename T, typename B, typename... MM> decltype(auto) find(ItemDef<Menu<T,B,MM...>>&);
    template<typename Q, typename T, typename B, typename... MM> decltype(auto) find(const ItemDef<Menu<T,B,MM...>>&);
    template<typename Q, typename... OO> decltype(auto) find(ItemDef<OO...>&);
    template<typename Q, typename... OO> decltype(auto) find(const ItemDef<OO...>&);
  }

  struct WrapNav {
    template<typename I>
    struct Part:I {
      static constexpr bool wraps() {return true;}
    };
  };

  /// @brief pure marker — no behavior of its own, just makes "this Menu<> is
  /// a Choose field's own inner body" queryable via hapi::query<hapi::SameAs
  /// <IsChoiceBody>,typename Menu::Types> (MenuPrinter, printers.h), letting
  /// a web client (XmlFmt) tell "these sibling items are selectable options"
  /// apart from an ordinary nested submenu with independently-editable
  /// fields. Spliced into ChooseFieldDef's own Menu<T,B,IsChoiceBody,OO...>
  /// (fields.h) / am4.h's chooseDef() only — no other Menu<> composes it.
  struct IsChoiceBody {
    template<typename I>
    struct Part:I {
      using I::I;
    };
  };

  struct PadDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<ItemNav>, Before, After>,
        "PadDraw: ItemNav cannot coexist with PadDraw — PadDraw handles Enter/close; remove ItemNav");
      static_assert(Excludes<hapi::SameAs<ParentDraw>, Before, After>,
        "PadDraw: use either PadDraw or ParentDraw, not both");
      return true;
    }
    template<typename I>
    struct Part:ParentDraw::template Part<I> {
      static constexpr bool isPad() {return true;}
    };
  };

  // menu ------------------------------------------------------------------------------------
  template<typename T,typename B,typename... OO>
  struct Menu {
    using Chain_=hapi::Chain<OO...>;
    using Types=hapi::Chain<OO..., B>; // expose body for compile-time queries (StaticBody only)
    template<typename O>
    struct Part : Chain_::template Part<O> {
      using Base=typename Chain_::template Part<O>;
      using Base::Base;
      using Types=hapi::Chain<OO..., B>; // expose body for compile-time queries (StaticBody only)
      using Title=T;
      using Body=B;
      Title title;
      Body body;
      Part(Title&& t,Body&& b):title{std::move(t)},body{std::move(b)} {}

      template<typename Out>
      void print(Out& out) const {title.print(out);}
      template<typename Out> void print(Out& out,Ctx&) {print(out);}
      Sz size() const {return body.size();}

      template<typename Out>
      bool printMenu(Out& out,Ctx& ctx) {
        if(ctx.pAt>ctx.at){
          Ctx tmp{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,(Depth)(ctx.at+1),ctx.prev,ctx.pad,0,ctx.idx};
          Sz s=ctx.sel();
          return body.printMenu(out,tmp,s);
        }
        ctx.at++;
        bool r=out.printMenu(*this,ctx);
        return r;
      }

      template<typename Out>
      bool printBody(Out& out,Ctx& ctx) {return body.printBody(out,ctx);}

      // Jump straight to one item, skipping every sibling — same primitive RecallNavPos
      // already uses for inline pad display (Base::body.printItem(out,ctx,m_sel), item.h).
      // Exposed here so an output-side body printer (e.g. SelectBodyPrinter, printers.h)
      // can reach it the same way printBody() is already exposed for the sequential walk.
      template<typename Out>
      bool printBodyAt(Out& out,Ctx& ctx,Sz idx) {return body.printItem(out,ctx,idx);}

      template<typename Out>
      bool printItem(Out& out,Ctx& ctx) {
        bool r=title.printItem(out,ctx);
        if constexpr(Base::isPad()) {
          // JsonFmt admitted alongside XmlFmt — out.printMenu() below
          // re-enters the WHOLE printer chain format-agnostically;
          // jsonFmt.h's own Menu/Title handling needed a small nesting fix
          // (a named "menu" wrapper key, since JSON can't rely on XML's
          // free element nesting) to be safely re-entrant here.
          if constexpr(hapi::query<IsXmlFmt,typename Out::Types>
                    || hapi::query<IsJsonFmt,typename Out::Types>) {
            // Real menu/title/body/item structure, for a web client — the
            // SAME shape a genuine nested submenu gets (MenuPrinter/
            // TitlePrinter/BodyPrinter/ItemsPrinter, printers.h), so the
            // exact same generic XSLT templates apply with zero pad-
            // specific logic on the client side. Reached via
            // out.printMenu(*this,inner) — the SAME
            // call Menu::Part::printMenu itself makes for a real top-level
            // submenu — re-entering the WHOLE printer chain from the top;
            // FmtPrinter<Fmt::View>'s own re-entrant guard (printers.h)
            // keeps this from re-wrapping a nested, invalid <view> around
            // it (the real, outermost <view> from the top-level print pass
            // is already open at this point).
            //
            // Pad items never actually open a real nav level (Enter calls
            // n.padOpen(), not n.open() — see nav() below), so ctx.path's
            // own REAL stored array has no entry for THIS item's own index
            // at the right depth — unlike a genuine nested submenu, whose
            // own index IS stored there by a real n.open(). Synthesized
            // here instead, mirroring putPath's own "ctx.path always holds
            // exactly ctx.at entries, with the LAST one excluded from the
            // printed prefix since idx supplies it separately" convention
            // (confirmed against a real, working item's own path — e.g.
            // Toggle at depth 2 reads exactly ctx.path.data[0..0], one real
            // ancestor entry, excluding data[1]): THIS item's own real
            // ancestor prefix is ctx.path.data[0..ctx.at-2] (ctx.at-1
            // entries — Toggle-at-depth-2's own equivalent would be just
            // data[0]) — copy that, then place ctx.idx (this item's own
            // index within ITS parent, captured before resetting the inner
            // ctx's own idx to 0 for the sub-items below) as the new "last,
            // excluded" entry — giving pad sub-items real, addressable
            // paths (e.g. "/4/0/") the same way a genuine nested submenu's
            // own items get theirs.
            Sz padPath[8]{};
            Depth n=(Depth)(ctx.at>0?ctx.at-1:0);
            if(n>6) n=6;
            for(Depth i=0;i<n;i++) padPath[i]=ctx.path.data[i];
            padPath[n]=ctx.idx;
            Path syntheticPath{(Depth)(n+2),padPath};
            Ctx inner{syntheticPath,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,
                      (Depth)(ctx.at+1),ctx.prev,ctx.pad,0,ctx.idx};
            out.printMenu(*this,inner);
          } else {
            // Inline body display: one level deeper so focus-check uses the pad's selection.
            // pad=true suppresses newlines between items (TextFmt::fmtStop skips nl when pad).
            // pIdx=ctx.idx: tells ANSIFmt which parent-body slot owns this pad, enabling
            // the psel() focus check for color/EditMode highlighting of inline sub-items.
            Ctx inner{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,
                      (Depth)(ctx.at+1),ctx.prev,true,0,ctx.idx};
            body.printInline(out,inner);
          }
        }
        return r;
      }

      static constexpr Depth depth() {return Body::depth()+1;}

      template<typename Nav>
      bool setStr(Nav& n,const char* s,Path p) {
        if(p.len>0) return body.setStr(n,s,p.next(),p.sel());
        return Base::setStr(n,s,p);
      }

      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,Path p) {
        if(p.len>0) {
          bool inEdit=n.navMode()==NavMode::Edit;
          // In edit mode, route all keys to the focused item (not doNav).
          return (p.len>1||cke.cmd==Cmd::Enter||(p.len==1&&inEdit)
              ?body.template nav<isKbd>(n,cke,p.next(),p.sel()):false)
            ||Base::template nav<isKbd>(n,cke,p)
            ||(p.len==1&&!inEdit&&(n.doNav(cke,size(),Base::wraps())||(cke.cmd==Cmd::Enter&&n.close())));
        }
        if(cke.cmd==Cmd::Enter) return Base::isPad()?n.padOpen():n.open();
        return Base::template nav<isKbd>(n,cke,p);
      }

      /// @brief locate the item carrying Q (e.g. an Id<V>): own chain (OO...) wins,
      /// else descend into body. Compile error if Q is nowhere to be found.
      template<typename Q>
      decltype(auto) find() {
        if constexpr((hapi::query<Q,OO> || ...) || hapi::query<Q,Title>) return (*this);
        else return oneMenu::detail::find<Q>(body);
      }
      template<typename Q>
      decltype(auto) find() const {
        if constexpr((hapi::query<Q,OO> || ...) || hapi::query<Q,Title>) return (*this);
        else return oneMenu::detail::find<Q>(body);
      }
      /// @brief same as find<Q>(), Q passed by value to avoid `<>` at the call site, e.g. find(byId<id>)
      template<typename Q>
      decltype(auto) find(Q) {return find<Q>();}
      template<typename Q>
      decltype(auto) find(Q) const {return find<Q>();}
    };
  };

  //menu factory ---
  template<typename... MM,typename T,typename B>
  constexpr ItemDef<Menu<T,B,MM...>> menuDef(T&& t,B&& b) {return {std::forward<T>(t),std::forward<B>(b)};}

  template <typename T, typename B,typename... OO> using PadMenu=ItemDef<Menu<T,B,PadDraw,OO...>>;
  template <typename T, typename B,typename... OO> using MenuDef=ItemDef<Menu<T,B,OO...>>;
  template <typename T, typename B,typename... OO> using IMenuDef=IItemDef<Menu<T,B,OO...>>;

};//oneMenu

// Traverse specialization: expose body structure to HAPI queries
namespace hapi {
  template<typename Op, typename T, typename B, typename... MM>
  struct Traverse<Op, oneMenu::Menu<T,B,MM...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, MM>::Beta...,
                                                   typename Traverse<Op, B>::Beta>;
  };
}

// query<Q,Menu<...>> specialization: keeps hapi::query recursive through Menu nesting,
// consistent with the ItemDef/StaticBody specializations (Title, own chain OO..., or the body).
template<typename Q, typename T, typename B, typename... OO>
constexpr const bool hapi::template query<Q, oneMenu::Menu<T,B,OO...>>{
  hapi::template query<Q, T> || (hapi::template query<Q, OO> || ...) || hapi::template query<Q, B>
};

// detail::find definitions — needs Menu and ItemDef complete (above), so they live here.
namespace oneMenu { namespace detail {

  // StaticBody: descend to head if it carries Q, else keep walking the tail.
  // StaticBody<> has no overload here — a miss is a compile error (dead end).
  template<typename Q, typename O, typename... OO>
  decltype(auto) find(StaticBody<O,OO...>& b) {
    if constexpr(hapi::query<Q,O>) return find<Q>(b.head);
    else return find<Q>(b.tail);
  }
  template<typename Q, typename O, typename... OO>
  decltype(auto) find(const StaticBody<O,OO...>& b) {
    if constexpr(hapi::query<Q,O>) return find<Q>(b.head);
    else return find<Q>(b.tail);
  }

  // ItemDef<Menu<T,B,MM...>>: own chain (Title or MM...) wins, else descend into its body.
  template<typename Q, typename T, typename B, typename... MM>
  decltype(auto) find(ItemDef<Menu<T,B,MM...>>& m) {
    if constexpr(hapi::query<Q,T> || (hapi::query<Q,MM> || ...)) return (m);
    else return find<Q>(m.body);
  }
  template<typename Q, typename T, typename B, typename... MM>
  decltype(auto) find(const ItemDef<Menu<T,B,MM...>>& m) {
    if constexpr(hapi::query<Q,T> || (hapi::query<Q,MM> || ...)) return (m);
    else return find<Q>(m.body);
  }

  // plain ItemDef<OO...> (no nested Menu): terminal — Q is already verified present
  // in OO... by the caller's query<Q,Head> check, so this item itself is the match.
  template<typename Q, typename... OO>
  decltype(auto) find(ItemDef<OO...>& item) {return item;}
  template<typename Q, typename... OO>
  decltype(auto) find(const ItemDef<OO...>& item) {return item;}

}}
