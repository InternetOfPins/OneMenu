/**
 * @file menu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

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
  template<typename Out> void print(Out& out) const {title.print(out);}
  template<typename Out> void printBody(Out& out) const {body.printBody(out);}
  template<typename Out> void printMenu(Out& out) const {
    print(out);
    printBody(out);
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

//rules Menu query specialization --
template<typename Q,typename T,typename B,typename... OO>
constexpr const bool query<Q,Menu<T,B,OO...>>{(query<Q,OO>||...)||query<Q,T>||query<Q,B>};

//menu factory ---
template<typename... MM,typename T,typename B>
constexpr ItemDef<Menu<T,B,MM...>> menuDef(T&& t,B&& b) {return {std::forward<T>(t),std::forward<B>(b)};}
