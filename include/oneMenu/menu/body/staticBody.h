/**
 * @file body.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

namespace oneMenu {
  // body ------------------------------------

  template<typename...> struct StaticBody;

  template<> struct StaticBody<>:Chain<> {
    template<typename Out> static constexpr bool printBody(Out& out) {return false;}
  };

  template<typename O,typename... OO>
  struct StaticBody<O,OO...>:Chain<O,OO...> {
    using Head=O;
    using Tail=StaticBody<OO...>;
    Head head;
    Tail tail;
    template<typename I,typename... II>
    constexpr StaticBody(I&& o,II&&... oo):head{std::forward<I>(o)},tail{std::forward<II>(oo)...}{}
    template<typename... II>
    constexpr StaticBody(II&&... oo):head{},tail{std::forward<II>(oo)...}{}
    template<typename Out> bool printBody(Out& out) const {
      head.print(out);//TODO change to `printItem` or `printTo` verify name available
      return tail.printBody(out);}
  };

  //static body factory ---
  template<typename... OO> constexpr StaticBody<OO...> staticBody(OO&&... oo) {return {std::forward<OO>(oo)...};}
};//namespace oneMenu

//rules StaticBody query specialization --
template<typename Q,typename... OO>
constexpr const bool hapi::query<Q,oneMenu::StaticBody<OO...>>{(hapi::query<Q,OO>||...)};

