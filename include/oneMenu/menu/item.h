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
    static constexpr bool enabled() {return true;}
    static constexpr bool wraps() {return false;}
    static constexpr bool isPad() {return false;}
    static constexpr void enable(bool=true) {}
    static constexpr bool changed() {return false;}
    static constexpr Pos pos() {return {0,0};}
    static constexpr void sync() {}
    template<typename Out> static constexpr void sync(Out&) {}
    static constexpr bool up() {return false;}
    static constexpr bool down() {return false;}
    template<typename Out> static constexpr bool printMenu(Out&,Ctx&) {return false;}
    template<typename Out> static constexpr bool printBody(Out&,Ctx&) {return false;}
    template<typename Out> static constexpr void printHidden(Out&,Ctx&) noexcept {}
    template<typename Out> static constexpr bool printHiddenMenu(Out&,Ctx&) noexcept {return false;}
    using Base::print;
    // printItem(Out&,Ctx&) is reached via ordinary inheritance from oneData::DataAPI's
    // generic no-op — no redeclaration or using-declaration needed here.
    template<bool isKbd,typename Nav> static constexpr bool nav(Nav& n,const CKE& cke,Path) {return false;}
    template<typename Nav,typename P> static constexpr bool setStr(Nav&,const char*,P) {return false;}
    /// @brief AM4-parity semantic event hook (see enums.h EventMask). Default no-op,
    /// same pattern as nav()/printItem() — override in a component that cares
    /// (e.g. EventAction<mask,fn> below), not by patching every item.
    static constexpr bool onEvent(EventMask) {return false;}
    /// @brief per-frame animation hook, dispatched only to the currently-focused item
    /// (see TickFocus, nav.h). Default no-op — returns false (nothing to redraw), same
    /// pattern as onEvent()/nav(). Override in a component that animates (e.g. TextRoll
    /// below) to advance internal state and report whether a redraw is needed.
    static constexpr bool tick() {return false;}
  };

  template<typename... OO> struct ItemDef; // forward — ItemDefC's Ins/App alias it below

  /// @brief ItemDef's real logic, parameterized on Cfg (threaded down to
  /// ItemAPI<Cfg> -> oneItem::ItemAPI<Cfg> -> oneData::DataAPI<Cfg>, which
  /// derives from Cfg directly — the same "anchor" slot hapi::Chain<>::Part<T>
  /// uses for T). Plain ItemDef<OO...> below is just Cfg=Nil. IItemDef (this
  /// file) is the other instantiation, Cfg=hapi::CRTP<IItemDef<II...>> — giving
  /// its OO... chain a working obj() self-reference, same pattern nav.h's
  /// DefinedNav<API,NN...> already uses for NavDef/INavDef. Needed so a
  /// component nested in OO... can reach the fully-assembled IItemDef&, and
  /// through it, IItem& (see EventActionItem below) — a plain static_cast from
  /// inside OO... can't reach IItem& otherwise, since IItemDef puts IItem and
  /// OO... on separate multiple-inheritance branches (verified empirically:
  /// neither static_cast nor dynamic_cast can bridge that gap without this).
  template<typename Cfg,typename... OO>
  struct ItemDefC:APIOf<ItemAPI<Cfg>,OO...>{
    using Base=APIOf<ItemAPI<Cfg>,OO...>;
    using Base::Base;
    using Base::printMenu;
    using Base::enabled;
    using Base::print;

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path p) {
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

  /// @brief virtual-dispatch item interface — the item-side twin of out.h's
  /// IOut and nav.h's INav, same "escape hatch capped at one boundary"
  /// pattern all three share. This is a *cap*, not a cost that spreads: IItem
  /// sits at exactly the outer edge of IItemDef<II...> (below); the II...
  /// component chain inside it stays ordinary static HAPI composition, same
  /// zero-vtable machinery as plain ItemDef<OO...>. Nothing about being
  /// composed *inside* an IItemDef pays a virtual-dispatch tax internally —
  /// only code that actually needs a type-erased IItem&/IItem* (e.g.
  /// EventActionItem, for AM4-compat's OP() — see am4.h's am4compat::opItem,
  /// which reaches for IItemDef only when a handler's own signature asks for
  /// it) touches this boundary at all. Plain ItemDef stays the zero-cost
  /// default everywhere else; this is opt-in, not the norm.
  struct IItem {
    virtual bool printMenu(IOut& out,Ctx& ctx)=0;
    virtual bool printBody(IOut& out,Ctx&)=0;

    virtual bool enabled() const=0;
    virtual void enable(bool=true)=0;
    virtual bool changed() const=0;
    virtual void sync()=0;
    virtual void sync(IOut& out)=0;
    virtual bool up() const=0;
    virtual bool down() const=0;
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p)=0;
    virtual bool _nav(INav& n,const CKE& cke,const Path p)=0;

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path& path) {
      return isKbd?_kbdNav(n,cke,path):_nav(n,cke,path);
    }

    virtual void printItem(IOut& out,Ctx& ctx) {}

    /// @brief fake-no-op default (same shape as printItem() above) — overridden
    /// by IItemDef to forward into the real OO... chain's onEvent(); lets
    /// EventActionItem (below) hand a real item& to a user handler for items
    /// that opted into virtual dispatch, without every IItem user needing to
    /// override this.
    virtual bool onEvent(EventMask) {return false;}

    template <typename Out>
    static constexpr bool printMenu(Out& out,Ctx& ctx)
      {return printMenu(out,ctx);}

  };

  template<typename... II>
  struct IItemDef:IItem, ItemDefC<hapi::CRTP<IItemDef<II...>>,II...> {
    using Base=ItemDefC<hapi::CRTP<IItemDef<II...>>,II...>;
    using Base::Base;
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
    // visible, and a concrete OutDef<...> doesn't convert to IOut& (found by
    // finally composing an IItemDef into a body that's actually printed via
    // nav.printTo(), first time ever — the earlier standalone nav-only check
    // never exercised printing). The templated Base::printItem and the
    // virtual IOut&-typed one below are different signatures, so both stay
    // selectable — same non-colliding-overload shape as `nav` above.
    using Base::printItem;

    virtual bool printMenu(IOut& out,Ctx& ctx) override {return Base::printMenu(out,ctx);}
    virtual bool printBody(IOut& out,Ctx& ctx) override {return Base::printBody(out,ctx);}
    virtual void printItem(IOut& out,Ctx& ctx) override {Base::printItem(out,ctx);}
    virtual bool enabled() const override {return Base::enabled();}
    virtual void enable(bool o=true) override {return Base::enable(o);}
    virtual bool changed() const override {return Base::changed();}
    virtual void sync() override {Base::sync();}
    virtual void sync(IOut& out) override {Base::sync(out);}
    virtual bool up() const {return Base::up();};
    virtual bool down() const {return Base::down();};
    virtual bool _nav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<false>(n,cke,p);}
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<true>(n,cke,p);}
    virtual bool onEvent(EventMask e) override {return Base::onEvent(e);}
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

  /// @brief AM4-parity event handler: fires fn(raisedEvent) when EventDispatch (nav.h)
  /// raises any bit overlapping mask — mirrors AM4's navNode::event() exactly (masked
  /// "any overlap" match, handler receives the raised bit not the registered mask; see
  /// enums.h EventMask). Generalizes Action<fn> (which stays as the zero-cost
  /// Enter-only default, unmodified) rather than replacing it — use EventAction when you
  /// need Exit/Focus/Blur or a combined mask, Action for the plain Enter-only case.
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

  /// @brief AM4-parity event handler for the plain zero-arg void() handler shape — AM4's
  /// `action` class accepts several handler signatures (result(eventMask),
  /// result(eventMask,navNode&), result(eventMask,navNode&,prompt&) — menuBase.h ~169-171)
  /// via constructor overloads; a real AM4 sketch's FIELD/MENU handler is very commonly
  /// just `void fn()` though (e.g. neu-rah/Fielduino's `updateWave`). EventAction above
  /// needs `bool(EventMask)`; this sibling component exists so that exact shape can be
  /// used unmodified rather than requiring every ported handler to be rewritten — "other
  /// action components to match AM4 features" (Rui), not one signature trying to cover
  /// every AM4 handler shape.
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

  /// @brief AM4-parity event handler that also hands the item itself to fn —
  /// closest match to AM4's real result(eventMask,navNode&,prompt&) shape
  /// (the navNode& half isn't provided; see notes.md's open question on a
  /// nav-context variant). Requires the item be built as IItemDef<...>, not
  /// plain ItemDef<...>: IItem& is only reachable via IItemDef's own CRTP
  /// self-reference (ItemDefC's Cfg slot, see IItemDef above) — a plain
  /// ItemDef<...> has no such anchor and no IItem base to cast to. This is
  /// the vtable-cost sibling of EventAction: use EventAction for the
  /// zero-cost case, this one only where AM4-compat needs the item reference
  /// (e.g. am4.h's OP()) and the escape-hatch cost is already accepted.
  using EventFuncItem=bool(&)(EventMask,IItem&);

  template<EventMask mask,EventFuncItem fn>
  struct EventActionItem {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      bool onEvent(EventMask e) {
        bool r=(e&mask)?fn(e,static_cast<IItem&>(Base::obj())):false;
        return Base::onEvent(e)||r;
      }
    };
  };

  /// @brief scrolling/marquee text for a long item label — animates only while the item
  /// is focused (matches AM22's `TextRoller`/`TextRoll`, the one working prior-art
  /// implementation across the whole AM4/AM5/AM22-AM26 lineage — see notes.md
  /// "Animation"). Wraps a text component (StaticText, MultiLangText, ...) that provides
  /// get(): const char* — same "measure then decide how to print" shape as
  /// oneData::Decimals, not a plain pass-through decorator.
  ///
  /// @tparam cps scroll speed in characters per second (advance one character every
  ///             1000/cps ms, via OneChip's hw::Period — see clock.h)
  /// @tparam pad right margin, in characters, reserved when scrolling (space before wrap)
  ///
  /// Behavior, only decided by (a) whether the text overflows the available print width
  /// and (b) whether this exact instance currently has focus (`ctx`'s own bool operator —
  /// same "is this the selected item" check GfxFmt::itemInverted already uses):
  ///   - fits (len<=free width): plain full print, always, focused or not.
  ///   - overflows + focused: scroll a `rollPos`-relative window, ticker-tape wrap at the
  ///     end (one trailing space then restart from character 0), same shape as AM22's.
  ///   - overflows + not focused: static clip to the available width, no animation —
  ///     avoids corrupting layout on outputs with no line-wrap of their own.
  /// rollPos is genuine per-item state (not the shared/static state AM22 used) — a
  /// blurred item simply stops advancing (tick() is only ever dispatched to the
  /// currently-focused item by TickFocus, nav.h) and resumes mid-scroll on refocus,
  /// deliberately not reset — same "no reset on blur" behavior notes.md's research found
  /// in AM22, just achieved by dispatch instead of a shared static.
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
      bool changed() const {return Base::changed()||m_ticked;}
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

  /// @brief oneMenu's own extension of oneData::Hidden<II...> (oneData.h) — the
  /// print()/printItem() suppress-and-redirect logic itself now lives there (pure
  /// blind forwarding, generic on Ctx, no oneMenu dependency needed to compile or
  /// test it standalone). This layer derives Part<I> from it and adds exactly the
  /// two pieces that genuinely need real oneMenu semantics, not just a type name:
  ///  - printHidden(): pull-based "render II... on demand" (secondary/footer-device
  ///    content) — its `if(!ctx)` relies on oneMenu::Ctx::operator bool() specifically
  ///    (base.h: true iff this call targets the currently-focused path), a real
  ///    navigation concept, not an opaque passthrough like print()/printItem() are.
  ///  - nav(): routes Cmd/CKE navigation to I, oneMenu's own concrete CKE/Path types.
  template<typename... II>
  struct Hidden {
    template<typename I>
    struct Part:oneData::Hidden<II...>::template Part<I> {
      using Base=typename oneData::Hidden<II...>::template Part<I>;
      using Base::Base;
      // render II... only — Base inherits from Chain<II...,End>::Part<I> (oneData.h);
      // End stops before I, so this is the only path that actually reaches II...'s
      // own printItem.
      template<typename Out>
      void printHidden(Out& out,Ctx& ctx) {
        if(!ctx) return;
        static_cast<Base&>(*this).printItem(out,ctx);
      }
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p)
        {return I::template nav<isKbd>(n,cke,p);}
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
      using Type=typename Base::Type;
      constexpr bool valid() const {return Base::valid(get());}
      constexpr bool clamp() {return Base::set(Base::clamp(get()));}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path) {
        if(n.navMode()==NavMode::Edit) switch(cke.cmd){
          case Cmd::Up: Base::down(); return true;
          case Cmd::Down: Base::up(); return true;
          default: break;
        }
        return Base::template nav<isKbd>(n,cke,path);
      }
    };
  };

  /// @brief store and restore navigation position
  struct RecallNavPos {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<typename... OO>
      constexpr Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        Base::printItem(out,ctx);
        out.setPos(out.getPos());
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

  /// @brief syncs the selected choice's EnumValue<V> out to an external storage W on Enter —
  /// enum fields (Select/Choose/Toggle) otherwise only ever hold their own m_sel index
  /// (RecallNavPos), with no equivalent to how NumField's value can be owned or externally
  /// referenced/composed. W follows the same Data-chain shape as everywhere else — Data<T>
  /// for an owned member, DataRef<&ext>/DataFn<Src> for external — so SyncValue<Data<T>>,
  /// SyncValue<DataRef<&ext>>, SyncValue<DataFn<Src>> are all equally valid.
  /// Place above a RecallNavPos-based field (SelectFieldDef/ChooseFieldDef/ToggleFieldDef).
  template<typename W>
  struct SyncValue {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      using Storage = typename W::template Part<oneData::DataAPI<>>;
      Storage m_value{};

      auto value() const noexcept { return m_value.get(); }

      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r = Base::template nav<isKbd>(n,cke,path);
        if(cke.cmd==Cmd::Enter) Base::visit([this](auto& item){ m_value.set(item.value()); });
        return r;
      }
    };
  };

  struct ItemNav; // forward — needed by ParentDraw::rules()

  struct ParentDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<RecallNavPos>, After>,
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

  /// @brief alternative representation for a value
  /// @tparam Title title type
  /// @tparam Value value type
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
      static constexpr bool changed() {return ref.changed();}
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

  /// @brief compile-time item position: printItem moves cursor to (x,y), renders,
  /// then restores the original position so the next sequential item isn't displaced.
  template<Sz x,Sz y>
  struct Liquid {
    template<typename I>
    struct Part:I {
      using Base=I;
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

  /// @brief item claims the rest of the current page: after printing its own content it
  /// pads down to the bottom of the free area (Cursor::clearFree()) so the enclosing
  /// body's per-item free().y accounting sees this item as having consumed the whole
  /// page. Requires ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — its
  /// scroll-to-fit loop is what turns "this item eats all remaining room" into
  /// single-item-per-screen paging as the selection moves; under a non-scrolling body
  /// (FullPrinter/NoTitlePrinter) this would just pad trailing blank lines with no
  /// paging effect. Unlike Liquid/LiquidPos this never jumps the cursor backward — it
  /// only pads forward — so it doesn't corrupt ScrollBodyPrinter's sequential position
  /// measurement the way a decal that warps away and back would.
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
          // big row under-reserves by one and overshoots — see notes.md for the
          // FullScreen big-font bug this fixes.
          if constexpr(hapi::query<IsFillRect,typename Out::Types>) {
            // GFX outputs (HasFillRect via aFillRect, e.g. OledOut): one native-coord rect
            // fill instead of a clearToEOL()+nl() per row. free().y here still includes the
            // current (just-printed, not yet closed) row — fill only what's *below* it
            // (p.y+L onward), never the row itself: text was drawn there via
            // Base::printItem(out,ctx) just above, and fillRect must never re-touch pixels
            // the item's own content already owns (see notes.md/
            // [[project_fullscreen_nav_redraw_bug]] bug #3 — fillRect starting *at* p.y
            // instead of p.y+L silently painted over the just-drawn text on real hardware).
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

  /// @brief horizontal text alignment (Left/Center/Right) tag/grouping — no rendering logic of
  /// its own. Align used to carry its own measure-then-pad-then-print implementation (a
  /// standalone dry-run against the full out.free()), but that's now Row's job exclusively —
  /// having two copies of the same algorithm was duplicate code for no benefit, and Align's own
  /// version couldn't coordinate with siblings sharing a line the way Row does. Whoever wants
  /// aligned content composes Row directly (Row<Left,Center,Right>) — Align just groups/labels
  /// its II... the same way Hidden<II...>/Decor<II...> do (hapi::Chain<II...> wrapped below),
  /// e.g. AlignCenter<Watch<Default<Int,0>>>, as a plain marker of intent. An Align used on its
  /// own (nothing routing it through Row) just prints its content plain/unaligned — it's a tag,
  /// not a printer.
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

  /// @brief coordinates 3 independently-printable items on one line: Left prints at the current
  /// position, Right ends flush with the far right edge, Center fills the *gap* between them —
  /// not the full line width. Unlike Align (which only knows its own content vs whatever's left
  /// after prior siblings), Row measures all three up front (LockMode::Measure dry runs, no
  /// printing) before printing any of them for real, so Center's position correctly accounts for
  /// Right's width too — the thing a plain Chain<Text,AlignCenter<Text>,AlignRight<Text>> can't
  /// do, since each Align only ever sees out.free() *at the moment it runs*, never what a later
  /// sibling will still consume.
  /// Left/Center/Right must each be a standalone printable+constructible type (print(out) const,
  /// printItem(out,ctx)) — typically an ItemDef<...>. nav()/setStr()/changed()/sync()/etc. are
  /// forwarded to Center (2026-07-06 — see Part below) so an editable field placed there works
  /// correctly, matching CenterRow's own established convention (Left/Right empty, real content
  /// in Center); Left/Right stay structural/display-only.
  /// Adequate for small (single-line) devices; a fuller layout (rows of Rows, VAlign, etc.) can
  /// be built on top later.
  template<typename Left,typename Center,typename Right>
  struct Row {
    template<typename I>
    struct Part:I {
      using Base=I;
      Left   leftItem{};
      Center centerItem{};
      Right  rightItem{};

      // Forward the interactive surface to centerItem — same fix, same reasoning, as
      // Rows's forwarding to bodyItem below: Left/Right are structural (matching
      // CenterRow's own established convention of empty Left/Right placeholders, real
      // content always in Center), and without this an editable field placed in Center
      // would silently never receive nav()/setStr() (feedback_row_breaks_field_editing),
      // NOR would its own changed() reach the outer redraw-gating — the latter isn't just
      // cosmetic: TreeNav::changed(out) only re-probes the print tree when nav-level state
      // (level/navMode/selection) is unchanged, which is exactly the case for adjusting an
      // already-open field's value via up/down without also toggling edit mode — without
      // this forward, that value change would never be detected as needing a redraw.
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {return centerItem.template nav<isKbd>(n,cke,p);}
      template<typename Nav,typename P>
      bool setStr(Nav& n,const char* s,P p) {return centerItem.setStr(n,s,p);}
      bool changed() const {return centerItem.changed();}
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
            // idiom as FullScreen's own fillRect (see notes.md's FullScreen bug writeup),
            // rather than approximating position via padWith(N space glyphs), which floors
            // to whole-glyph increments and can leave up to one glyph-width of visible
            // off-center error on a real pixel-addressable device.
            Pos start=out.getPos();
            out.fillRect(start.x, start.y, lineW, out.lineHeight());
            // fillRect moves the *driver's own* internal cursor to the far edge of
            // whatever it just filled (Ssd1306::fillRect, OneIO) — separate from the
            // logical Cursor's own m_at, which fillRect never touches. Resync explicitly
            // before leftFn() rather than assume it's still at `start` (found the hard
            // way — see gfxFmt.h's roleBig-tagged fmtStart comment for the same bug in a
            // different spot). Currently harmless here since every real Left is empty,
            // but latent otherwise.
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

  /// @brief vertical layout: Top, then Body anchored (Left=top/Center/Right=bottom) within
  /// whatever free vertical space remains after Bottom's `bottomLines` are reserved — a
  /// header+body+footer stack where Body's own placement in the free space is a **vertical
  /// Align, applied to lines the same way Align/Row apply to characters on one line**. Top
  /// and Bottom are structural (a free-space partition: how much room each edge gets),
  /// exactly mirroring Row's Left/Right; `vMethod` is the internal-alignment half — unlike
  /// horizontal Align (which has a coherent standalone no-op meaning: "print, wherever the
  /// cursor is"), vertical anchoring is meaningless without knowing the region's free height
  /// vs Body's own height, so it can't be a separable tag the way Align is — it only exists
  /// as a mode here, on the thing that actually partitions the space.
  /// vMethod==Left needs no measurement (print Body immediately, same as today); Center/Right
  /// need Body's own line count first, via a LockMode::Measure dry run — the vertical twin of
  /// Row::measure, over lines instead of pixel width. Bottom's `bottomLines` stays a compile-
  /// time NTTP (static: usually known up front); a fully dynamic sibling (Bottom's height also
  /// measured at runtime) is the natural next tier if that stops being true.
  /// On non-cursor (streaming) devices neither the footer-pin nor vMethod mean anything (no
  /// known height to measure against), so Rows falls back to plain sequential printing — same
  /// fallback Row uses for its own horizontal centering.
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
      // "display-only, editing silently doesn't work" restriction already documented for
      // Row's Left/Center/Right (feedback_row_breaks_field_editing) — fixed here rather
      // than left as a limitation, since a real editable field vertically laid out under
      // its own label is the actual motivating case (see notes.md "lolin32 demo styling").
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {return bodyItem.template nav<isKbd>(n,cke,p);}
      template<typename Nav,typename P>
      bool setStr(Nav& n,const char* s,P p) {return bodyItem.setStr(n,s,p);}
      bool changed() const {return bodyItem.changed();}
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
          // own padding (item.h, printed-with-one-fillRect-not-a-clearLine-loop).
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
  /// @brief marks an item as belonging to output partition Tag.
  /// - SkipOutId<Tag> in the main OutDef skips these items on the main output.
  /// - PartitionBody<Tag,Body> renders only these items to a secondary output.
  /// - Items are nav-invisible when placed after the body's navSize() boundary.
  template<typename Tag>
  struct OutId {
    template<typename O> using Part=O;  // pure tag: zero behavior, queryable via SameAs<OutId<Tag>>
  };

};//namespace oneMenu

//rules ItemDef query specialization --
template<typename Q,typename... OO>
constexpr const bool hapi::template query<Q,oneMenu::template ItemDef<OO...>>{(hapi::template query<Q,OO>||...)};

