/**
 * @file menu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

#include "oneMenu/menu/item.h"
// #include "tinyTimeUtils.h"

namespace oneMenu {
  // Tag for passing Q as a value to avoid <Q> function-call template syntax (C++17 ambiguity workaround)
  template<typename Q> struct BodyQ {};

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

  namespace detail {
    template<typename B, typename Out, typename=void>
    struct has_printBody : std::false_type {};
    template<typename B, typename Out>
    struct has_printBody<B,Out,std::void_t<
      decltype(std::declval<B&>().printBody(std::declval<Out&>(),std::declval<Ctx&>()))
    >> : std::true_type {};

    template<typename T, typename Out, typename=void>
    struct has_print : std::false_type {};
    template<typename T, typename Out>
    struct has_print<T,Out,std::void_t<
      decltype(std::declval<T&>().print(std::declval<Out&>()))
    >> : std::true_type {};
  }

  // menu ------------------------------------------------------------------------------------
  template<typename T,typename B,typename O,typename... OO>
  struct MenuPart:Chain<OO...>::template Part<O> {
    using Base=typename Chain<OO...>::template Part<O>;
    using Base::Base;
    using Types=Chain<OO...>;
    using Title=T;
    using Body=B;
    Title title;
    Body body;
    MenuPart(Title&& t,Body&& b):title{std::move(t)},body{std::move(b)} {}

    // Shadows Hapi::Part::find<Q>() — searches chain first, then body
    template<typename Q> auto& find() {
      if constexpr (hapi::query<Q, Types>)
        return hapi::template find<Q>(*this);
      else { using BQ = BodyQ<Q>; return findBody(BQ{}, body); }
    }
    template<typename Q> const auto& find() const {
      if constexpr (hapi::query<Q, Types>)
        return hapi::template find<Q>(*this);
      else { using BQ = BodyQ<Q>; return findBody(BQ{}, body); }
    }

    // template<typename Out> void print(Out& out) const {
    //   static_assert(detail::has_print<Title,Out>::value,
    //     "Title::print(Out&) missing");
    //   title.print(out);
    // }
    // template<typename Out> bool printBody(Out& out,Ctx& ctx) {
    //   if constexpr (detail::has_printBody<Body,Out>::value) {
    //     return body.printBody(out,ctx);
    //   } else {
    //     static_assert(detail::has_printBody<Body,Out>::value,
    //       "Body::printBody(Out&,Ctx&) missing");
    //     return false;
    //   }
    // }
    // template<typename Out> bool printMenu(Out& out,Ctx& ctx) {
    //   print(out);
    //   return printBody(out,ctx);
    // }
    template<typename Out>
    void print(Out& out) const {
      title.print(out);
      if(Base::isPad()) {//<----- this is a pad... (second pass) lets print the body inplace, will need a new ctx thou, the original will be messed up
        // Ctx tmp{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,(Depth)(ctx.at+(Depth)1),0,true,0,ctx.idx};
        // {Ctx tmp{{0}};body.printBody(out,tmp);}
        body.print(out);
      }
    }
    template<typename Out> void print(Out& out,Ctx&) {print(out);}
    Sz size() const {return body.size();}

    template<typename Out>
    bool printMenu(Out& out,Ctx& ctx) {
      if(ctx.pAt>ctx.at){//walk to print level
        //TODO: can this tmp be an update of ctx?
        Ctx tmp{ctx.path,ctx.mode,ctx.pAt,ctx.enabled,ctx.tops,(Depth)(ctx.at+1),0,ctx.pad,0, ctx.idx };
        Sz s=ctx.sel();
        return body.printMenu(out,tmp,s);
      }
      ctx.at++;
      bool r=out.printMenu(/*obj()*/*this,ctx);// print the target menu
      return r;
    }

    template<typename Out> 
    bool printBody(Out& out,Ctx& ctx)
      {return body.printBody(out,ctx);}

    template<typename Out>
    bool printItem(Out& out,Ctx& ctx)
      {return title.printItem(out,ctx);}

    static constexpr Depth depth() {return Body::depth()+1;}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path p) {
      if(p.len>0)
        return body.template nav<isKbd>(n,cke,p.next(),p.sel())
        ||Base::template nav<isKbd>(n,cke,p)
        ||(p.len==1&&n.doNav(cke,size(),Base::wraps()));
      return Base::template nav<isKbd>(n,cke,p);
    }

  };

  template<typename T,typename B,typename... OO>
  struct Menu {
    using Types=Chain<OO...>;
    // template<typename Before,typename After>
    // static constexpr bool rules() {return CheckRules<typename Types::Head,typename Types::Tail,Chain<>>::check();}
    template<typename P> using Part=MenuPart<T,B,P,OO...>;
  };

  template<typename T,typename B>
  struct Menu<T,B> {
    using Types=Chain<>;
    template<typename O> using Part=MenuPart<T,B,O>;
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
    else { using BQ = BodyQ<Q>; return findBody(BQ{}, node.body); }
  }
  template<typename Q, typename T, typename B, typename... MM>
  const auto& find(const ItemDef<Menu<T,B,MM...>>& node) {
    using Node = ItemDef<Menu<T,B,MM...>>;
    if constexpr (hapi::query<Q, typename Node::Types::Tail>)
      return hapi::find<Q>(node);
    else { using BQ = BodyQ<Q>; return findBody(BQ{}, node.body); }
  }

};//oneMenu
