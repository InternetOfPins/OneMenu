/**
 * @file in.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief input API
 * @version 5
 * @date 2026-04-28
 *
 * @copyright Copyright (c) 2026
 *
*/
#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {
  // Input chain terminal.
  // cmd()      — poll for a command event; returns CKE{Cmd::None} = no event.
  // parseKey() — translate a raw key character; terminal silently drops it.
  // available()— true if the component has a pending event ready to deliver.
  template<typename K>
  struct InAPI : K {
    using Base = K;
    static constexpr bool available()       { return false; }
    static constexpr CKE  cmd()             { return {}; }
    static constexpr CKE  parseKey(Key)     { return {}; }
  };

  template<typename... KK>
  struct InDef : APIOf<InAPI<Nil>, KK...> {
    using Base = APIOf<InAPI<Nil>, KK...>;

    // Drains up to maxCount pending events in one call instead of one per poll — a fast
    // encoder spin or held button otherwise trickles out at one event per ~30ms tick.
    // maxCount bounds it (AM4's inputBurst counter — no timer needed, just a counter) so a
    // stuck/noisy input source can't starve the redraw. Returns how many were processed;
    // callers doing partial/differential redraw may want to treat >1 like a bigger change
    // (see LockMode::None "scroll => full redraw" precedent in ScrollPrinter::printMenu).
    //
    // NOTE: loop condition is Base::available(), NOT nav.in(*this)'s return value. A single
    // in() call can legitimately return false while there's still more to read — e.g. PCKbd's
    // escape-sequence parser (pcKbdIn.h) absorbs ESC and '[' across two separate in() calls
    // before the third byte finally yields a real CKE. available() reflects "more raw input
    // queued" independent of whether the current byte completed a command.
    //
    // Lives here (on the fully-assembled InDef), not inside one of the composed K::Part<N>
    // layers, and takes nav by reference rather than being a Nav-side method: both `nav` and
    // `*this` are references to their complete, concrete chain types at this call site, so
    // nav.in(*this) resolves via ordinary inheritance lookup to the most-derived overrides on
    // both sides (e.g. IndexGo::Part::in() on the nav side) with no CRTP self-cast needed —
    // unlike a method placed inside an intermediate Part, which only ever sees its own scope
    // and bases, never a component composed above it (see nav.h's TreeNav::Part::doCmd()).
    template<typename Nav>
    Sz inBurst(Nav& nav, Sz maxCount=8) {
      Sz count=0;
      while(count<maxCount && Base::available()) {
        if(nav.in(*this)) count++;
      }
      return count;
    }

    // AM4's poll(){doInput();doOutput();}: doInput() is inBurst() narrowed to a bool (was any
    // event processed at all — callers wanting the actual count still call inBurst directly,
    // e.g. to decide burst size mattered). poll() fuses both steps for the common case; callers
    // needing to act between them (e.g. gate a secondary output device on Nav::changed(), read
    // before doOutput()'s sync() clears it) should call doInput()/Nav::doOutput() separately —
    // that's why both stay individually callable instead of only existing fused.
    template<typename Nav>
    bool doInput(Nav& nav, Sz maxCount=8) { return inBurst(nav,maxCount)>0; }

    template<typename Nav,typename Out>
    bool poll(Nav& nav, Out& out, Sz maxCount=8) {
      bool i=doInput(nav,maxCount);
      bool o=nav.doOutput(out);
      return i||o;
    }
  };

};//namespace oneMenu
