#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  // Fixed-size, same-type array body windowed over a backing store whose
  // true size may not be known at compile time (or may not fit in RAM at
  // all — a directory listing on an SD card, a database table, anything
  // where the item count is discovered, not declared). Same "cache page"
  // role CArrayBody<T,data,sz> already fills for a *fixed*, fully-in-RAM
  // array — CacheBody reuses that exact idea (a stable-address array whose
  // *contents* get mutated, not its identity) but adds the recenter-on-miss
  // windowing AM4's own real `plugin/SDMenu.h`'s `CachedFSO<SDC,maxSz>`
  // already proved: `cache[maxSz]` (fixed window), `cacheStart` (window
  // offset), `size` (true total count, from a full backing-store scan),
  // `refresh(start)` (rescans the *whole* backing store — updating the true
  // count — but only repopulates the `[start,start+maxSz)` slice), and a
  // recenter (`refresh(idx-maxSz/2)`) whenever an out-of-window index is
  // requested. Ported here backing-store-agnostic (not SD-specific): any
  // `Backing` exposing `count()`/`populate(idx,slot)` works — a real SD
  // directory, a database cursor, or (for testing) a plain in-memory array.
  //
  // `..`-navigation: compose `JoinBody<BackItemBody, CacheBody<...>>`
  // (joinBody.h, already exists) rather than building parent-navigation
  // into CacheBody itself — `JoinBody` already does exactly the index-
  // concatenation this needs (a fixed 1-item "back" body in front of the
  // windowed cache body), it just doesn't (and shouldn't) know anything
  // about what "back" means semantically — that's an ordinary item's own
  // `nav()`, same as any other button/action item.
  //
  // Backing contract:
  //   static Sz count();                     // true total size (may scan)
  //   static void populate(Sz idx, T& slot);  // fill slot with backing-store item at idx
  template<typename T, T data[], Sz MaxSz, typename Backing>
  struct CacheBody {
    inline static Sz cacheStart = 0;
    // Real trap, caught by a native test before this was ever wired to a
    // real backing store: cacheStart's own default-zero-initialization is
    // indistinguishable from "window [0,MaxSz) genuinely already loaded" —
    // ensure(0) would silently skip the very first refresh() entirely,
    // reading uninitialized/stale slot contents. _loaded forces the first
    // ensure() call (whatever index it's for) to always refresh.
    inline static bool _loaded = false;

    static constexpr Depth depth() {return T::depth();}
    static Sz size() noexcept {return Backing::count();}
    static Sz size(Sz i) {ensure(i); return data[i-cacheStart].size();}

    // Rescans the whole backing store (Backing::count(), inside populate's
    // own real implementation for a real SD directory — matching AM4's own
    // CachedFSO::refresh, which also does a full rewind+rescan every time)
    // but only repopulates the visible [start,start+MaxSz) slice.
    static void refresh(Sz start) {
      cacheStart = start;
      _loaded = true;
      for (Sz k = 0; k < MaxSz; k++) Backing::populate(start + k, data[k]);
    }

    // Recenter-on-miss — same algorithm as AM4's real entry(idx): if the
    // requested absolute index falls outside the current window (or
    // nothing has been loaded yet at all), refresh around it (centered,
    // clamped to 0).
    static void ensure(Sz i) {
      if (!_loaded || i < cacheStart || i >= cacheStart + MaxSz)
        refresh(i > MaxSz/2 ? Sz(i - MaxSz/2) : Sz(0));
    }

    template<typename Out> bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      Sz n = size();
      for (Sz i=bidx; i<n && out.free().y; i++) {
        ensure(i);
        out.printItem(data[i-cacheStart], ctx);
      }
      return false;
    }

    template<typename Out> bool printMenu(Out& out,Ctx& ctx,Sz i)
      {ensure(i); return data[i-cacheStart].printMenu(out,ctx);}

    template<typename Out> bool printItem(Out& out,Ctx& ctx,Sz i)
      {ensure(i); return data[i-cacheStart].printItem(out,ctx);}
    template<typename Out> bool printItem(Out& out,Sz i=0)
      {ensure(i); return data[i-cacheStart].printItem(out);}

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,Path path,Sz i)
      {ensure(i); return data[i-cacheStart].template nav<isKbd>(n,cke,path);}

    // required by EventDispatch's own recursive descent — same requirement
    // as CArrayBody's own visit() (cArrayBody.h).
    template<typename Fn>
    auto visit(Sz i,Fn&& fn) {ensure(i); return fn(data[i-cacheStart]);}
  };

};//namespace oneMenu
