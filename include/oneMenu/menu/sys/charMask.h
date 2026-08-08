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
#pragma once

#include "oneMenu/menu/sys/base.h"

namespace CharMask {
  using oneData::CText;

  /// @brief Values within [l, h].
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

  /// @brief Character range [l, h].
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

  /// @brief Combines multiple character masks and cycles between them.
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

  /// @brief Per-character-position mask: position `pos` is validated/cycled against M0 (pos%n()==0) or MM...[pos%n()-1], repeating cyclically.
  template<typename M0,typename... MM>
  struct PerPos {
    static constexpr int n() {return 1+(int)sizeof...(MM);}
    private:
    // Walk(I) selects pack member I (0-based, already reduced mod n()) —
    // an unrolled linear search over the pack, same recursive "one
    // comparison per level" shape as _Ranges' own Prev/Next chain above,
    // not a function-pointer table: n() is always small (a handful of
    // real validator entries), so this costs nothing that matters and
    // avoids AVR's fn-ptr-as-static-member friction entirely.
    template<int I,typename N0,typename... NN> struct At {
      static bool chk(int i,char c)  {return i==I? N0::chk(c) : At<I+1,NN...>::chk(i,c);}
      static char up(int i,char c)   {return i==I? N0::up(c)  : At<I+1,NN...>::up(i,c);}
      static char down(int i,char c) {return i==I? N0::down(c): At<I+1,NN...>::down(i,c);}
    };
    template<int I,typename N0> struct At<I,N0> {
      // last pack entry — i is already pos%n(), guaranteed in range, so no
      // further comparison needed here.
      static bool chk(int,char c)  {return N0::chk(c);}
      static char up(int,char c)   {return N0::up(c);}
      static char down(int,char c) {return N0::down(c);}
    };
    using Walk=At<0,M0,MM...>;
    public:
    static bool chk(int pos,char c)  {return Walk::chk(pos%n(),c);}
    static char up(int pos,char c)   {return Walk::up(pos%n(),c);}
    static char down(int pos,char c) {return Walk::down(pos%n(),c);}
  };

  /// @brief Position-indexed character-set mask: one C-string per buffer position, repeating cyclically via pos%N once N is shorter than the buffer.
  /// @tparam V validators array address; declare as `static const char* validators[]` (not `const char* const*`)
  /// @tparam N number of entries in the validators array
  template<CText* V,int N>
  struct PosSet {
    static int len(int pos) {return (int)strlen(V[pos%N]);}
    static int find(int pos,unsigned char c) {
      const char* s=V[pos%N];
      for(int n=0;n<(int)strlen(s);n++) if(s[n]==c) return n;
      return -1;
    }
    static bool chk(int pos,unsigned char c) {return find(pos,c)>=0;}
    static unsigned char up(int pos,unsigned char c) {
      int i=find(pos,c);
      return i<0? V[pos%N][0] : i==len(pos)-1? V[pos%N][0] : V[pos%N][i+1];
    }
    static unsigned char down(int pos,unsigned char c) {
      int i=find(pos,c);
      return i<0? V[pos%N][0] : i? V[pos%N][i-1] : V[pos%N][len(pos)-1];
    }
  };
};//namespace CharMask
