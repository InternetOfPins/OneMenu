#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  template<typename T>
  struct StdBody:T {
    static constexpr Depth depth() {return 1;}
    Sz size() const {return T::size();}
    Sz size(Sz i) const {return T::operator[](i)->size();}

    bool changed() const {
      bool c{false};
      for(Sz i=0;i<T::size();i++) c=c||T::operator[](i)->changed();
      return c;
    }

    void sync() {
      for(Sz i=0;i<T::size();i++) T::operator[](i)->sync();
    }

    template<typename Out>
    void print(Out&) const noexcept {}

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      for(Sz i=0;i<T::size()&&out.free().y;i++) out.printItem(*T::operator[](i),ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return T::operator[](i)->printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return i<T::size()&&T::operator[](i)->printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return i<T::size()&&T::operator[](i)->printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {return i<T::size()&&T::operator[](i)->template nav<isKbd>(n,cke,path);}
  };

};//namespace oneMenu
