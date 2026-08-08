#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// Joins two bodies into one logical body: indices [0, BodyA::size()) route to b1,
  /// [BodyA::size(), total) route to b2. Any body type; nest to join more than two.
  template<typename BodyA, typename BodyB>
  struct JoinBody {
    BodyA b1;
    BodyB b2;

    static constexpr Depth depth() {return staticMax<BodyA::depth(),BodyB::depth()>();}

    Sz size() const {return b1.size()+b2.size();}

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
