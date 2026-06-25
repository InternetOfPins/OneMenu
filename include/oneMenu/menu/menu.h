/**
 * @file menu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief
*/

#pragma once

#include "oneMenu/menu/item.h"

namespace oneMenu {
  // Tag for passing Q as a value to avoid <Q> function-call template syntax (C++17 ambiguity workaround)
  template<typename Q> struct BodyQ {};

  namespace detail {
    // Declared here (BodyQ<Q> available), defined in staticBody.h (Body::Head/Tail available).
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int> = 0>
    auto& findBody(BodyQ<Q>, Body&);
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int> = 0>
    const auto& findBody(BodyQ<Q>, const Body&);
  }

  //TODO: make this just a query target, it will give the boolean we need, we can generalize this type of tags here
  struct WrapNav {
    template<typename I>
    struct Part:I {
      static constexpr bool wraps() {return true;}
    };
  };

  struct PadDraw {
    template<typename I>
    struct Part:ParentDraw::template Part<I> {
      static constexpr bool isPad() {return true;}
    };
  };

  // menu ------------------------------------------------------------------------------------
  template<typename T,typename B,typename... OO>
  struct Menu {
    using Chain_=hapi::Chain<OO...>;
    using Types=Chain_; // needed for hapi::query traversal into this component
    template<typename O>
    struct Part : Chain_::template Part<O> {
      using Base=typename Chain_::template Part<O>;
      using Base::Base;
      using Types=Chain_;
      using Title=T;
      using Body=B;
      Title title;
      Body body;
      Part(Title&& t,Body&& b):title{std::move(t)},body{std::move(b)} {}

      // Shadows Hapi::Part::find<Q>() — searches chain first, then body
      template<typename Q> auto& find() {
        if constexpr (hapi::query<Q, Types>)
          return hapi::template find<Q>(*this);
        else {
          static_assert(hapi::query<Q, Body>, "find<Q>: Q not found in chain or body");
          using BQ = BodyQ<Q>; return detail::findBody(BQ{}, body);
        }
      }
      template<typename Q> const auto& find() const {
        if constexpr (hapi::query<Q, Types>)
          return hapi::template find<Q>(*this);
        else {
          static_assert(hapi::query<Q, Body>, "find<Q>: Q not found in chain or body");
          using BQ = BodyQ<Q>; return detail::findBody(BQ{}, body);
        }
      }
      template<typename Q> auto& find(Q) {
        static_assert(hapi::is_predicate<Q>::value,"find(Q{}): Q must be a hapi predicate");
        return find<Q>();
      }
      template<typename Q> const auto& find(Q) const {
        static_assert(hapi::is_predicate<Q>::value,"find(Q{}): Q must be a hapi predicate");
        return find<Q>();
      }

      bool changed() const {return Base::changed()||body.changed();}
      void sync() {Base::sync();body.sync();}

      template<typename Out>
      void print(Out& out) const {
        title.print(out);
        if constexpr(Base::isPad()) body.print(out);
      }
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

      template<typename Out>
      bool printHiddenMenu(Out& out,Ctx& ctx) {
        if(ctx.pAt>ctx.at) {
          Ctx tmp{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,(Depth)(ctx.at+1),ctx.prev,ctx.pad,0,ctx.idx};
          Sz s=ctx.sel();
          return body.printHiddenMenu(out,tmp,s);
        }
        ctx.at++;
        return body.printHiddenBody(out,ctx);
      }

      template<typename Out>
      bool printItem(Out& out,Ctx& ctx) {
        bool r=title.printItem(out,ctx);
        if constexpr(Base::isPad()) {
          // Inline body display: one level deeper so focus-check uses the pad's selection.
          // pad=true suppresses newlines between items (TextFmt::fmtStop skips nl when pad).
          // pIdx=ctx.idx: tells ANSIFmt which parent-body slot owns this pad, enabling
          // the psel() focus check for color/EditMode highlighting of inline sub-items.
          Ctx inner{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,
                    (Depth)(ctx.at+1),ctx.prev,true,0,ctx.idx};
          body.printInline(out,inner);
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
    };
  };

  //menu factory ---
  template<typename... MM,typename T,typename B>
  constexpr ItemDef<Menu<T,B,MM...>> menuDef(T&& t,B&& b) {return {std::forward<T>(t),std::forward<B>(b)};}

  template <typename T, typename B,typename... OO> using PadMenu=ItemDef<Menu<T,B,PadDraw,OO...>>;
  template <typename T, typename B,typename... OO> using MenuDef=ItemDef<Menu<T,B,OO...>>;
  template <typename T, typename B,typename... OO> using IMenuDef=IItemDef<Menu<T,B,OO...>>;

  // find<Q>(menu): more specialized than hapi::find — wins via partial ordering; tries chain then body.
  // Uses BodyQ<Q> tag dispatch to avoid C++17 template-arg-as-less-than ambiguity.
  template<typename Q, typename T, typename B, typename... MM>
  auto& find(ItemDef<Menu<T,B,MM...>>& node) {
    using Node = ItemDef<Menu<T,B,MM...>>;
    if constexpr (hapi::query<Q, typename Node::Types::Tail>)
      return hapi::find<Q>(node);
    else { using BQ = BodyQ<Q>; return detail::findBody(BQ{}, node.body); }
  }
  template<typename Q, typename T, typename B, typename... MM>
  const auto& find(const ItemDef<Menu<T,B,MM...>>& node) {
    using Node = ItemDef<Menu<T,B,MM...>>;
    if constexpr (hapi::query<Q, typename Node::Types::Tail>)
      return hapi::find<Q>(node);
    else { using BQ = BodyQ<Q>; return detail::findBody(BQ{}, node.body); }
  }

};//oneMenu
