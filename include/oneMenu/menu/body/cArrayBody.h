#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  template<typename T, T data[], Sz _sz>
  struct CArrayBody {
    static constexpr Depth depth() {return 1;}
    static constexpr Sz size() noexcept {return _sz;}
    static constexpr Sz size(Sz i) {assert(i<_sz);return data[i].size();}

    bool changed() const {
      bool c{false};
      for(Sz i=0;i<_sz;i++) c=c||data[i].changed();
      return c;
    }
    void sync() {for(Sz i=0;i<_sz;i++) data[i].sync();}

    template<typename Out>
    void print(Out& out) const noexcept {
      for(Sz i=0;i<_sz;i++) data[i].print(out);
    }

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(data[i],ctx);
      return false;
    }
    template<typename Out> static bool printHiddenBody(Out&,Ctx&) noexcept {return false;}
    template<typename Out> static bool printHiddenMenu(Out&,Ctx&,Sz=0) noexcept {return false;}

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i].printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return data[i].printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return data[i].printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {return data[i].template nav<isKbd>(n,cke,path);}
  };

  template<typename T, T* data[], Sz _sz>
  struct CPtrArrayBody {
    static constexpr Depth depth() {return 1;}
    static constexpr Sz size() noexcept {return _sz;}
    static constexpr Sz size(Sz i) {assert(i<_sz);return data[i]->size();}

    bool changed() const {
      bool c{false};
      for(Sz i=0;i<_sz;i++) c=c||data[i]->changed();
      return c;
    }
    void sync() {for(Sz i=0;i<_sz;i++) data[i]->sync();}

    template<typename Out>
    void print(Out& out) const noexcept {
      for(Sz i=0;i<_sz;i++) data[i]->print(out);
    }

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(*data[i],ctx);
      return false;
    }
    template<typename Out> static bool printHiddenBody(Out&,Ctx&) noexcept {return false;}
    template<typename Out> static bool printHiddenMenu(Out&,Ctx&,Sz=0) noexcept {return false;}

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i]->printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return data[i]->printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return data[i]->printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {return data[i]->template nav<isKbd>(n,cke,path);}
  };

};//namespace oneMenu
