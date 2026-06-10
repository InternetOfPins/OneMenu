/**
 * @file staticBody.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
*/

#pragma once

namespace oneMenu {

  template<typename...> struct StaticBody;

  template<> struct StaticBody<> : Chain<> {
    template<typename Out> static constexpr void printTo(Out&) noexcept {}
  };

  template<typename O, typename... OO>
  struct StaticBody<O, OO...> : Chain<O, OO...> {
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
  };

  template<typename... OO> 
  constexpr StaticBody<OO...> staticBody(OO&&... oo) {
    return {std::forward<OO>(oo)...};
  }

} // namespace oneMenu

// New optimized HAPI query engine syntax
template<typename Q, typename... OO>
constexpr inline bool hapi::query<Q, oneMenu::StaticBody<OO...>> = (hapi::query<Q, OO> || ...);