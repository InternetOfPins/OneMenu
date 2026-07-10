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

  /// @brief input interface base for runtime-polymorphic input dispatch (mirrors out.h's IOut)
  struct IIn {
    virtual bool available()=0;
    virtual CKE  cmd()=0;
  };

  /// @brief InDef variant with virtual dispatch for runtime-polymorphic input
  template<typename... KK>
  struct IInDef:IIn,InDef<KK...> {
    using Base=InDef<KK...>;
    virtual bool available() override {return Base::available();}
    virtual CKE  cmd()       override {return Base::cmd();}
  };

  // Runtime *list* of independent IIn* sources, polled in sequence, first with an event
  // wins. Distinct from the compat layer's InGroup (am4.h), which polls every member
  // unconditionally every call ("poll everything") — this stops at the first source that
  // actually has something pending, same one-event-per-poll contract InDef's own compiled
  // chain gives (whichever component is listed first gets first refusal there; here,
  // whichever IIn* was add()ed first gets first refusal). Meant for a device set only
  // known/changeable at runtime (hot-swappable input, or heterogeneous devices picked at
  // startup) — when the set is fixed at compile time, prefer plain InDef<KK...> composition
  // (zero vtables) or InGroup (compat layer) instead.
  //
  // Fixed-capacity array, not a dynamic container — no heap, no std:: dependency (AVR has
  // no libstdc++). Capacity N is a compile-time bound; add() silently ignores sources past
  // N (mirrors AVR-wide "no exceptions" convention elsewhere in IOP).
  /// @brief runtime list of IIn* sources; inBurst()/doInput()/poll() mirror InDef's own surface
  template<Sz N>
  struct InList {
    InList() = default;
    // Compile-time-count construction — for a device set fixed at the call site
    // (AM4-compat's MENU_INPUTS(in,dev1,dev2,...), am4.h), not the runtime-growable
    // case add() targets. Each Ins* must already derive from IIn (e.g. IInDef<...>,
    // not plain InDef<...> — virtual dispatch accepted deliberately here).
    template<typename... Ins>
    InList(Ins*... ins) : m_items{static_cast<IIn*>(ins)...}, m_count((Sz)sizeof...(Ins)) {
      static_assert(sizeof...(Ins)<=N, "InList: more devices than capacity N");
    }
    Sz add(IIn& in) {
      Sz i=m_count;
      if(i<N) m_items[m_count++]=&in;
      return i;
    }
    bool available() const {
      for(Sz i=0;i<m_count;i++) if(m_items[i]->available()) return true;
      return false;
    }
    CKE cmd() {
      for(Sz i=0;i<m_count;i++) {
        if(m_items[i]->available()) {
          CKE r=m_items[i]->cmd();
          if(r.cmd!=Cmd::None) return r;
        }
      }
      return {};
    }

    // Same shape/semantics as InDef::inBurst/doInput/poll (see their comments above) —
    // InList is a drop-in "in" object wherever InDef<...> is used today.
    template<typename Nav>
    Sz inBurst(Nav& nav, Sz maxCount=8) {
      Sz count=0;
      while(count<maxCount && available()) {
        if(nav.in(*this)) count++;
      }
      return count;
    }
    template<typename Nav>
    bool doInput(Nav& nav, Sz maxCount=8) { return inBurst(nav,maxCount)>0; }
    template<typename Nav,typename Out>
    bool poll(Nav& nav, Out& out, Sz maxCount=8) {
      bool i=doInput(nav,maxCount);
      bool o=nav.doOutput(out);
      return i||o;
    }
  protected:
    // N?N:1 — see OutList's identical comment (out.h): a plain IIn*[N] is
    // ill-formed for N=0, and N==0 (every device elided as NONE) is legitimate.
    IIn* m_items[N?N:1]{};
    Sz m_count{0};
  };
  template<typename... Ins> InList(Ins*...) -> InList<(Sz)sizeof...(Ins)>;

  // Compile-time-fixed pack of independent input devices — the input-side
  // twin of OutGroup (out.h; see its own comment for the full rationale —
  // this is symmetric with it, not driven by input having the same
  // type-erasure problem output does, since IIn's available()/cmd() aren't
  // templated on anything). "Poll everything" (unconditional, not
  // short-circuited by ||), unlike InList's "first source wins" — matches
  // AM4's own MENU_INPUTS semantics (every listed device gets polled every
  // tick) and a compile-time InDef<KK...> chain's own behavior. Zero vtable
  // cost: devices can be plain InDef<...>, no IInDef<...> needed.
  template<typename... Ins> struct InGroup {
    template<typename Nav> bool doInput(Nav&, Sz = 8) { return false; }
  };
  template<typename I, typename... Ins>
  struct InGroup<I, Ins...> : InGroup<Ins...> {
    I* p;
    InGroup(I* p_, Ins*... rest) : InGroup<Ins...>(rest...), p(p_) {}
    template<typename Nav>
    bool doInput(Nav& nav, Sz maxCount = 8) {
      bool a = p->doInput(nav, maxCount);
      bool b = InGroup<Ins...>::doInput(nav, maxCount);
      return a || b;
    }
  };
  template<typename... Ins> InGroup(Ins*...) -> InGroup<Ins...>;

};//namespace oneMenu
