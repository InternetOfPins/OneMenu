/**
 * @file base.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

#include "menu/sys/enums.h"

using oneData::DefaultDataDef;
using oneData::CText;
using oneData::Default;
using oneData::Watch;
using oneData::NumRange;
using oneData::StaticNumRange;

#ifdef ARDUINO
#include <Arduino.h>
#undef max
#endif

#ifdef __AVR__
  #include <assert.h>
  #include "menu/sys/platform/avr/avr_std.h"
#else
  #include <iostream>
  #include <cstdint>
  #include <cassert>
  #include <type_traits>
  #include <utility>
  #include <cstring>
  #include <cstdlib>
  #include <cstdio>
  #include <limits>
  #include <algorithm>
#endif

using Sz=int;//must be signed
#ifdef __AVR__
  using Depth=char;//must be signed
  using Key=unsigned char;
#else
  using Depth=int;//must be signed
  using Key=unsigned int;
#endif 

/// @brief compile time `max(a,b)` function
/// @tparam a value
/// @tparam b value
/// @return Sz
template<const Sz a,const Sz b> constexpr Sz staticMax() {return a>b?a:b;}

// struct Nil{};//moved to hapi

struct IItem;
struct IOut;
struct INav;

// struct XY{Sz x;Sz y;};
// using Pos=XY;
// using Area=XY;
 
struct CKE {
  Cmd cmd;
  Key key;
  bool ext;
};

template<typename Cor> struct Colors{Cor fg;Cor bg;};

struct Path {
  Depth len;
  Sz* data;
  constexpr Sz sel(Depth i=0) const {return len>i?data[(int)i]:0;}
  constexpr Sz last() const {return sel(len-1);}
  constexpr Path next() const {
    #ifndef ARDUINO
      assert(len>0);
    #endif
    return {(Depth)(len-1),&data[1]};
  }
};

template<Depth depth> struct PathData {
  Sz data[depth]{0};
  Path focusAt(Depth at)  {assert(at<depth);return {at,data};}
  Sz operator[](Depth i) const {assert(i<depth);return data[(int)i];}
  operator Path() {return Path{depth,data};}
};

struct Ctx {
  Path path{};//full path
  NavMode mode{NavMode::Nav};
  Depth pAt{0};//print level mark
  bool enabled{true};//collected from target item
  Sz* tops{nullptr};//given by nav (nav+output specialized)
  //--------
  Depth at{0};//depth level counter
  Sz prev{0};
  bool pad{false};//pad printing?
  Sz idx{0};
  Sz pIdx{0};

  Ctx(
    Path path,
    NavMode mode={NavMode::Nav},
    Depth pAt={0},
    bool enabled={true},
    Sz* tops={nullptr},
    Depth at={0},
    Sz prev={0},
    bool pad={false},
    Sz idx={0},
    Sz pIdx={-1}
  ):path{path},mode{mode},pAt{pAt},enabled{enabled},tops{tops},at{at},prev{prev},pad{pad},idx{idx},pIdx{pIdx}{}

  constexpr bool psel() const {return sel(pAt)==pIdx;}// <=> parent is selected?
  constexpr Depth after() const {return path.len-pAt;}// <=> depth after print root, 1=>menu nav, 2=>pad menu nav, 3=>pad menu edit
  constexpr Sz sel() const {return path.sel(std::min((Depth)at,(Depth)(path.len-1)));}
  constexpr Sz sel(Depth i) const {assert(i<path.len);return path.sel(i);}
  constexpr Sz top() const {return tops[(int)at];}
  constexpr operator bool() const {return path.sel(at>0?at-1:0)==idx;}
  constexpr bool padPrinting() const {return at-pAt>0;}
  Sz top(Sz i) {return tops[(int)at]=i;}
  Ctx next() const {assert(at+1<path.len);return Ctx{path,mode,pAt,enabled,tops,(Depth)(at+1),0,pad,0};}
};

#ifdef MENU_DEBUG
  template<typename Out> Out& operator<<(Out& out,const Pos& o) {return out<<"{"<<o.x<<","<<o.y<<"}";}

  template<typename Out>
  Out& operator<<(Out& out,const Path o) {
    out<<"{";
    for(Sz i=0;i<o.len;i++) {
      if(i) out<<",";
      out<<o.data[i];
    }
    return out<<"}";
  }

  template<typename Out>
  Out& operator<<(Out& out,const Ctx& o) {
  return out
    <<"#"<<o.idx
    <<" path:"<<o.path
    <<" mode:"<<o.mode
    <<" pAt:"<<o.pAt
    <<" at:"<<o.at
    <<" pIdx:"<<o.pIdx
    <<" en:"<<o.enabled
    // <<" tops:"<<o.tops
    <<" prev:"<<o.prev
    <<" pad:"<<o.pad;
  }
#endif

// rule build ------------------------------------------

//rules system, requires/excludes simple class (will use std::same_as<>) from the derivation chain
template<typename O> struct Class {
  template<typename Head,typename Base> using Requires=std::bool_constant<std::is_same<O,Head>::value||Base::template Requires<Class<O>>::value>;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

//rule predicates------------------------------------------
struct IsCursor {
  template<typename Head,typename Base> using Requires=typename Base::IsCursor;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct RawDevice {
  template<typename Head,typename Base> using Requires=typename Base::RawDevice;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsFormat {
  template<typename Head,typename Base> using Requires=typename Base::IsFormat;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsPrinter {
  template<typename Head,typename Base> using Requires=typename Base::IsPrinter;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsDataParser {
  template<typename Head,typename Base> using Requires=typename Base::IsDataParser;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsParser {
  template<typename Head,typename Base> using Requires=typename Base::IsParser;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsArea {
  template<typename Head,typename Base> using Requires=typename Base::IsArea;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

struct IsBuffer {
  template<typename Head,typename Base> using Requires=typename Base::IsBuffer;
  template<typename Head,typename Base> using Excludes=std::bool_constant<!Requires<Head,Base>::value>;
};

//debug ---
#include "menu/sys/debug.h"


