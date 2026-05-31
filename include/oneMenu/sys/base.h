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

namespace oneMenu {
  using hapi::Chain;
  // using hapi::query;

  // using oneItem::ItemDef;

  using oneData::DefaultDataDef;
  using oneData::CText;
  using oneData::Default;
  using oneData::Watch;
  using oneData::NumRange;
  using oneData::StaticNumRange;

  using oneOutput::Pos;
  using oneOutput::Area;

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

  //rule predicates------------------------------------------
  //IsCursor predicate
  struct IsCursor {
    template<typename O,typename =void> struct Check {static constexpr const bool value{false};};
    template<typename O> struct Check<O,std::void_t<typename O::IsCursor>> {static constexpr const bool value{O::IsCursor::value};};
  };

  //RawDevice predicate
  struct RawDevice {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::RawDevice>> {
      static constexpr const bool value{O::RawDevice::value};
    };
  };

  //IsFormat predicate
  struct IsFormat {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsFormat>> {
      static constexpr const bool value{O::IsFormat::value};
    };
  };

  //IsPrinter predicate
  struct IsPrinter {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsPrinter>> {
      static constexpr const bool value{O::IsPrinter::value};
    };
  };

  //IsDataParser predicate
  struct IsDataParser {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsDataParser>> {
      static constexpr const bool value{O::IsDataParser::value};
    };
  };

  //IsParser predicate
  struct IsParser {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsParser>> {
      static constexpr const bool value{O::IsParser::value};
    };
  };

  //IsArea predicate
  struct IsArea {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsArea>> {
      static constexpr const bool value{O::IsArea::value};
    };
  };

  //IsBuffer predicate
  struct IsBuffer {
    template<typename O,typename =void> struct Check {
      static constexpr const bool value{false};
    };
    template<typename O> struct Check<O,std::void_t<typename O::IsBuffer>> {
      static constexpr const bool value{O::IsBuffer::value};
    };
  };
};

//debug ---
#include "oneMenu/sys/debug.h"
