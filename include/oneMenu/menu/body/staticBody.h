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
    template<bool isKbd,typename Nav>
    static constexpr bool nav(Nav&,const CKE&,Path,Sz=0) noexcept {return false;}
    static constexpr bool changed() noexcept {return false;}
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

  template<bool isKbd,typename Nav>
  bool nav(Nav& n,const CKE& cke,Path path,Sz i)
    {return i?((Tail&)tail).template nav<isKbd>(n,cke,path,i-1):head.template nav<isKbd>(n,cke,path);}

  bool changed() {return head.changed()||((Tail&)tail).changed();}

  };

  template<typename... OO>
  constexpr StaticBody<OO...> staticBody(OO&&... oo)
    {return StaticBody<OO...>{std::forward<OO>(oo)...};}

  template<typename Q, typename Body>
  auto& findBody(Body& body) {
    if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
    else return findBody<Q>(body.tail);
  }
  template<typename Q, typename Body>
  const auto& findBody(const Body& body) {
    if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
    else return findBody<Q>(body.tail);
  }
  template<typename Q>
  auto& findBody(StaticBody<>&) {
    static_assert(sizeof(Q)==0, "findBody<>: no item satisfies predicate Q");
  }

  // Tag-dispatched overloads: findBody(BodyQ<Q>{}, body)
  // Used from contexts where findBody<Q>(body) cannot be parsed (C++17 template arg ambiguity).
  // BodyQ<Q> is defined in menu.h (included before staticBody.h via fields.h).

  template<typename Q, typename Body>
  auto& findBody(BodyQ<Q>, Body& body) {
    if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
    else return findBody(BodyQ<Q>{}, body.tail);
  }
  template<typename Q, typename Body>
  const auto& findBody(BodyQ<Q>, const Body& body) {
    if constexpr (hapi::query<Q, typename Body::Head>) return body.head;
    else return findBody(BodyQ<Q>{}, body.tail);
  }
  template<typename Q>
  auto& findBody(BodyQ<Q>, StaticBody<>&) {
    static_assert(sizeof(Q)==0, "findBody<>: no item satisfies predicate Q");
  }
  template<typename Q>
  const auto& findBody(BodyQ<Q>, const StaticBody<>&) {
    static_assert(sizeof(Q)==0, "findBody<>: no item satisfies predicate Q");
  }

} // namespace oneMenu
