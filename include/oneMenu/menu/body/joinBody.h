#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief Joins two bodies into one logical body.
  /// Indices [0, BodyA::size()) route to b1; [BodyA::size(), total) route to b2.
  /// Both bodies may be any body type (StaticBody, CArrayBody, StdBody, …).
  /// Nest to join more than two: JoinBody<A, JoinBody<B, C>>.
  /// NOTE: template parameters are named BodyA/BodyB, not B1/B2 — Arduino's
  /// own binary.h (`#define B1 1`, `#define B2 2`, ..., pulled in
  /// transitively by <Arduino.h> under any framework=arduino build) would
  /// otherwise macro-substitute B1/B2 in this file's own template<>
  /// declarations before the compiler ever sees them as identifiers,
  /// breaking the template entirely — confirmed empirically (a real
  /// avr-g++ build under framework=arduino that includes both <Arduino.h>
  /// and this header, e.g. via U8g2lib.h, fails with "expected
  /// nested-name-specifier before numeric constant").
  template<typename BodyA, typename BodyB>
  struct JoinBody {
    BodyA b1;
    BodyB b2;

    static constexpr Depth depth() {return staticMax<BodyA::depth(),BodyB::depth()>();}

    Sz size() const {return b1.size()+b2.size();}

    bool changed() const {return b1.changed()||b2.changed();}
    void sync() {b1.sync();b2.sync();}

    template<typename Out>
    bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      bool r=b1.printBody(out,ctx,bidx);
      return b2.printBody(out,ctx,bidx+b1.size())||r;
    }

    template<typename Out>
    bool printMenu(Out& out,Ctx& ctx,Sz i) {
      Sz s=b1.size();
      return i<s?b1.printMenu(out,ctx,i):b2.printMenu(out,ctx,i-s);
    }

    template<typename Out>
    bool printItem(Out& out,Ctx& ctx,Sz i) {
      Sz s=b1.size();
      return i<s?b1.printItem(out,ctx,i):b2.printItem(out,ctx,i-s);
    }

    template<typename Out>
    bool printItem(Out& out,Sz i=0) {
      Sz s=b1.size();
      return i<s?b1.printItem(out,i):b2.printItem(out,i-s);
    }

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i) {
      Sz s=b1.size();
      return i<s?b1.template nav<isKbd>(n,cke,path,i)
                :b2.template nav<isKbd>(n,cke,path,i-s);
    }

    template<typename Nav,typename P>
    bool setStr(Nav& n,const char* str,P p,Sz i) {
      Sz s=b1.size();
      return i<s?b1.setStr(n,str,p,i):b2.setStr(n,str,p,i-s);
    }

    template<typename Fn>
    auto visit(Sz i,Fn&& fn) {
      Sz s=b1.size();
      if(i<s) return b1.visit(i,std::forward<Fn>(fn));
      return b2.visit(i-s,std::forward<Fn>(fn));
    }
  };

  template<typename BodyA,typename BodyB>
  JoinBody<std::decay_t<BodyA>,std::decay_t<BodyB>> joinBody(BodyA&& b1,BodyB&& b2)
    {return {std::forward<BodyA>(b1),std::forward<BodyB>(b2)};}

};//namespace oneMenu
