#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief Joins two bodies into one logical body.
  /// Indices [0, B1::size()) route to b1; [B1::size(), total) route to b2.
  /// Both bodies may be any body type (StaticBody, CArrayBody, StdBody, …).
  /// Nest to join more than two: JoinBody<A, JoinBody<B, C>>.
  template<typename B1, typename B2>
  struct JoinBody {
    B1 b1;
    B2 b2;

    static constexpr Depth depth() {return staticMax<B1::depth(),B2::depth()>();}

    Sz size() const {return b1.size()+b2.size();}

    bool changed() const {return b1.changed()||b2.changed();}
    void sync() {b1.sync();b2.sync();}

    template<typename Out>
    void print(Out& out) const noexcept {b1.print(out);b2.print(out);}

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

  template<typename B1,typename B2>
  JoinBody<std::decay_t<B1>,std::decay_t<B2>> joinBody(B1&& b1,B2&& b2)
    {return {std::forward<B1>(b1),std::forward<B2>(b2)};}

};//namespace oneMenu
