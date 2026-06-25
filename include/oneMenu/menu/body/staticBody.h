/**
 * @file staticBody.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

namespace oneMenu {

  template<typename...> struct StaticBody;

  template<> struct StaticBody<> {
    static constexpr Sz size() noexcept {return 0;}
    static constexpr Depth depth() noexcept {return 0;}
    template<typename Out> static constexpr void print(Out&) noexcept {}
    template<typename Out> static constexpr bool printBody(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printMenu(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printItem(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printItem(Out&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printInline(Out&,Ctx&) noexcept {return false;}
    template<bool isKbd,typename Nav>
    static constexpr bool nav(Nav&,const CKE&,Path,Sz=0) noexcept {return false;}
    template<typename Nav,typename P>
    static constexpr bool setStr(Nav&,const char*,P,Sz=0) noexcept {return false;}
    static constexpr bool changed() noexcept {return false;}
    static constexpr void sync() noexcept {}
  };

  template<typename O, typename... OO>
  struct StaticBody<O, OO...> {
    static constexpr Sz size() noexcept {return 1+sizeof...(OO);}
    using Head = O;
    using Tail = StaticBody<OO...>;
    static constexpr Depth depth() {return staticMax<Head::depth(),Tail::depth()>();}
    
    Head head;
    Tail tail;

    template<typename I, typename... II>
    constexpr StaticBody(I&& o, II&&... oo) 
      : head{std::forward<I>(o)}, tail{std::forward<II>(oo)...} {}

    template<typename... II>
    constexpr StaticBody(II&&... oo) 
      : head{}, tail{std::forward<II>(oo)...} {}

    template<typename Out>
    constexpr void print(Out& out) const noexcept {
      head.print(out);
      tail.print(out);
    }

  template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
    {return i?((Tail&)tail).printMenu(out,ctx,i-1):head.printMenu(out,ctx);}

  template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
    bool r=out.printItem(head,ctx);
    return ((Tail&)tail).printBody(out,ctx,bidx+1)||r;
  }

  template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
    {return i?((Tail&)tail).printItem(out,ctx,i-1):head.printItem(out,ctx);}
  template<typename Out> bool printItem(Out& out,Sz i=0)
    {return i?((Tail&)tail).printItem(out,i-1):head.printItem(out);}
  // Inline pad rendering: calls each item's printItem directly (no printer chain),
  // ctx.idx tracks which item is focused for the focus-cursor check.
  template<typename Out> bool printInline(Out& out,Ctx& ctx) {
    bool r=head.printItem(out,ctx);
    ctx.idx++;
    return ((Tail&)tail).printInline(out,ctx)||r;
  }

  template<bool isKbd,typename Nav>
  bool nav(Nav& n,const CKE& cke,Path path,Sz i)
    {return i?((Tail&)tail).template nav<isKbd>(n,cke,path,i-1):head.template nav<isKbd>(n,cke,path);}

  template<typename Nav,typename P>
  bool setStr(Nav& n,const char* s,P p,Sz i)
    {return i?((Tail&)tail).setStr(n,s,p,i-1):head.setStr(n,s,p);}

  bool changed() const {return head.changed()||((const Tail&)tail).changed();}
  void sync() {head.sync();((Tail&)tail).sync();}

  template<typename Fn>
  auto visit(Sz i, Fn&& fn)
    {return i?((Tail&)tail).visit(i-1,std::forward<Fn>(fn)):fn(head);}

  };

  template<typename... OO>
  constexpr StaticBody<OO...> staticBody(OO&&... oo)
    {return StaticBody<OO...>{std::forward<OO>(oo)...};}

  namespace detail {
    // Internal body search — use find<Q>(node) from the public API, not these directly.
    // SFINAE on query<Q,Body>: absent Q → "no matching function" at the find<Q> call site.
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int> = 0>
    auto& findBody(Body& body) {
      if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
      else return findBody<Q>(body.tail);
    }
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int> = 0>
    const auto& findBody(const Body& body) {
      if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
      else return findBody<Q>(body.tail);
    }
    // BodyQ<Q> tag-dispatch: avoids C++17 template-arg-as-less-than parse ambiguity.
    // Declared in menu.h (where BodyQ<Q> is defined); defined here where Body::Head/Tail are available.
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int>>
    auto& findBody(BodyQ<Q>, Body& body) {
      if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
      else return findBody(BodyQ<Q>{}, body.tail);
    }
    template<typename Q, typename Body, std::enable_if_t<hapi::query<Q, Body>, int>>
    const auto& findBody(BodyQ<Q>, const Body& body) {
      if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
      else return findBody(BodyQ<Q>{}, body.tail);
    }
  } // namespace detail

} // namespace oneMenu

template<typename Q, typename... OO>
constexpr const bool hapi::template query<Q, oneMenu::StaticBody<OO...>>{
  (hapi::template query<Q, OO> || ...)
};
