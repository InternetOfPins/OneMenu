#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {
  template<typename T>
  struct StdBody:T {
    template<typename> static constexpr const bool has{false};
    static constexpr Depth depth() {return 1;}
    constexpr Sz size() const {return T::size();}
    constexpr Sz size(Sz i) const {return T::operator[](i).size();}

    bool changed() {
      bool c{false};
      for(auto i=T::begin();i<T::end();i++) c=c||(**i).changed();
      return c;
    }
    template<bool isKbd,typename Nav> 
    bool nav(Nav& n,const CKE& cke,Path path,Sz i) 
      {return i<T::size()&&T::operator[](i)->template nav<isKbd>(n,cke,path);}

    template<typename Out> bool printBody(Out& out,Ctx& ctx) {
      for(auto i=T::begin();i<T::end()&&out.freeY();i++) out.printItem(**i,ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return T::operator[](i)->printMenu(out,ctx);}

  // //Id, this is compile-time search/reference, but NOT here, all items are virtual here --
  //   template<int> using HasId=std::integral_constant<bool,false>;
  //   template<int> using WithId=std::integral_constant<bool,false>;
  };

};//namespace oneMenu