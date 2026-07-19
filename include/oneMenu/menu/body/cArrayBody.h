#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  template<typename T, T data[], Sz _sz>
  struct CArrayBody {
    // T::depth(), not a hardcoded 1 — matches StaticBody's own convention
    // (report the contained items' own depth, uninflated; the enclosing
    // Menu::Part::depth() is what adds +1 for the level CArrayBody itself
    // sits at). A hardcoded 1 silently under-sizes TreeNav's own
    // PathData<depth()+1> buffer whenever T has any internal nesting of its
    // own (e.g. a per-slot item that itself opens a submenu) — caught via a
    // real `assert(at<depth)` failure inside PathData::focusAt, not assumed.
    static constexpr Depth depth() {return T::depth();}
    static constexpr Sz size() noexcept {return _sz;}
    static constexpr Sz size(Sz i) {assert(i<_sz);return data[i].size();}

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(data[i],ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i].printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return data[i].printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return data[i].printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {return data[i].template nav<isKbd>(n,cke,path);}

    // required by EventDispatch's own recursive descent (nav.h detail::
    // eventVisit calls body.visit(idx,fn) uniformly on whatever body type
    // it reaches) — without this, any CArrayBody reachable from a
    // NAVROOT/EventDispatch-enabled nav tree fails to compile the moment
    // EventDispatch is composed at all, not just when an event actually
    // fires through it.
    template<typename Fn>
    auto visit(Sz i,Fn&& fn) {return fn(data[i]);}
  };

  template<typename T, T* data[], Sz _sz>
  struct CPtrArrayBody {
    // Deliberately NOT T::depth() (unlike CArrayBody above) — every slot
    // here is a genuinely different concrete type behind a common T*
    // (that's the whole point of the pointer form), so T's own depth()
    // (when T even has one — e.g. the abstract IItem itself doesn't) tells
    // us nothing real about what's actually stored. A same-type CArrayBody
    // can honestly report T::depth() because every slot really is T; a
    // CPtrArrayBody can't without either a runtime walk (not viable —
    // depth() must stay constexpr, sizes a compile-time PathData<> buffer)
    // or an explicit per-instantiation depth NTTP (a real design surface,
    // not worth building speculatively — see notes.md's own account of a
    // declared-depth()-NTTP variant built then reverted as unused). Assumed
    // 1 instead: every real caller today (neurMenu's iBody/sBody) only ever
    // holds leaf items, so this is accurate, not just convenient — if a
    // real caller needs deeper nesting through this exact type later, that
    // NTTP is the fix to bring back then, not now.
    static constexpr Depth depth() {return 1;}
    static constexpr Sz size() noexcept {return _sz;}
    static constexpr Sz size(Sz i) {assert(i<_sz);return data[i]->size();}

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      for(Sz i=0;i<_sz&&out.free().y;i++) out.printItem(*data[i],ctx);
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return data[i]->printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return data[i]->printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return data[i]->printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {return data[i]->template nav<isKbd>(n,cke,path);}

    // see CArrayBody's own visit() doc comment above — same requirement.
    template<typename Fn>
    auto visit(Sz i,Fn&& fn) {return fn(*data[i]);}
  };

};//namespace oneMenu
