/**
 * @file charMask.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief characters mask/validation/ranges
 * @version 5
 * @date 2026-05-07
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#include "oneMenu/sys/base.h"

namespace CharMask {
  
  /// @brief values between l and t
  /// @tparam T value type
  /// @tparam l lower value
  /// @tparam h higher value
  template<typename T,T l,T h>
  struct Range {
    static constexpr T high() {return h;}
    static constexpr T low() {return l;}
    static constexpr bool high(T c) {return c==h;}
    static constexpr bool low(T c) {return c==l;}
    static constexpr T up(T c) {return c>=h?l:++c;}
    static constexpr T down(T c) {return c<=l?h:--c;}
    static constexpr bool chk(T c) {return l<=c&&c<=h;}
  };

  /// @brief string of allowed characters
  /// @tparam text 
  template<CText* text>
  struct Set {
    static constexpr int len() {return strlen(text[0]);}
    static constexpr char high() {return text[0][len()-1];}
    static constexpr char low() {return text[0][0];}
    static constexpr bool high(char c) {return c==high();}
    static constexpr bool low(char c) {return c==low();}
    static int at(unsigned char c) {
      for(int n=0;n<strlen(text[0]);n++)
        if(text[0][n]==c) return n;
      return -1;
    }
    static constexpr unsigned char up(unsigned char c) {
      return at(c)<0?
        text[0][0]:
        at(c)==len()-1?
          text[0][0]:
          text[0][at(c)+1];
    }
    static constexpr unsigned char down(unsigned char c) {
      return at(c)<0?
        text[0][0]:
        at(c)?
          text[0][at(c)-1]:
          text[0][len()-1];
    }
    static constexpr bool chk(unsigned char c) {return at(c)>=0;}
  };

  /// @brief characters range
  /// @tparam l lower value
  /// @tparam h higher value
  template<unsigned char l,unsigned char h> using CharRange=Range<unsigned char,l,h>;

  /// @brief common character ranges type definitions
  /// @{
  /**digits [0-9]*/
  using Digits=CharRange<'0','9'>;
  /**lower case leters [a-z]*/
  using Letters=CharRange<'a','z'>;
  /**capital letters [A-Z]*/
  using Caps=CharRange<'A','Z'>;
  /**7 bits characters*/
  using ASCII7=CharRange<' ','~'>;
  /**8 bits characters*/
  using ASCII8=CharRange<' ',(unsigned char)0xFF>;
  /// @}

  template<typename P,typename O,typename... OO> struct _Ranges;

  template<typename P,typename O,typename N,typename... OO>
  struct _Ranges<P,O,N,OO...>:O {
    using Base=O;
    using Prev=P;
    using Next=_Ranges<_Ranges<P,O,OO...>,N,OO...>;
    using Front=typename Prev::Front;
    using Back=typename Next::Back;
    static constexpr int sz(){return 1+_Ranges<N,OO...>::sz();}
    static bool chk(unsigned char c) {return O::chk(c)||Next::chk(c);}
    static unsigned char up(unsigned char c) 
      {return O::chk(c)?(O::high(c)?Next::low():O::up(c)):Next::up(c);}
    static unsigned char down(unsigned char c) 
      {return O::chk(c)?(O::low(c)?Prev::high():O::down(c)):Next::down(c);}
  };

  template<typename P,typename O,typename N>
  struct _Ranges<P,O,_Ranges<_Ranges<P,O,N>,N>>:O {
    using Prev=P;
    using Base=O;
    using Next=N;
    using Front=typename Prev::Front;
    using Back=typename Next::Back;
    static constexpr int sz(){return 2;}
    static bool chk(char c) {return O::chk(c)||Next::chk(c);}
    static char up(char c) 
      {return O::chk(c)?(O::high(c)?Next::low():O::up(c)):Next::up(c);}
    static char down(char c) 
      {return O::chk(c)?(O::low(c)?P::high():O::down(c)):Next::down(c);}
  };

  template<typename P,typename O>
  struct _Ranges<P,O>:O {
    using Base=O;
    using Prev=P;
    using Front=typename Prev::Front;
    using Back=_Ranges<P,O>;
    static constexpr int sz(){return 1;}
    static bool chk(char c) {return O::chk(c);}
    static char up(char c) 
      {return O::chk(c)?(O::high(c)?Front::low():O::up(c)):Front::low();}
    static char down(char c) 
      {return O::chk(c)?(O::low(c)?Prev::high():O::down(c)):Base::high();}
  };

  /// @brief combines multiple character masks and enumerates between them
  /// @tparam O first mask type
  /// @tparam ...OO optional mask types
  template<typename O,typename... OO>
  struct Ranges:O {
    using Base=O;
    using Front=Ranges<O,OO...>;
    using Next=_Ranges<Ranges<O,OO...>,OO...>;
    using Back=typename Next::Back;
    static constexpr int sz(){return 1+_Ranges<O,OO...>::sz();}
    static bool chk(char c) {return O::chk(c)||Next::chk(c);}
    static char up(char c) 
      {return O::chk(c)?(O::high(c)?Next::low():O::up(c)):Next::up(c);}
    static char down(char c) 
      {return O::chk(c)?(O::low(c)?Back::high():O::down(c)):Next::down(c);}
  };

  /// @brief last mask on the Ranges list
  /// @tparam O mask type
  template<typename O>
  struct Ranges<O>:O {
    using Front=Ranges<O>;
    using Back=Ranges<O>;
    static constexpr int sz(){return 1;}
  };
};//namespace CharMask
