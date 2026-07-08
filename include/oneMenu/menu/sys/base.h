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

#ifdef __AVR__
  #include <assert.h>
#else
  // No <iostream> here — nothing in base.h uses it (only debug.h's
  // MENU_DEBUG-gated std::cerr does, and it includes it itself now;
  // streamOut.h's ConsoleOut also self-includes it). Including it
  // unconditionally here used to force libstdc++'s iostream/locale
  // static-init machinery into every non-AVR build regardless of platform
  // or actual usage — real cost on embedded non-AVR targets (ESP32/STM32
  // under Arduino framework), see OneOutput's oneOutput.h for the other
  // (bigger) half of this same fix and OneMenu/.RnD/mem.md for the measured
  // effect.
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
#include <oneChip/clock.h>

#include <hapi/hapi.h>
#include <oneOutput/oneOutput.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>

#include "oneMenu/menu/sys/enums.h"

using hapi::Nil;
using hapi::Chain;

namespace oneMenu {

  using oneData::CText;
  using oneData::Default;
  using oneData::Watch;
  using oneData::NumRange;
  using oneData::StaticNumRange;

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

  struct IItem;
  struct IOut;
  struct INav;

  struct XY{Sz x;Sz y;};
  using Pos=XY;
  using Area=XY;
  
  struct CKE {
    Cmd  cmd = Cmd::None;
    Key  key = 0;
    bool ext = false;
    bool kbd = false;  // true for raw keyboard key events (was isKbd template param)
  };

  template<typename Cor> struct Colors{Cor fg;Cor bg;};

  struct Path {
    Depth len;
    Sz* data;
    constexpr Sz sel(Depth i=0) const {return len>i?data[(int)i]:0;}
    constexpr Sz last() const {return sel(len-1);}
    constexpr Path next() const {
      #if !defined(__AVR__)
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

    Ctx():path{0,nullptr},mode{NavMode::Nav},pAt{0},enabled{true},tops{nullptr},at{0},prev{0},pad{false},idx{-1},pIdx{-1}{}

    constexpr bool psel() const {return sel(pAt)==pIdx;}// <=> parent is selected?
    constexpr Depth after() const {return path.len-pAt;}// <=> depth after print root, 1=>menu nav, 2=>pad menu nav, 3=>pad menu edit
    constexpr Sz sel() const {return path.len>0?path.sel(std::min((Depth)at,(Depth)(path.len-1))):(Sz)-1;}
    constexpr Sz sel(Depth i) const {assert(i<path.len);return path.sel(i);}
    constexpr Sz top() const {return tops[(int)at];}
    constexpr operator bool() const {return path.len>0&&path.sel(at>0?at-1:0)==idx;}
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
      <<" prev:"<<o.prev
      <<" pad:"<<o.pad;
    }
  #endif

  // tag structs — inherit from these on the outer component struct to declare membership
  struct aCursor    {};
  struct aFormat    {};
  struct aPrinter   {};
  struct aParser    {};
  struct anArea     {};
  struct aBuffer    {};
  struct aDataParser{};
  struct aRawDevice {};
  struct aScrollBody{};
  struct aFillRect  {};

  // predicate aliases — use with hapi::query<>, Requires<>, Excludes<>
  using IsCursor     = hapi::TagIs<aCursor>;
  using IsScrollBody = hapi::TagIs<aScrollBody>;
  using IsFormat     = hapi::TagIs<aFormat>;
  using IsPrinter    = hapi::TagIs<aPrinter>;
  using IsParser     = hapi::TagIs<aParser>;
  using IsArea       = hapi::TagIs<anArea>;
  using IsBuffer     = hapi::TagIs<aBuffer>;
  using IsDataParser = hapi::TagIs<aDataParser>;
  using RawDevice    = hapi::TagIs<aRawDevice>;
  using IsFillRect   = hapi::TagIs<aFillRect>;

  using hapi::Requires;
  using hapi::Excludes;
};

//debug ---
#include "oneMenu/menu/sys/debug.h"
