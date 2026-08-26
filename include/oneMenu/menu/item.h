/**
 * @file item.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief item API
 * @version 5
 * @date 2026-04-17
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

#include "oneData/oneData.h"
using oneData::Bool;
using oneData::CText;
#if defined(MENU_DEBUG_FULLSCREEN) || defined(MENU_DEBUG_ROWS)
#include <cstdio>
#endif

namespace oneMenu {

  template<typename Cfg=Nil>
  struct ItemAPI:oneItem::ItemAPI<Cfg> {
    using Base=oneItem::ItemAPI<Cfg>;
    constexpr ItemAPI() {}
    template<typename> using Requires=std::false_type;
    template<typename> using Excludes=std::true_type;
    static constexpr Depth depth() {return 1;}
    [[nodiscard]] static constexpr bool enabled() {return true;}
    static constexpr bool wraps() {return false;}
    static constexpr bool isPad() {return false;}
    static constexpr void enable(bool=true) {}
    [[nodiscard]] static constexpr bool changed() {return false;}
    static constexpr Pos pos() {return {0,0};}
    static constexpr void sync() {}
    template<typename Out> static constexpr void sync(Out&) {}
    static constexpr bool up() {return false;}
    static constexpr bool down() {return false;}
    template<typename Out> static constexpr bool printMenu(Out&,Ctx&) {return false;}
    template<typename Out> static constexpr bool printBody(Out&,Ctx&) {return false;}
    using Base::print;
    // printItem(Out&,Ctx&) is reached via ordinary inheritance from oneData::DataAPI's
    // generic no-op — no redeclaration or using-declaration needed here.
    template<bool isKbd,typename Nav> static constexpr bool nav(Nav& n,const CKE& cke,Path) {return false;}
    template<typename Nav,typename P> static constexpr bool setStr(Nav&,const char*,P) {return false;}
    /// @brief Semantic event hook (see enums.h EventMask); default no-op, override in a component that cares.
    static constexpr bool onEvent(EventMask) {return false;}
    /// @brief Per-frame animation hook, dispatched only to the currently-focused item (see TickFocus, nav.h); default no-op.
    static constexpr bool tick() {return false;}
  };

  template<typename... OO> struct ItemDef; // forward — ItemDefC's Ins/App alias it below

  /// @brief ItemDef's real logic, parameterized on Cfg; ItemDef<OO...> is Cfg=Nil, IItemDef is Cfg=hapi::CRTP<IItemDef<II...>>.
  template<typename Cfg,typename... OO>
  struct ItemDefC:APIOf<ItemAPI<Cfg>,OO...>{
    using Base=APIOf<ItemAPI<Cfg>,OO...>;
    using Base::Base;
    using Base::printMenu;
    using Base::enabled;
    using Base::print;

    template<bool isKbd,typename Nav>
    [[nodiscard]] bool nav(Nav& n,const CKE& cke,const Path p) {
      return enabled()?Base::template nav<isKbd>(n,cke,p):cke.cmd==Cmd::Enter;
    }

    template<typename Nav,typename P>
    bool setStr(Nav& n,const char* s,P p) {
      return enabled()?Base::template setStr(n,s,p):false;
    }

    template<typename Out> bool printItem(Out& out,Ctx& ctx) {
      Base::printItem(out,ctx);
      return Base::changed();
    }
    template<typename Out> bool printItem(Out& out,Ctx&& ctx={{}})
      {return printItem(out,static_cast<Ctx&>(ctx));}

    // find<Q>()/find(Q) are NOT declared here: for ItemDef<Menu<...>> they must be
    // inherited unshadowed from Menu::Part (own chain or body search); a plain ItemDef
    // (no nested Menu) has nothing to search and isn't a valid find<> receiver.

    template<typename... XX> using Ins=oneMenu::ItemDef<XX...,OO...>;
    template<typename... XX> using App=oneMenu::ItemDef<OO...,XX...>;

  };

  template<typename... OO>
  struct ItemDef:ItemDefC<Nil,OO...>{
    using Base=ItemDefC<Nil,OO...>;
    using Base::Base;
  };

  /// @brief Virtual-dispatch item interface, the item-side twin of out.h's IOut and nav.h's INav; opt-in escape hatch, not the zero-cost default (plain ItemDef).
  struct IItem {
    virtual bool printMenu(IOut& out,Ctx& ctx)=0;
    virtual bool printBody(IOut& out,Ctx&)=0;

    [[nodiscard]] virtual bool enabled() const=0;
    virtual void enable(bool=true)=0;
    [[nodiscard]] virtual bool changed() const=0;
    virtual void sync()=0;
    virtual void sync(IOut& out)=0;
    [[nodiscard]] virtual bool up() const=0;
    [[nodiscard]] virtual bool down() const=0;
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p)=0;
    virtual bool _nav(INav& n,const CKE& cke,const Path p)=0;

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path& path) {
      return isKbd?_kbdNav(n,cke,path):_nav(n,cke,path);
    }

    virtual void printItem(IOut& out,Ctx& ctx) {}

    /// @brief No-op default; overridden by IItemDef to forward into the real OO... chain's onEvent().
    virtual bool onEvent(EventMask) {return false;}

    /// @brief Nav-carrying sibling of onEvent(EventMask); default forwards to the 1-arg overload.
    virtual bool onEvent(EventMask e, INav&) {return onEvent(e);}

    template <typename Out>
    static constexpr bool printMenu(Out& out,Ctx& ctx)
      {return printMenu(out,ctx);}

  };

  // True when T has a REAL (not default) onEvent(EventMask,INav&) — i.e.
  // some component in T's own OO... chain actually wants the nav reference
  // (am4compat::EventActionItemNav, am4.h). Plain compile-time chain
  // components (EventAction, EventCall, EventActionItem — all below) never
  // declare this overload at all, deliberately: giving ItemAPI a default
  // 2-arg onEvent whose body tried to call back into the item's own 1-arg
  // onEvent(e) would be a real static-dispatch trap for any such
  // *non-virtual* chain (a Part<N> method reaching only its own lexical
  // scope, not a more-derived override) — there's no vtable to save it
  // there the way there is for IItem's own 2-arg default above. So
  // instead: no default at all in the compile-time chain, and callers (this
  // trait's two users: IItemDef::onEvent below, and nav.h's EventDispatch)
  // check first, falling back to the ordinary 1-arg onEvent(e) — which for
  // a plain item already correctly reaches the real, most-derived override
  // (that's what ordinary compile-time chain composition already does) and
  // for an IItem is the genuinely safe virtual self-call above.
  template<typename T, typename = void> struct HasNavOnEvent : std::false_type {};
  template<typename T> struct HasNavOnEvent<T, std::void_t<decltype(
      std::declval<T&>().onEvent(std::declval<EventMask>(), std::declval<INav&>()))>> : std::true_type {};

  template<typename... II>
  struct IItemDef:IItem, ItemDefC<hapi::CRTP<IItemDef<II...>>,II...> {
    using Base=ItemDefC<hapi::CRTP<IItemDef<II...>>,II...>;
    using Base::Base;
    // Deliberately virtual (runtime-polymorphic dispatch, via the IItem base
    // above) -- this is NOT an HLS synthesis target: no HLS backend can
    // synthesize a real vtable call (confirmed: Bambu segfaults on it, see
    // OneOutput/.RnD/hls/FINDINGS.md -- same mechanism applies here). No
    // static_assert here -- std::is_polymorphic_v<IItemDef> on the injected
    // class name hits "incomplete type" this early in the class body, and
    // the property can't silently regress anyway: it's baked into the
    // IItem base right above.
    // IItem also declares a nav() template (forwarding to the virtual
    // _nav/_kbdNav, for callers holding only an IItem&/IItem*) — ambiguous
    // multiple-inheritance name lookup otherwise when nav<>() is called on
    // the concrete IItemDef type directly (e.g. composed into a real
    // StaticBody, as every other item is). Prefer the real implementation;
    // _nav/_kbdNav below still serve the IItem&-typed callers.
    using Base::nav;
    // Same reasoning for printItem: the real printer chain (ItemBodyPrinter,
    // printers.h) calls i.printItem(concreteOut,ctx) with the compile-time-
    // known Out& type, not IOut& — it has the concrete IItemDef<...> type in
    // hand (i's static type), so it never goes through virtual dispatch at
    // all. Without this, only the IOut&-typed virtual overload below is
    // visible, and a concrete OutDef<...> doesn't convert to IOut&. The
    // templated Base::printItem and the virtual IOut&-typed one below are
    // different signatures, so both stay
    // selectable — same non-colliding-overload shape as `nav` above.
    using Base::printItem;

    virtual bool printMenu(IOut& out,Ctx& ctx) override {return Base::printMenu(out,ctx);}
    virtual bool printBody(IOut& out,Ctx& ctx) override {return Base::printBody(out,ctx);}
    virtual void printItem(IOut& out,Ctx& ctx) override {Base::printItem(out,ctx);}
    virtual bool enabled() const override {return Base::enabled();}
    virtual void enable(bool o=true) override {return Base::enable(o);}
    [[nodiscard]] virtual bool changed() const override {return Base::changed();}
    virtual void sync() override {Base::sync();}
    virtual void sync(IOut& out) override {Base::sync(out);}
    // Not Base::up()/down(): those names collide with an unrelated concept
    // some components define (e.g. NumRange<N>::Part::up(NRP s=1), a
    // mutating value-stepper NumField deliberately re-exposes via `using
    // Base::up;` for its own internal nav() use) — blindly forwarding
    // picks up whichever same-named method the composed chain happens to
    // have, not necessarily one matching this bool-const "sibling exists"
    // contract. No real caller currently depends on a non-default answer
    // here (checked: zero call sites through IItem's own up()/down()).
    virtual bool up() const {return false;};
    virtual bool down() const {return false;};
    virtual bool _nav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<false>(n,cke,p);}
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<true>(n,cke,p);}
    virtual bool onEvent(EventMask e) override {return Base::onEvent(e);}
    // SFINAE-gated: only forward into Base's own 2-arg onEvent when Base
    // genuinely has one (a nav-aware component is actually composed in
    // II...); otherwise fall back to the virtual 1-arg onEvent(e) above —
    // see HasNavOnEvent's own doc comment for why this can't be a plain
    // unconditional Base::onEvent(e,n) call, or a default on the
    // compile-time chain itself.
    virtual bool onEvent(EventMask e, INav& n) override {
      if constexpr (HasNavOnEvent<Base>::value) return Base::onEvent(e,n);
      else return onEvent(e);
    }
    template <typename Out> static constexpr bool printMenu(Out& out,Ctx& ctx) {return Base::printMenu(out,ctx);}
  };

  //---------------------------------------------------------------------------------------------
  using ActionFunc=bool(&)(int);

  template<ActionFunc action>
  struct Action {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      static constexpr bool act(int i) {return action(i);}
      template<bool isKbd,typename Nav>
      static constexpr bool nav(Nav& n,const CKE& cke,Path path)
        {return cke.cmd==Cmd::Enter&&action(path.sel());}
    };
  };

  /// @brief Fires fn(raisedEvent) when EventDispatch raises any bit overlapping mask; use for Exit/Focus/Blur or combined masks (Action covers the plain Enter-only case).
  using EventFunc=bool(&)(EventMask);

  template<EventMask mask,EventFunc fn>
  struct EventAction {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      bool onEvent(EventMask e) {
        bool r=(e&mask)?fn(e):false;
        return Base::onEvent(e)||r;
      }
    };
  };

  /// @brief Event handler for the plain zero-arg `void fn()` handler shape (EventAction above needs `bool(EventMask)`).
  using VoidFunc=void(&)();

  template<EventMask mask,VoidFunc fn>
  struct EventCall {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      bool onEvent(EventMask e) {
        bool r=false;
        if(e&mask) {fn();r=true;}
        return Base::onEvent(e)||r;
      }
    };
  };

  /// Event handler that also receives the item itself; requires IItemDef<...>.
  using EventFuncItemPtr=bool(*)(EventMask,IItem&);

  /// Runtime-configurable event handler (mask/fn set via constructor).
  struct EventActionItem {
    template<typename I>
    struct Part:I {
      using Base=I;
      EventMask mask;
      EventFuncItemPtr fn;
      template<typename... Rest>
      constexpr Part(EventMask m,EventFuncItemPtr f,Rest&&... rest)
        : Base(std::forward<Rest>(rest)...), mask(m), fn(f) {}
      bool onEvent(EventMask e) {
        bool r=(e&mask)?fn(e,static_cast<IItem&>(Base::obj())):false;
        return Base::onEvent(e)||r;
      }
    };
  };

  /// @brief Scrolling/marquee text for a long item label; animates only while focused.
  /// Fits: plain full print. Overflows+focused: scrolls a rolling window. Overflows+blurred: static clip, no animation.
  /// @tparam cps scroll speed in characters per second
  /// @tparam pad right margin, in characters, reserved when scrolling
  template<Sz cps, Sz pad=1>
  struct TextRoll {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      mutable Sz rollPos = 0;
      mutable bool m_overflow = false;// cached by the last printItem(): does text need scrolling?
      mutable bool m_ticked = false;  // set by tick(), cleared by sync() — drives changed()
      hw::Period<(cps>0 ? 1000/cps : 1000)> roller;

      // Gated on m_overflow: an item whose text currently fits never even calls roller()
      // (so its Period<> doesn't accumulate elapsed time it'll never use), and never marks
      // itself dirty — ticking a short label forever would otherwise cost a wasted
      // single-row Update-mode reprint every period, for a label that never visibly changes.
      bool tick() {
        bool r=false;
        if(m_overflow && roller()) {rollPos++; m_ticked=true; r=true;}
        return Base::tick()||r;
      }

      // Update-mode's per-row redraw gate (printers.h's ItemPrinter) only unlocks a row
      // when *this item's* changed() is true — TickFocus's own changed(Out&) (nav.h) only
      // decides whether a redraw pass happens *at all*, not which rows within it actually
      // get re-sent to the device. Cleared by sync() below, same contract as Watch<>.
      [[nodiscard]] bool changed() const {return Base::changed()||m_ticked;}
      void sync() {m_ticked=false; Base::sync();}

      template<typename Out,typename Ctx>
      void printItem(Out& out,Ctx& ctx) noexcept {
        const char* t=Base::get();
        Sz len=(Sz)strlen(t);
        Sz fx=out.free().x;
        bool wasOverflow=m_overflow;
        m_overflow=len>fx;
        // hw::Period<> fires true on its very first call regardless of elapsed time
        // (clock.h's own documented "First call returns true immediately" — last starts
        // at 0). Since roller() is only ever called once overflow is true (tick()'s own
        // gate), that freebie first-fire would otherwise land on whatever tick() call
        // happens to be first after scrolling starts — one frame after the real initial
        // redraw, reading as a spurious extra redraw rather than a real period elapsing.
        // Resetting exactly on the false->true transition re-baselines it to "now".
        if(m_overflow&&!wasOverflow) roller.reset();
        if(!m_overflow) {Base::printItem(out,ctx); return;}// fits: normal print, continue chain
        Sz w=fx>pad?fx-pad:fx;
        if(ctx) {// focused + overflowing: scroll window
          if(rollPos>=len) rollPos=0;
          Sz n=len-rollPos;
          if(n>=w) out.put(t+rollPos,w);
          else {
            out.put(t+rollPos,n);
            if(w>n) out.put(' ');
            if(w>n+1) out.put(t,w-n-1);
          }
        } else out.put(t,w);// blurred + overflowing: static clip, no animation
        Base::Base::printItem(out,ctx);// Base's own print already replaced above — skip
                                        // it but still continue whatever's chained beneath
      }
    };
  };

  //attach an action on enter
  template <ActionFunc f>
  struct BodyAction {
    template <typename I>
    struct Part : I {
      using Base=I;
      using Base::Base;
      using Base::enabled;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {
        if(cke.cmd==Cmd::Enter&&p.len) f(p.last());
        return Base::template nav<isKbd>(n,cke,p);
      }
    };
  };

  /// @brief Extension of oneData::Hidden<II...>: suppresses print/printItem and adds oneMenu's own Cmd/CKE nav() routing.
  template<typename... II>
  struct Hidden {
    template<typename I>
    struct Part:oneData::Hidden<II...>::template Part<I> {
      using Base=typename oneData::Hidden<II...>::template Part<I>;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p)
        {return I::template nav<isKbd>(n,cke,p);}
    };
  };

  /// @brief Extension of oneData::IdText<id,Src>: for web formats (XmlFmt/JsonFmt) emits a marked id ("#N") instead of resolved text; other formats resolve text as usual.
  template<int id, typename Src>
  struct IdText {
    template<typename I>
    struct Part:oneData::IdText<id,Src>::template Part<I> {
      using Base=typename oneData::IdText<id,Src>::template Part<I>;
      using Base::Base;
      template<typename Out>
      void printItem(Out& out, Ctx& ctx) {
        if constexpr(hapi::query<IsXmlFmt,typename Out::Types>
                  || hapi::query<IsJsonFmt,typename Out::Types>) {
          out.put('#');
          out.put(id);
          I::printItem(out,ctx);
        } else {
          Base::printItem(out,ctx);
        }
      }
    };
  };

  /// @brief Per-item mouseover/description text (id-indexed, multilingual), a side-channel separate from the item's own label.
  /// XmlFmt/JsonFmt: emitted as a child tag for every item. TextFmt: an indented follow-up line for the focused item only. Other formats: no-op.
  template<int id, typename Src>
  struct Footer {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      using HasDescription=std::true_type;//see base.h — lets a FooterOnly device (out.h) tell this item has description content
      static const char* get() noexcept { return Src::text(id,oneData::MultiLangText::current); }
      template<typename Out>
      void printItem(Out& out, Ctx& ctx) {
        if constexpr(hapi::query<IsXmlFmt,typename Out::Types>
                  || hapi::query<IsJsonFmt,typename Out::Types>) {
          out.template fmtStart<Fmt::Footer>(ctx);
          out.put(get());
          out.template fmtStop<Fmt::Footer>(ctx);
          Base::printItem(out,ctx);
        } else if constexpr(hapi::query<IsTextFmt,typename Out::Types>) {
          Base::printItem(out,ctx);
          if(ctx) { out.nl(); out.put("  "); out.put(get()); }
        } else {
          Base::printItem(out,ctx);
        }
      }
    };
  };

  /// @brief Like Footer<id,Src>, sourced from a single fixed CText pointer (StaticText<>'s
  /// own NTTP style) instead of an id/Src multilingual table. Same Xml/Json/TextFmt dispatch,
  /// plus a HasFooterOnly branch (out.h): a device tagged FooterOnly shows just the focused
  /// item's description, nothing else — unlike the TextFmt branch, it never calls
  /// Base::printItem, since a footer-only device never shows the item's own label/body.
  /// HasFooterOnly<Out> (SFINAE probe, not hapi::query<Tag,Out::Types>) is deliberate — Out is
  /// commonly IOutDef (out.h's own Types override empties Out::Types for IOut&-erasure
  /// safety, which also blinds a Types-query here); see out.h's HasFooterOnly for the full why.
  template<const CText* Text>
  struct StaticFooter {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      using HasDescription=std::true_type;//see base.h — lets a FooterOnly device (out.h) tell this item has description content
      static const char* get() noexcept { return *Text; }
      template<typename Out>
      void printItem(Out& out, Ctx& ctx) {
        if constexpr(HasFooterOnly<Out>::value) {
          if(ctx) out.put(get());
        } else if constexpr(hapi::query<IsXmlFmt,typename Out::Types>
                  || hapi::query<IsJsonFmt,typename Out::Types>) {
          out.template fmtStart<Fmt::Footer>(ctx);
          out.put(get());
          out.template fmtStop<Fmt::Footer>(ctx);
          Base::printItem(out,ctx);
        } else if constexpr(hapi::query<IsTextFmt,typename Out::Types>) {
          Base::printItem(out,ctx);
          if(ctx) { out.nl(); out.put("  "); out.put(get()); }
        } else {
          Base::printItem(out,ctx);
        }
      }
    };
  };

  template<typename... II>
  struct Decor {
    template<typename I>
    struct Part:oneItem::Decor<II...>::template Part<I> {
      using Base=typename oneItem::Decor<II...>::template Part<I>;
      using Base::Base;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {Base::printItem(out,ctx);}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) noexcept
        {return I::template nav<isKbd>(n,cke,p);}
    };
  };

  template<bool ens>
  struct EnDis {
    using Type = bool;
    template<typename I>
    struct Part:Chain<Hidden<Default<Bool,ens>>>::template Part<I> {
      using Base=typename Chain<Hidden<Default<Bool,ens>>>::template Part<I>;
      using Base::Base;
      bool enabled(const Path ={}) const {return Base::get();}
      void enable(bool e=true) {Base::set(e);}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path)
        {return enabled()?Base::template nav<isKbd>(n,cke,path):false;}
    };
  };

  //fields ---
  struct EditField {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      bool m_levelOpened{false};  // true when I::nav opened a level on Enter (e.g. TextField padOpen)
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r=false;
        if(cke.cmd==Cmd::Enter) {
          auto lv=n.level();
          n.navMode(path.len?NavMode::Nav
            :(n.navMode()==NavMode::Edit?NavMode::Nav:NavMode::Edit));
          r=true;
          bool ir=I::template nav<isKbd>(n,cke,path);
          m_levelOpened=(n.level()!=lv&&n.navMode()==NavMode::Edit);
          return ir||r;
        } else if(cke.cmd==Cmd::Esc&&!path.len&&n.navMode()==NavMode::Edit) {
          n.navMode(NavMode::Nav);
          // if I::nav opened a level on Enter, let close() fire to undo it (r stays false)
          // if not (NumField, date sub-items), consume Esc so close() is skipped
          if(!m_levelOpened) r=true;
          m_levelOpened=false;
        }
        return I::template nav<isKbd>(n,cke,path)||r;
      }
    };
  };

  /// @brief Fires fn(v) on every value mutation while editing; v is re-read via get() after Base::set() (may be clamped/transformed).
  /// Compose inside NumField<...>'s range/storage chain, e.g. NumField<StaticNumRange<...>, OnChange<fn>, AsField<Watch<...>>>.
  template<auto fn>
  struct OnChange {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      using Base::get;
      using Type=typename Base::Type;
      void set(Type v) {Base::set(v); fn(Base::get());}
    };
  };

  /// @brief Fires fn(v) once when edit mode is left (Enter-to-commit or Esc-while-editing; Esc is treated as a commit, not a cancel).
  /// Compose outside (wrapping) EditField, e.g. ItemDef<..., OnUpdate<fn>, EditField, NumField<...>>.
  template<auto fn>
  struct OnUpdate {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      using Base::get;
      using Type=typename Base::Type;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool wasEdit=n.navMode()==NavMode::Edit;
        bool r=I::template nav<isKbd>(n,cke,path);
        if(wasEdit&&n.navMode()!=NavMode::Edit) fn(get());
        return r;
      }
    };
  };

  /// @brief Reverts to the value held at edit-mode entry when Esc leaves editing (an Enter-commit leaves the live value untouched).
  /// Compose outside (wrapping) EditField, e.g. ItemDef<..., CancelOnEsc, EditField, NumField<...>>.
  struct CancelOnEsc {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      using Base::get;
      using Base::set;
      using Type=typename Base::Type;
      Type saved{};
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool wasEdit=n.navMode()==NavMode::Edit;
        if(!wasEdit&&!path.len&&cke.cmd==Cmd::Enter) saved=get();
        bool r=I::template nav<isKbd>(n,cke,path);
        if(wasEdit&&n.navMode()!=NavMode::Edit&&cke.cmd==Cmd::Esc) set(saved);
        return r;
      }
    };
  };

  //use range and data field to change value
  template<typename...II>
  struct NumField {
    template<typename I>
    struct Part:Chain<II...>::template Part<I> {
      using Base=typename Chain<II...>::template Part<I>;
      using Base::Base;
      using Base::set;
      using Base::get;
      using Base::up;
      using Base::down;
      using Base::low;
      using Base::high;
      using Type=typename Base::Type;
      constexpr bool valid() const {return Base::valid(get());}
      constexpr bool clamp() {return Base::set(Base::clamp(get()));}

      // Real child tags (Fmt::Low/High — not attribute-only, see enums.h's
      // own comment), emitted BEFORE the field's own <fld> so a consumer
      // sees range-then-value in document order. low()/high() are plain
      // OneData accessors (StaticNumRange, added alongside this) — safe to
      // call from here since this Part already lives in oneMenu, not
      // oneData.
      //
      // Gated on hapi::query<IsXmlFmt,Out::Types> — NOT unconditional.
      // fmtStart/fmtStop alone would be safely no-op on other formats (out.h's
      // own base default), but the out.put(low()/high()) calls in between are
      // NOT — put() is a real, unconditional device write on every format,
      // so without this gate a real ANSI/graphics display would show
      // "Power0100755%" — the range numbers rendered before the real value.
      // IsXmlFmt is a plain hapi::TagIs<aXmlFmt> predicate (base.h) — same
      // hapi::query<...,typename Out::Types> pattern already used
      // throughout this file (IsCursor/IsFillRect checks) — Out is the same
      // single, top-level device object type threaded through the whole
      // printItem recursion, so this check is genuinely per-active-format.
      template<typename Out>
      void printItem(Out& out, Ctx& ctx) {
        // JsonFmt admitted alongside XmlFmt — jsonFmt.h implements
        // Fmt::Low/High itself (its own quoted-"lo"/"hi"-string
        // convention), reusing the same fmtStart/put/fmtStop calls below
        // unchanged for both formats.
        if constexpr(hapi::query<IsXmlFmt,typename Out::Types>
                  || hapi::query<IsJsonFmt,typename Out::Types>) {
          out.template fmtStart<Fmt::Low>(ctx);
          out.put(low());
          out.template fmtStop<Fmt::Low>(ctx);
          out.template fmtStart<Fmt::High>(ctx);
          out.put(high());
          out.template fmtStop<Fmt::High>(ctx);
        }
        Base::printItem(out,ctx);
      }

      // Digit-literal accumulator: numeric fields need to deliver Cmd::Key
      // when nav is on edit mode (see nav.h's IndexGo, idxParser.h). AM4's
      // own menuField::parseInput reads a whole numeric
      // token in one stream call (in.parseFloat()); OneMenu's nav()
      // processes one keypress per call, so the literal is built up across
      // multiple nav() invocations instead, then handed to Base::setStr
      // (oneData's NumRange/StaticNumRange — already-working
      // strtod/strtol+clamp()+set(), reached via plain inheritance) — this
      // buffer only ever exists to grow a string, never reimplements parsing.
      // NumFieldDef has no PadDraw (unlike TextFieldDef), so there's no
      // per-character cursor dimension to reuse from Path the way TextField
      // does — Path is always {len=0,...} here, hence the own buffer.
      static constexpr Sz LitBuf=16;  // "-2147483648\0" + margin
      char m_lit[LitBuf]{0};
      Sz   m_litLen{0};

      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path) {
        if(n.navMode()==NavMode::Edit) {
          // any non-Key command abandons the in-progress literal — Up/Down
          // stepping or Enter/Esc must never leave stale chars for next time
          if(cke.cmd!=Cmd::Key) m_litLen=0;
          switch(cke.cmd){
            // Natural mapping (Up increases, Down decreases) — matches
            // AM4's own real shipped default (config::invertFieldKeys =
            // false, confirmed against AM4's actual source; the config
            // struct's own constructor default of true is overridden at
            // the point of instantiation) and is consistent with plain
            // (non-edit-mode) Up/Down semantics elsewhere in this
            // codebase. Compose oneData::InvDir<Range,true> in place of a
            // plain Range (e.g. NumField<InvDir<StaticNumRange<...>,true>,
            // AsField<...>>) for a field that wants the opposite —
            // AM4's own comment on the equivalent option suggests it
            // exists to compensate for specific input devices (e.g. a
            // rotary encoder), not as a universal default.
            case Cmd::Up: Base::up(); return true;
            case Cmd::Down: Base::down(); return true;
            case Cmd::Key: {
              char c=char(cke.key);
              if(c==8||c==127) {  // backspace — mirrors TextField::PartEnd
                if(m_litLen>0) m_lit[--m_litLen]='\0';
                if(m_litLen>0) Base::setStr(n,m_lit,path);
                return true;
              }
              bool isDigit=c>='0'&&c<='9';
              bool isDot=c=='.'&&std::is_floating_point<std::decay_t<Type>>::value
                                &&!strchr(m_lit,'.');
              bool isSign=c=='-'&&m_litLen==0;  // leading sign only
              if((isDigit||isDot||isSign)&&m_litLen<LitBuf-1) {
                m_lit[m_litLen++]=c;
                m_lit[m_litLen]='\0';
                Base::setStr(n,m_lit,path);  // live update as you type, AM4-parity UX
                return true;
              }
              return true;  // swallow any other typed char while editing
            }
            default: break;
          }
        }
        return Base::template nav<isKbd>(n,cke,path);
      }
    };
  };

  /// @brief Stores and restores navigation position.
  /// @tparam IsRealMenu true for real nested-menu navigation (ChooseFieldDef); false for cycle-in-place fields (Toggle/Select), which compose RecallNavPos<false> directly.
  template<bool IsRealMenu=true>
  struct RecallNavPos {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<typename... OO>
      constexpr Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        // Choose (IsRealMenu): tag THIS item's own <item> tag with
        // choice="1" BEFORE the title prints — while the tag's own
        // attribute-writing window is still open (Base::printItem below,
        // once called, prints title text and closes it). Choose still
        // falls through to show its own current value inline (same as
        // every other format, below) — the choice="1" marker is what lets
        // a web client render THIS as a clickable link with its value
        // shown, instead of the plain item[fld] editable-input template a
        // bare <fld> child would otherwise match. JsonFmt is admitted
        // alongside XmlFmt here and below — jsonFmt.h implements
        // Fmt::Choice/Option/Selected itself, reusing the same
        // fmtStart/put/fmtStop calls unchanged for both formats.
        if constexpr(IsRealMenu && (hapi::query<IsXmlFmt,typename Out::Types>
                                 || hapi::query<IsJsonFmt,typename Out::Types>)) {
          out.template fmtStart<Fmt::Choice>(ctx);
          out.template fmtStop<Fmt::Choice>(ctx);
        }
        Base::printItem(out,ctx);
        out.setPos(out.getPos());
        // Toggle/Select (!IsRealMenu): needs its FULL option list (for a
        // radio-group/dropdown widget), not just the single currently-
        // selected sub-item's own inline rendering below — walk every
        // option, each wrapped in <opt>, tagged with a plain <sel> 0/1 (a
        // stored index comparison, not ctx-derived — there is no real
        // navigation focus on the non-selected options, only m_sel itself).
        if constexpr((hapi::query<IsXmlFmt,typename Out::Types>
                   || hapi::query<IsJsonFmt,typename Out::Types>) && !IsRealMenu) {
          for(Sz i=0; i<Base::body.size(); i++) {
            out.template fmtStart<Fmt::Option>(ctx);
            out.template fmtStart<Fmt::Selected>(ctx);
            out.put(i==m_sel ? 1 : 0);
            out.template fmtStop<Fmt::Selected>(ctx);
            Base::body.printItem(out,ctx,i);
            out.template fmtStop<Fmt::Option>(ctx);
          }
          return;
        }
        if(ctx) {
          if(ctx.mode==NavMode::Edit||ctx.after()>1) Base::body.printItem(out,ctx,ctx.path.last());
          else Base::body.printItem(out,ctx,m_sel);
        }
        else Base::body.printItem(out,ctx,m_sel);
      }
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        if(cke.cmd==Cmd::Enter) {
          if(path.len) m_sel=path.sel();//store selected item index
          else {
            bool r=I::template nav<isKbd>(n,cke,path);//still check base
            n.go(m_sel);//restore selected index
            return r;
          }
        }
        return I::template nav<isKbd>(n,cke,path);//still check base
      }
      template<typename Fn>
      auto visit(Fn&& fn) {return Base::body.visit(m_sel,std::forward<Fn>(fn));}
      // Web/async value-set path (AsyncNav::set() -> Menu::setStr() -> here,
      // reached via the item's OWN path — e.g. /set?path=/3/2/&val=1 for
      // Toggle/Select's option index): parse the given string as a plain
      // option index and jump directly to it. Same strtol+p.len==0 pattern
      // oneData's own StaticNumRange/StaticRange already use for their own
      // /set endpoint — NOT the same mechanism as nav()'s own path.len check
      // above (that one reflects the nav's stored multi-level cursor state
      // for physical multi-level restore, not a caller-supplied index).
      // A deeper /nav?path=... route cannot be reused for direct web
      // option-jump: AsyncNav::async() fires a real Cmd::Enter on every
      // *intermediate* path segment, which cycles/opens whatever item sits
      // there rather than reaching m_sel. This setStr()-based route
      // sidesteps that entirely by addressing the field directly, one path,
      // no nested segment.
      template<typename Nav,typename P>
      bool setStr(Nav&,const char* s,P p) {
        if(p.len==0) {
          Sz i=(Sz)strtol(s,nullptr,10);
          if(i<Base::body.size()) { m_sel=i; return true; }
        }
        return false;
      }
      protected: Sz m_sel{0};
    };
  };

  /// @brief tags a choice item with its real value, retrieved via RecallNavPos::visit(fn)
  template<auto V>
  struct EnumValue {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static constexpr decltype(V) value() noexcept { return V; }
    };
  };

  /// @brief Syncs the selected choice's EnumValue<V> to external storage W, on both a physical commit and a web-driven setStr().
  /// W follows the standard Data-chain shape (Data<T>, DataRef<&ext>, DataFn<Src>, or a chain). Place above a RecallNavPos-based field.
  template<typename W>
  struct SyncValue {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      using Storage = typename W::template Part<oneData::DataAPI<>>;
      Storage m_value{};

      auto value() const noexcept { return m_value.get(); }

      void syncFromSel() { Base::visit([this](auto& item){ m_value.set(item.value()); }); }

      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r = Base::template nav<isKbd>(n,cke,path);
        if(cke.cmd==Cmd::Enter) syncFromSel();
        return r;
      }

      // AsyncNav::setAt() (the web /set and WS "S|" path) writes m_sel via
      // RecallNavPos::setStr() directly, bypassing nav()/Cmd::Enter entirely
      // (by design — see asyncNav.h's own comment) — so the mirror-sync
      // above never fired for a web-driven selection change, only a real
      // physical/keyboard-driven commit. Mirroring here too closes that gap
      // with the same logic, not a new mechanism: whichever path actually
      // moved m_sel, sync the newly-selected item's value right after.
      template<typename Nav,typename P>
      bool setStr(Nav& n,const char* s,P p) {
        bool r = Base::setStr(n,s,p);
        if(r) syncFromSel();
        return r;
      }
    };
  };

  struct ItemNav; // forward — needed by ParentDraw::rules()

  struct ParentDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<RecallNavPos<>>, After>,
        "ParentDraw: RecallNavPos must be placed above ParentDraw in the ItemDef chain");
      static_assert(Excludes<hapi::SameAs<ItemNav>, Before, After>,
        "ParentDraw: ItemNav cannot coexist with ParentDraw — ParentDraw handles Enter/close by itself; remove ItemNav");
      return true;
    }
    template<typename I>
    struct Part:I {
      using I::I;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r=I::template nav<isKbd>(n,cke,path);
        if(!r&&cke.cmd==Cmd::Enter)
          r=path.len>0?n.close():n.padOpen();
        return r;
      }
    };
  };

  /// @brief put nav focus on this item on Cmd::Enter
  struct ItemNav {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<ParentDraw>, Before, After>,
        "ItemNav: cannot coexist with ParentDraw — ParentDraw handles Enter/close; remove ItemNav");
      return true;
    }
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path) {
        // Call base first — deeper component (ParentDraw etc.) gets priority.
        // ItemNav only acts when nothing deeper consumed Cmd::Enter.
        bool r=Base::template nav<isKbd>(n,cke,path);
        if(!r&&cke.cmd==Cmd::Enter) {
          if(path.len==0) r=n.open();
          else if(path.len==1) r=n.close();
        }
        return r;
      }
    };
  };

  /// @brief Alternative representation for a value.
  template<typename Title,typename Value>
  struct Alias {
    template<typename O>
    struct Part:Title::template Part<O> {
      using Base=typename Title::template Part<O>;
      ItemDef<Value> value{};
      using Base::Base;
      template<typename... OO>
      Part(Value&& v,OO&&... oo):value{v},Base{std::forward<OO&&>(oo)...}{}
    };
  };

  template<typename R,R& ref>
  struct ItemRef {
    template<typename O>
    struct Part:O {
      using Base=O;
      using Base::Base;
      using RefType=R;
      operator RefType&() const {return ref;}
      static constexpr const Depth depth() {return ref.depth();}
      static constexpr bool enabled() {return ref.enable(); }
      static constexpr void enable(bool o=true) {ref.enable(o);}
      [[nodiscard]] static constexpr bool changed() {return ref.changed();}
      static constexpr void sync() {ref.sync();}
      static constexpr bool up() {return ref.up();}
      static constexpr bool down() {return ref.down();}

      template <typename Out>
      static constexpr bool printMenu(Out& out,Ctx& ctx) 
        {return ref.printMenu(out.ctx);}
      
      template<typename Out>
      static constexpr bool printBody(Out& out,Ctx&ctx)
        {return ref.printBody(out,ctx);}

      template<typename Out> 
      static constexpr void printItem(Out& out,Ctx& ctx)
        {ref.printItem(out,ctx);}

      template<bool isKbd,typename Nav>
      static constexpr bool nav(Nav& n,const CKE& cke,const Path p)
        {return ref.nav(n,cke,p);}
    };
  };

  template<typename... OO>
  struct Put {
    template<typename Alt,Alt& alt,Clear clr=Clear::no>
    struct ToOut {
      struct End {
        template<typename O>
        struct Part:O {
          using Base=O;
          using Base::Base;
          template<typename Out> static constexpr void printItem(Out&,Ctx&) noexcept {}
          template<typename Out> static constexpr void print(Out&) noexcept {}
        };
      };
      template<typename O>
      struct Part:Chain<OO...,End>::template Part<O> {
        using Base=typename Chain<OO...,End>::template Part<O>;
        using Base::Base;
        template<typename Out> void print(Out& out) const { O::print(out); }
        template<typename Out>
        void printItem(Out& out,Ctx& ctx) {
          if(!(out.lockMode()==LockMode::None||out.lockMode()==LockMode::Update)) return;
          LockMode lm=out.lockMode();
          alt.resume();
          if(clr==Clear::yes) alt.clear();
          Base::printItem(alt,ctx);
          out.resume();  // restores lockMode→None, then cursor→origin, then colors
          out.lockMode(lm);
        }
      };
    };
  };

  template<typename... OO>
  struct OnFocus {
    struct End {
      template<typename O>
      struct Part:O {
        using Base=O;
        using Base::Base;
        template<typename Out> static constexpr void printItem(Out&,Ctx&) noexcept {}
        template<typename Out> static constexpr void print(Out&) noexcept {}
      };
    };
    template<typename O>
    struct Part:Chain<OO...,End>::template Part<O> {
      using Base=typename Chain<OO...,End>::template Part<O>;
      using Base::Base;
      template<typename Out> void print(Out& out) const { O::print(out); }
      template<typename Out> void printItem(Out& out,Ctx& ctx) {
        O::printItem(out,ctx);
        if(ctx) Base::printItem(out,ctx);
      }
    };
  };

  // compile-time ids --------------
  template<int id> struct Id {template<typename O> using Part=O;};
  template<auto V> inline constexpr hapi::SameAs<Id<V>> byId{};

  // liquid position — item jumps to a fixed screen position before rendering ----
  // Incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter): scroll
  // measurement assumes items advance the cursor sequentially; a liquid item warping
  // away and back corrupts that math. Enforced below, not just documented — a rule.
  // On a non-cursor device (e.g. plain serial) position is meaningless, so Liquid
  // falls back to a normal in-sequence print instead of touching setPos at all.

  /// @brief Compile-time box SIZE only (w,h); pairs with Liquid<x,y,Box> to declare a per-item highlight-box override for GfxColorFmt.
  template<Sz W,Sz H>
  struct BoxSize { static constexpr Sz w=W,h=H; };

  namespace detail_ {
    template<bool> struct MaybeLiquidBoxBase {};
    template<> struct MaybeLiquidBoxBase<true> : aLiquidBox {};
  }

  /// @brief Compile-time item position: printItem moves the cursor to (x,y), renders, then restores the original position.
  /// @tparam Box optional BoxSize<W,H> declaring this item's own on-screen box, scoping GfxColorFmt's selection highlight to it instead of the full row width.
  template<Sz x,Sz y,typename Box=hapi::Nil>
  struct Liquid : detail_::MaybeLiquidBoxBase<!std::is_same_v<Box,hapi::Nil>> {
    template<typename I>
    struct Part:I {
      using Base=I;
      static constexpr Pos liquidBoxPos() {return {x,y};}
      static constexpr Area liquidBoxSize() {return {Box::w,Box::h};}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Excludes<IsScrollBody,typename Out::Types>,
          "Liquid: incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "scroll measurement needs sequential item positions; use FullPrinter/NoTitlePrinter instead");
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Pos saved=out.getPos();
          out.setPos({x,y});
          Base::printItem(out,ctx);
          out.setPos(saved);
        } else Base::printItem(out,ctx);  // no addressable cursor: fall back to normal print
      }
    };
  };

  /// @brief Relative position nudge: prints wrapped content OO... offset by (+x,+y) from the current cursor position, then restores.
  /// Tail-position use only (last component in an ItemDef, e.g. `Shift<3,1,StaticText<&text::k1>>`) — does not thread through a following sibling.
  template<Sz x,Sz y,typename... OO>
  struct Shift {
    template<typename I>
    struct Part : hapi::Chain<OO...>::template Part<I> {
      using Base = typename hapi::Chain<OO...>::template Part<I>;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Excludes<IsScrollBody,typename Out::Types>,
          "Shift: incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "scroll measurement needs sequential item positions; use FullPrinter/NoTitlePrinter instead");
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Pos saved=out.getPos();
          out.setPos({saved.x+x, saved.y+y});
          Base::printItem(out,ctx);
          // out.setPos(saved);
        } else Base::printItem(out,ctx);  // no addressable cursor: fall back to normal print
      }
    };
  };

  /// @brief runtime item position: set liquidPos({x,y}) to relocate item on screen.
  /// Same save/restore and non-cursor fallback behavior as Liquid<x,y>.
  struct LiquidPos {
    template<typename I>
    struct Part:I {
      using Base=I;
      Pos m_liquidPos{0,0};
      void liquidPos(Pos p) {m_liquidPos=p;}
      Pos liquidPos() const {return m_liquidPos;}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Excludes<IsScrollBody,typename Out::Types>,
          "LiquidPos: incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "scroll measurement needs sequential item positions; use FullPrinter/NoTitlePrinter instead");
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Pos saved=out.getPos();
          out.setPos(m_liquidPos);
          Base::printItem(out,ctx);
          out.setPos(saved);
        } else Base::printItem(out,ctx);  // no addressable cursor: fall back to normal print
      }
    };
  };

  /// @brief Item claims the rest of the current page: pads down to the bottom of the free area after printing its own content.
  /// Requires ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter).
  struct FullScreen {
    template<typename I>
    struct Part:I {
      using Base=I;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Requires<IsScrollBody,typename Out::Types>,
          "FullScreen: requires ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "use one of those instead of FullPrinter/NoTitlePrinter");
#ifdef MENU_DEBUG_FULLSCREEN
        if(out.lockMode()!=LockMode::Changed&&out.lockMode()!=LockMode::Sync) {
          Pos p0=out.getPos();
          printf("[FullScreen] before content print: pos=(%d,%d) lockMode=%d\n",(int)p0.x,(int)p0.y,(int)out.lockMode());
        }
#endif
        Base::printItem(out,ctx);
#ifdef MENU_DEBUG_FULLSCREEN
        if(out.lockMode()!=LockMode::Changed&&out.lockMode()!=LockMode::Sync) {
          Pos p1=out.getPos();
          Area f1=out.free();
          printf("[FullScreen] after content print: pos=(%d,%d) free=(%d,%d)\n",(int)p1.x,(int)p1.y,(int)f1.x,(int)f1.y);
        }
#endif
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          // Runs before the enclosing ItemPrinter's own fmtStop<Fmt::Item> (clearToEOL()+nl(),
          // same shape as Cursor::clearLine()) finalizes *this* row — so free().y here still
          // counts the still-open current row as "free". Reserve out.lineHeight() rows (not a
          // hardcoded 1) for that finalization: a normal row costs 1 nl(), but a format that
          // renders this row big (e.g. GfxFmt's item-level big font) costs 2 — and at this point
          // big-font state (if any) is still active, so a Cursor<...,LnH> with a dynamic
          // line-height fn already reports the right number here. Reserving a fixed 1 for a
          // big row under-reserves by one and overshoots.
          if constexpr(hapi::query<IsFillRect,typename Out::Types>) {
            // GFX outputs (HasFillRect via aFillRect, e.g. OledOut): one native-coord rect
            // fill instead of a clearToEOL()+nl() per row. free().y here still includes the
            // current (just-printed, not yet closed) row — fill only what's *below* it
            // (p.y+L onward), never the row itself: text was drawn there via
            // Base::printItem(out,ctx) just above, and fillRect must never re-touch pixels
            // the item's own content already owns — starting the fill *at* p.y instead of
            // p.y+L would paint over the just-drawn text.
            Sz L = out.lineHeight();
            Area f = out.free();
            if(f.y>L) {
              Pos p = out.getPos();
              Sz fillH = f.y-L;
#ifdef MENU_DEBUG_FULLSCREEN
              if(out.lockMode()!=LockMode::Changed&&out.lockMode()!=LockMode::Sync) {
                printf("[FullScreen] fillRect(x=%d,y=%d,w=%d,h=%d) L=%d f=(%d,%d)\n",
                  (int)out.orgX(),(int)(p.y+L),(int)out.width(),(int)fillH,(int)L,(int)f.x,(int)f.y);
              }
#endif
              out.fillRect(out.orgX(), p.y+L, out.width(), fillH);
              out.setPos({0, p.y+f.y});
            }
          } else {
            while(out.free().y>out.lineHeight()) out.clearLine();
          }
        }
      }
    };
  };

  /// @brief Horizontal text alignment (Left/Center/Right) tag/grouping; no rendering logic of its own — compose Row<Left,Center,Right> for actual alignment.
  template<AlignMethod method,typename... II>
  struct Align {
    template<typename I>
    struct Part:hapi::Chain<II...>::template Part<I> {
      using Base=typename hapi::Chain<II...>::template Part<I>;
      using Base::Base;
    };
  };

  template<typename... II> using AlignLeft   = Align<AlignMethod::Left,II...>;
  template<typename... II> using AlignCenter = Align<AlignMethod::Center,II...>;
  template<typename... II> using AlignRight  = Align<AlignMethod::Right,II...>;

  /// @brief Coordinates 3 independently-printable items on one line: Left at the current position, Right flush right, Center filling the gap between them.
  /// Left/Center/Right must each be a standalone printable+constructible type (typically an ItemDef<...>); nav()/setStr()/changed()/sync()/etc. are forwarded to Center.
  template<typename Left,typename Center,typename Right>
  struct Row {
    template<typename I>
    struct Part:I {
      using Base=I;
      Left   leftItem{};
      Center centerItem{};
      Right  rightItem{};

      // Forward the interactive surface to centerItem — same reasoning as
      // Rows's forwarding to bodyItem below: Left/Right are structural (matching
      // CenterRow's own established convention of empty Left/Right placeholders, real
      // content always in Center), and without this an editable field placed in Center
      // would silently never receive nav()/setStr(),
      // NOR would its own changed() reach the outer redraw-gating — the latter isn't just
      // cosmetic: TreeNav::changed(out) only re-probes the print tree when nav-level state
      // (level/navMode/selection) is unchanged, which is exactly the case for adjusting an
      // already-open field's value via up/down without also toggling edit mode — without
      // this forward, that value change would never be detected as needing a redraw.
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {return centerItem.template nav<isKbd>(n,cke,p);}
      template<typename Nav,typename P>
      bool setStr(Nav& n,const char* s,P p) {return centerItem.setStr(n,s,p);}
      [[nodiscard]] bool changed() const {return centerItem.changed();}
      void sync() {centerItem.sync();}
      template<typename Out> void sync(Out& out) {centerItem.sync(out);}
      bool enabled() const {return centerItem.enabled();}
      void enable(bool o=true) {centerItem.enable(o);}
      bool up() {return centerItem.up();}
      bool down() {return centerItem.down();}
      bool onEvent(EventMask e) {return centerItem.onEvent(e);}
      bool tick() {return centerItem.tick();}

      template<typename Out,typename PrintFn>
      static Sz measure(Out& out,PrintFn&& printFn) {
        Pos start=out.getPos();
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Measure);
        printFn();
        Sz used=out.getPos().x-start.x;
        out.setPos(start);
        out.lockMode(om);
        return used;
      }

      // Pads with spaces (like Align's own padWith use) rather than setPos() jumps —
      // setPos() is a no-op on plain streaming devices (no real cursor addressing),
      // exactly the trap that bit ScrollPrinter+TextFmt+SerialOut earlier; padWith
      // works on any device since it's just characters, not real positioning.
      template<typename Out,typename LeftFn,typename CenterFn,typename RightFn>
      static void rowPrint(Out& out,LeftFn&& leftFn,CenterFn&& centerFn,RightFn&& rightFn) {
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Sz leftW  =measure(out,leftFn);
          Sz rightW =measure(out,rightFn);
          Sz centerW=measure(out,centerFn);
          Sz lineW=out.free().x;
          Sz gapW =lineW>(leftW+rightW) ? lineW-leftW-rightW : 0;
          Sz centerOffset=gapW>centerW ? (gapW-centerW)/2 : 0;

          if constexpr(hapi::query<IsFillRect,typename Out::Types>) {
            // GFX outputs: clean the whole row with one native-coord rect fill first, then
            // position each of Left/Center/Right by its exact measured pixel offset — same
            // "clear background first, text always has the last word on its own pixels"
            // idiom as FullScreen's own fillRect,
            // rather than approximating position via padWith(N space glyphs), which floors
            // to whole-glyph increments and can leave up to one glyph-width of visible
            // off-center error on a real pixel-addressable device.
            Pos start=out.getPos();
            out.fillRect(start.x, start.y, lineW, out.lineHeight());
            // fillRect moves the *driver's own* internal cursor to the far edge of
            // whatever it just filled (Ssd1306::fillRect, OneIO) — separate from the
            // logical Cursor's own m_at, which fillRect never touches. Resync explicitly
            // before leftFn() rather than assume it's still at `start` — same pitfall as
            // gfxFmt.h's roleBig-tagged fmtStart. Currently harmless here since every real
            // Left is empty, but latent otherwise.
            out.setPos(start);
            leftFn();
            out.setPos({start.x+leftW+centerOffset, start.y});
            centerFn();
            out.setPos({start.x+lineW-rightW, start.y});
            rightFn();
          } else {
            // Non-GFX cursor devices (e.g. ANSI terminal): no fillRect capability, and a
            // bare setPos() jump would leave stale characters from a previous, differently-
            // sized frame sitting in the gap — padWith(' ') both positions and erases in one
            // step, the only way to guarantee a clean gap without a real rect-fill primitive.
            Sz trailing=gapW>(centerOffset+centerW) ? gapW-centerOffset-centerW : 0;
            leftFn();
            if(centerOffset>0) out.padWith(centerOffset/out.glyphWidth(' '));
            centerFn();
            if(trailing>0) out.padWith(trailing/out.glyphWidth(' '));
            rightFn();
          }
        } else {
          leftFn(); centerFn(); rightFn();
        }
      }

      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        rowPrint(out,
          [&]{leftItem.printItem(out,ctx);},
          [&]{centerItem.printItem(out,ctx);},
          [&]{rightItem.printItem(out,ctx);});
        Base::printItem(out,ctx);
      }
      template<typename Out>
      void print(Out& out) const {
        rowPrint(out,
          [&]{leftItem.print(out);},
          [&]{centerItem.print(out);},
          [&]{rightItem.print(out);});
        Base::print(out);
      }
    };
  };

  /// @brief Vertical layout: Top, then Body vertically anchored (per vMethod) within the free space above Bottom's reserved `bottomLines`, then Bottom.
  /// nav()/setStr()/changed()/sync()/etc. are forwarded to Body. Falls back to plain sequential printing on non-cursor devices.
  template<Sz bottomLines,AlignMethod vMethod,typename Top,typename Body,typename Bottom>
  struct Rows {
    template<typename I>
    struct Part:I {
      using Base=I;
      Top    topItem{};
      Body   bodyItem{};
      Bottom bottomItem{};

      // Forward the interactive surface to bodyItem — Top/Bottom are structural chrome
      // (a header/footer partition of the space, per the file comment above), never meant
      // to be interactive themselves. Without this, an editable field (NumField/EditField)
      // placed as Body would print correctly but silently never receive nav()/setStr(),
      // and its own changed()/sync() would never reach the outer redraw-gating — same
      // "display-only, editing silently doesn't work" restriction Row's Left/Center/Right
      // have — fixed here rather than left as a limitation, since a real editable field
      // vertically laid out under its own label is a real use case.
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {return bodyItem.template nav<isKbd>(n,cke,p);}
      template<typename Nav,typename P>
      bool setStr(Nav& n,const char* s,P p) {return bodyItem.setStr(n,s,p);}
      [[nodiscard]] bool changed() const {return bodyItem.changed();}
      void sync() {bodyItem.sync();}
      template<typename Out> void sync(Out& out) {bodyItem.sync(out);}
      bool enabled() const {return bodyItem.enabled();}
      void enable(bool o=true) {bodyItem.enable(o);}
      bool up() {return bodyItem.up();}
      bool down() {return bodyItem.down();}
      bool onEvent(EventMask e) {return bodyItem.onEvent(e);}
      bool tick() {return bodyItem.tick();}

      template<typename Out,typename PrintFn>
      static Sz measureLines(Out& out,PrintFn&& printFn) {
        Pos start=out.getPos();
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Measure);
        printFn();
        Pos end=out.getPos();
        Sz used=(end.y-start.y)/out.lineHeight();
        // A body that never calls its own trailing nl() (the normal case — a plain
        // single-line StaticText/Decimals/value item, same as any of Row's own
        // Left/Center/Right members) leaves end.x>0 with end.y unchanged, undercounting
        // by exactly one real line. Same "N newlines vs N+1 lines" accounting any
        // line-counter needs: count a trailing partial row too.
        if(end.x>0) used++;
        out.setPos(start);
        out.lockMode(om);
        return used;
      }

      template<typename Out,typename BodyFn>
      static void rowsPrint(Out& out,BodyFn&& bodyFn) {
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Sz availLines=out.free().y/out.lineHeight();
          availLines=availLines>bottomLines ? availLines-bottomLines : 0;
          Sz bodyLines=measureLines(out,bodyFn);
          Sz topPad =
            vMethod==AlignMethod::Center ? (availLines>bodyLines?(availLines-bodyLines)/2:0) :
            vMethod==AlignMethod::Right  ? (availLines>bodyLines? availLines-bodyLines  :0) :
            0;
          #ifdef MENU_DEBUG_ROWS
          if(out.lockMode()!=LockMode::Changed&&out.lockMode()!=LockMode::Sync&&out.lockMode()!=LockMode::Measure)
            printf("[Rows] availLines=%d bodyLines=%d topPad=%d\n",(int)availLines,(int)bodyLines,(int)topPad);
          #endif
          // padWith(' ')/clearLine() erase by *printing* space glyphs — fine on a real
          // character-cell device (ANSI/text), but not adequate on GFX: whether printing
          // a space glyph actually blanks the underlying pixels depends on the font
          // renderer's own background-fill behavior, not guaranteed the way a real
          // fillRect is. Same "clean via rectangle, then reposition" fix as FullScreen's
          // own padding.
          if constexpr(hapi::query<IsFillRect,typename Out::Types>) {
            if(topPad>0) {
              Pos p=out.getPos();
              Sz h=topPad*out.lineHeight();
              out.fillRect(out.orgX(),p.y,out.width(),h);
              out.setPos({out.orgX(),p.y+h});
            }
          } else {
            for(Sz i=0;i<topPad;i++) out.clearLine();
          }
          bodyFn();
          out.nl();
          Sz remainLines=out.free().y/out.lineHeight();
          if constexpr(hapi::query<IsFillRect,typename Out::Types>) {
            if(remainLines>bottomLines) {
              Pos p=out.getPos();
              Sz h=(remainLines-bottomLines)*out.lineHeight();
              out.fillRect(out.orgX(),p.y,out.width(),h);
              out.setPos({out.orgX(),p.y+h});
            }
          } else {
            while(remainLines-->bottomLines) out.clearLine();
          }
        } else {
          bodyFn();
          out.nl();
        }
      }

      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        topItem.printItem(out,ctx);
        out.nl();
        rowsPrint(out,[&]{bodyItem.printItem(out,ctx);});
        bottomItem.printItem(out,ctx);
        Base::printItem(out,ctx);
      }
      template<typename Out>
      void print(Out& out) const {
        topItem.print(out);
        out.nl();
        rowsPrint(out,[&]{bodyItem.print(out);});
        bottomItem.print(out);
        Base::print(out);
      }
    };
  };

  template<Sz bottomLines,typename Top,typename Body,typename Bottom>
  using RowsTop    = Rows<bottomLines,AlignMethod::Left,Top,Body,Bottom>;
  template<Sz bottomLines,typename Top,typename Body,typename Bottom>
  using RowsCenter = Rows<bottomLines,AlignMethod::Center,Top,Body,Bottom>;
  template<Sz bottomLines,typename Top,typename Body,typename Bottom>
  using RowsBottom = Rows<bottomLines,AlignMethod::Right,Top,Body,Bottom>;

  // output partition tag -------------------------------------------------------
  /// @brief Marks an item as belonging to output partition Tag; see SkipOutId<Tag> and PartitionBody<Tag,Body>.
  /// Nav-invisible when placed after the body's navSize() boundary.
  template<typename Tag>
  struct OutId {
    template<typename O> using Part=O;  // pure tag: zero behavior, queryable via SameAs<OutId<Tag>>
  };

};//namespace oneMenu

//rules ItemDef query specialization --
template<typename Q,typename... OO>
constexpr const bool hapi::query<Q,oneMenu::ItemDef<OO...>>{(hapi::query<Q,OO>||...)};

// ItemDef's own query<> bypass (above) only covers the *bare* form,
// hapi::query<Q,ItemDef<OO...>> — every real call site elsewhere in this codebase
// instead queries through ::Types (hapi::query<Tag,typename Out::Types> and siblings),
// which for a StaticBody/JoinBody/Menu element goes through THEIR Traverse
// specializations recursing via Traverse<Op,O> (not query<Q,O>) — so without this,
// ItemDef was still an opaque leaf on that path despite the bypass above. Found via
// this audit's own verification test (StaticBody/JoinBody/Menu's ::Types-based
// hapi::query silently missing a tag nested inside an ItemDef element) — the same
// bug class the whole audit is about, one level deeper than initially mapped.
namespace hapi {
  template<typename Op,typename... OO>
  struct Traverse<Op, oneMenu::ItemDef<OO...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, OO>::Beta...>;
  };
}

// Hidden<II...>/Decor<II...>/EnDis<ens>/NumField<II...> are all the same
// Chain<...>::Part<O>-wrapping shape as MenuPrinter (printers.h) — opaque to
// hapi::query/Traverse without their own specialization. None currently has a tag
// nested inside it, but NumField is the single most widely-used wrapper in the
// codebase (every AM4 FIELD() macro expansion) and itself nests AsField<...> inside
// it, so it's the most exposed of the four to a future tag landing inside it unnoticed.
namespace hapi {
  template<typename Op, typename... II>
  struct Traverse<Op, oneMenu::Hidden<II...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, II>::Beta...>;
  };
  template<typename Op, typename... II>
  struct Traverse<Op, oneMenu::Decor<II...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, II>::Beta...>;
  };
  // EnDis<ens>::Part<I> wraps a single element, Hidden<Default<Bool,ens>> — not a
  // pack, so ApplyPack takes that one Beta directly rather than expanding II....
  template<typename Op, bool ens>
  struct Traverse<Op, oneMenu::EnDis<ens>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, oneMenu::Hidden<oneData::Default<oneData::Bool,ens>>>::Beta>;
  };
  template<typename Op, typename... II>
  struct Traverse<Op, oneMenu::NumField<II...>> {
    using Beta = typename Op::template ApplyPack<typename Traverse<Op, II>::Beta...>;
  };
}

