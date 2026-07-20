#pragma once

#include "oneMenu/menu/body/cacheBody.h"
#include "oneMenu/menu/body/cArrayBody.h"
#include "oneMenu/menu/body/joinBody.h"

namespace oneMenu {

  // File-browser machinery, layered on top of the already-shipped, unmodified
  // CacheBody/CArrayBody/JoinBody — mirrors AM4's real plugin/SDMenu.h
  // (SDMenuT<FS>+CachedFSO<SDC,maxSz>) but composed from existing OneMenu
  // pieces instead of a single hand-rolled class. See the port's own plan
  // for the real-source citations this design is grounded in.
  //
  // Backing contract (a superset of CacheBody's own count()/populate()):
  //   static Sz count();
  //   static void populate(Sz idx, T& slot);   // dir entries get a trailing '/'
  //   static bool atRoot();
  //   static void descend(const char* name);   // enter a "name/"-suffixed subfolder
  //   static void up();                        // pop one folder level
  //   static const char* currentFolder();

  // Shared pop-a-folder-or-close-the-level branch — used by both BackNav's
  // Enter handler and FolderEsc's real-Esc handler, so this decision exists
  // in exactly one place (AM4's own escCmd does both jobs too, see SDMenu.h).
  // Cache::refresh(0) forces an immediate rescan of the parent folder —
  // without it, CacheBody's own lazy ensure() sees a still-"in window,
  // already loaded" cache and never notices the backing folder changed
  // underneath it (caught by the native test, not assumed).
  template<typename Backing,typename Cache,typename Nav>
  bool folderPopOrClose(Nav& n) {
    if(Backing::atRoot()) return n.close();
    Backing::up();
    Cache::refresh(0);
    n.go(0);
    return true;
  }

  // ItemDef-composable "[..]" behavior — same Part<I>:I shape as Action<fn>
  // (item.h). Compose as ItemDef<BackNav<Backing,Cache>, Text>{"[..]"}.
  template<typename Backing,typename Cache>
  struct BackNav {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,Path path) {
        if(cke.cmd==Cmd::Enter) return folderPopOrClose<Backing,Cache>(n);
        return Base::template nav<isKbd>(n,cke,path);
      }
    };
  };

  // Menu-chain component — same Part<I>:I shape as WrapNav/PadDraw (menu.h),
  // placed in the file-browser submenu's own OO... pack. Intercepts a real
  // Esc key *before* the default level-close (nav.h doCmd()'s own
  // `if(!r&&cmd==Cmd::Esc) close();` fallback): returns true (pop one
  // folder, stay at this level) unless already at root, in which case it
  // returns false and lets that fallback close the level normally — the
  // "preemptive, else fall through" behavior this was designed for.
  template<typename Backing,typename Cache>
  struct FolderEsc {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,Path p) {
        if(cke.cmd==Cmd::Esc && p.len==1 && !Backing::atRoot())
          return folderPopOrClose<Backing,Cache>(n);
        return Base::template nav<isKbd>(n,cke,p);
      }
    };
  };

  // fn(folder,filename) — called when a plain (non-directory) entry is
  // picked. Deliberately a function POINTER, not a reference: a reference-
  // typed NTTP forwarded through fileBrowserDef()'s own factory function
  // (a template parameter re-used as another template's NTTP argument, one
  // level of indirection removed from the real function name) fails to
  // compile on real avr-g++ 7.3 ("not a valid template argument ... must be
  // the name of a function with external linkage") even though native g++
  // and a *direct* (non-forwarded) reference-NTTP instantiation both accept
  // it fine — confirmed via an isolated minimal repro before assuming this
  // was the fix, not just the SD.h/FileEntryBody instantiation being large.
  // A pointer-typed NTTP survives the same forwarding path on real hardware.
  using PickFunc=void(*)(const char*,const char*);

  // Windowed, folder-aware entry listing — holds a CacheBody<T,data,MaxSz,
  // Backing> by composition and forwards everything except nav(), which it
  // intercepts on Cmd::Enter: a "name/"-suffixed entry descends into that
  // folder (stays at this nav level, same as AM4's own folderName mutation);
  // a plain file fires OnPick(currentFolder(),name) then runs the same
  // pop-or-close branch Esc uses — matching AM4's real "file pick also runs
  // escCmd" behavior (picking a file several folders deep pops exactly one
  // folder, only a full exit if you were already at root).
  template<typename T,T data[],Sz MaxSz,typename Backing,PickFunc OnPick>
  struct FileEntryBody {
    using Cache=CacheBody<T,data,MaxSz,Backing>;
    Cache cache;

    static constexpr Depth depth() {return Cache::depth();}
    Sz size() const {return Cache::size();}
    Sz size(Sz i) {return cache.size(i);}

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0)
      {return cache.printBody(out,ctx,bidx);}
    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {return cache.printMenu(out,ctx,i);}
    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {return cache.printItem(out,ctx,i);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {return cache.printItem(out,i);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i) {
      if(cke.cmd==Cmd::Enter) {
        Cache::ensure(i);
        const char* name=data[i-Cache::cacheStart].get();
        Sz len=0; while(name[len]) len++;
        if(len>0 && name[len-1]=='/') {
          Backing::descend(name);
          Cache::refresh(0);
          n.go(0);
          return true;
        }
        OnPick(Backing::currentFolder(),name);
        return folderPopOrClose<Backing,Cache>(n);
      }
      return cache.template nav<isKbd>(n,cke,path,i);
    }

    template<typename Fn>
    auto visit(Sz i,Fn&& fn) {return cache.visit(i,std::forward<Fn>(fn));}
  };

  // Factory sugar (matches menuDef<>()/padDef<>() precedent): builds
  // JoinBody<"[..]" back item, FileEntryBody<...>> wrapped in a Menu whose
  // own chain carries FolderEsc<Backing> for the real-Esc-key case.
  template<typename T,T data[],Sz MaxSz,typename Backing,PickFunc OnPick,typename Title>
  auto fileBrowserDef(Title&& title) {
    using Cache=CacheBody<T,data,MaxSz,Backing>;
    using BackItem=ItemDef<BackNav<Backing,Cache>,oneData::Text>;
    static BackItem backData[1]={BackItem{"[..]"}};
    using Back=CArrayBody<BackItem,backData,1>;
    using Entries=FileEntryBody<T,data,MaxSz,Backing,OnPick>;
    return menuDef<FolderEsc<Backing,Cache>>(std::forward<Title>(title),joinBody(Back{},Entries{}));
  }

};//namespace oneMenu
