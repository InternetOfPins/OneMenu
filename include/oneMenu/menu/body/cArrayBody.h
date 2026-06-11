#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {
  template<typename T,T data[],Sz _sz>
  struct CArrayBody {
    template<typename> static constexpr const bool has{false};
    static constexpr Depth depth() {return 1;}
    static constexpr const Sz size() {return _sz;}
    static constexpr const Sz size(Sz i) {assert(i<_sz);return data[i];}

    bool changed() {
      bool c{false};
      for(Sz i=0;i<_sz;i++) c=c||data[i].changed();
      return c;
    }

    template<bool isKbd,typename Nav> 
    bool nav(Nav& n,const CKE& cke,Path path,Sz i) 
      {return data[i].template nav<isKbd>(n,cke,path);}

    template<typename Out> bool printBody(Out& out,Ctx& ctx) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(data[ctx.idx],ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i].printMenu(out,ctx);}


  // //Id, this is compile-time search/reference, but NOT here, all items are equal here --
  //   template<int> using HasId=std::integral_constant<bool,false>;
  //   template<int> using WithId=std::integral_constant<bool,false>;
  };

  template<typename T,T* data[],Sz _sz>
  struct CPtrArrayBody {
    template<typename> static constexpr const bool has{false};
    static constexpr Depth depth() {return 1;}
    static constexpr const Sz size() {return _sz;}
    static constexpr const Sz size(Sz i) {assert(i<_sz);return data[i]->size();}

    bool changed() {
      bool c{false};
      for(Sz i=0;i<_sz;i++) c=c||data[i]->changed();
      return c;
    }

    template<bool isKbd,typename Nav> 
    bool nav(Nav& n,const CKE& cke,Path path,Sz i) 
      {return data[i]->template nav<isKbd>(n,cke,path);}

    template<typename Out> bool printBody(Out& out,Ctx& ctx) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(*data[ctx.idx],ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i]->printMenu(out,ctx);}


  // //Id, this is compile-time search/reference, but NOT here, all items are equal here --
  //   template<int> using HasId=std::integral_constant<bool,false>;
  //   template<int> using WithId=std::integral_constant<bool,false>;
  };
};//namespace oneMenu 