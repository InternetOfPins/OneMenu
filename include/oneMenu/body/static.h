/**
 * @file body.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

// body ------------------------------------

template<typename...> struct StaticBody;

template<> struct StaticBody<>:Chain<> {
  template<typename Out> static constexpr void printBody(Out& out) {}
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
  template<typename Out> void printBody(Out& out) const {head.print(out);tail.printBody(out);}
};

//rules StaticBody query specialization --
template<typename Q,typename... OO>
constexpr const bool query<Q,StaticBody<OO...>>{(query<Q,OO>||...)};

//static body factory ---
template<typename... OO> constexpr StaticBody<OO...> staticBody(OO&&... oo) {return {std::forward<OO>(oo)...};}
