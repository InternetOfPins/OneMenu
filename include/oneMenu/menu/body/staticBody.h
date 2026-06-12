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
    template<typename Out> static constexpr void printTo(Out&) noexcept {}
    template<typename Out> static constexpr bool printBody(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printMenu(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printItem(Out&,Ctx&,Sz=0) noexcept {return false;}
    template<typename Out> static constexpr bool printItem(Out&,Sz=0) noexcept {return false;}
    template<bool isKbd,typename Nav>
    static constexpr bool nav(Nav&,const CKE&,Path,Sz=0) noexcept {return false;}
  };

  template<typename O, typename... OO>
  struct StaticBody<O, OO...> : Chain<O, OO...> {
    static constexpr Sz size() noexcept {return 1+sizeof...(OO);}
    static constexpr Depth depth() {return staticMax<Head::depth(),Tail::depth()>();}
    using Head = O;
    using Tail = StaticBody<OO...>;
    
    Head head;
    Tail tail;

    template<typename I, typename... II>
    constexpr StaticBody(I&& o, II&&... oo) 
      : head{std::forward<I>(o)}, tail{std::forward<II>(oo)...} {}

    template<typename... II>
    constexpr StaticBody(II&&... oo) 
      : head{}, tail{std::forward<II>(oo)...} {}

    template<typename Out>
    constexpr void printTo(Out& out) const noexcept {
      head.printTo(out);
      tail.printTo(out);
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

  };

  template<typename... OO>
  constexpr StaticBody<OO...> staticBody(OO&&... oo)
    {return StaticBody<OO...>{std::forward<OO>(oo)...};}

} // namespace oneMenu
