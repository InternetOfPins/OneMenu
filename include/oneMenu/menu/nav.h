/**
 * @file nav.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief menu navigation API
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

namespace oneMenu {

  template<typename N> struct NavAPI:N {};

  template <typename API,typename... NN>
  struct DefinedNav:APIOf<API, NN...> {
    using Base = APIOf<API, NN...>;
    // Forwards whatever constructor APIOf ends up exposing (itself forwarded, link by
    // link via each hapi::Chain::Part's own `using Base::Base;`, from the *one* stateful
    // NN... link — if any — that declares its own constructor; see nav.h's Pool for the
    // motivating case). Every existing NN... component stays purely default-constructible
    // today, so this only *adds* reachability, it changes nothing for current chains.
    using Base::Base;
    bool up() {return Base::template doCmd<false>(Cmd::Up);}
    bool down() {return Base::template doCmd<false>(Cmd::Down);}
    bool enter() {return Base::template doCmd<false>(Cmd::Enter);}
    bool esc() {return Base::template doCmd<false>(Cmd::Esc);}

    // AM4's poll(){doInput();doOutput();} half: self-gating output step — redraws (and syncs)
    // only if something visible actually changed, else a no-op. Callers needing to react to
    // *why* it redrew (e.g. gating a secondary output device on position vs. value changes)
    // should check Base::changed() themselves before calling this, since sync() below clears
    // the flags changed() reads.
    template<typename Out>
    bool doOutput(Out& out) {
      if(!Base::changed(out)) return false;
      // A level change (open/close a submenu — Select/Choose entering their choice list,
      // or Esc/close backing out) swaps in a whole different, unrelated set of items at the
      // same rows. The normal Update-mode pass only force-unlocks items whose ctx.idx equals
      // the old or new *selection index* (see ItemPrinter::printItem) — it has no idea the
      // page's entire content changed, so it keeps skipping/leaving stale rows from the other
      // level. changed() itself must stay a pure query (see its own comment), so the forced
      // full redraw belongs here, the actual output-driving step.
      if(Base::levelChanged()) {
        out.lockMode(LockMode::None);
        out.clear();
      }
      Base::printTo(out);
      Base::sync(out);
      // printTo (via ScrollBodyPrinter) may have forced lockMode to None for a scroll — nothing
      // downstream restores it, and sync(out) itself just puts back whatever lockMode was at its
      // own entry (still None in that case). Left alone, every subsequent doOutput() call keeps
      // running a full unconditional redraw forever (visible as permanent flicker once any scroll
      // has happened) instead of resting at the normal per-item-selective Update mode. Every other
      // caller that forces None for a manual full redraw (setup(), idleRun(), action::ok() in the
      // demo) explicitly restores Update afterward — doOutput() is the one path that didn't.
      //
      // But Update mode itself ("draw only changed") only makes sense for a device that can
      // actually overwrite just the changed part — a device with no real partial-draw capability
      // (no PartialDraw in its chain, e.g. a plain non-ANSI serial stream) can only ever append,
      // so "selective" redraw there just means silently dropping everything except the one
      // changed item instead of showing the whole menu. Gate to PartialDraw capability instead
      // of assuming every device supports it.
      out.lockMode(hapi::TagIs<PartialDraw>::Check<Out>::value ? LockMode::Update : LockMode::None);
      return true;
    }
  };

  /// @brief compose a navigation chain from nav components (TreeNav, Root, IndexGo, etc.)
  template<typename... II>
  struct NavDef:DefinedNav<NavAPI<hapi::CRTP<NavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<NavDef<II...>>>,II...>;
    using Base::Base;
  };

  // Declared here (rather than next to RunLoop, further down) because
  // INav::idleOn's signature needs AltRunFn.
  using RunFn = bool(&)();
  using AltRunFn = bool(*)();

  /// @brief virtual-dispatch nav interface — the nav-side twin of item.h's
  /// IItem and out.h's IOut, same "escape hatch capped at one boundary"
  /// pattern all three share. level()/sel()/navMode() are pure virtual:
  /// every real INavDef<...> chain composes TreeNav, so INavDef can always
  /// forward them. idling()/idleOn()/idleOff() default to a total no-op —
  /// core INav doesn't know or care whether any concrete nav chain is bound
  /// to a real RunLoop<mainFn> (below, this file) — that's AM4-flavored
  /// wiring for a *particular* sketch's mainFn (AM4-port machinery stays
  /// AM5/compat-side unless it brings value to OneMenu itself — RunLoop
  /// itself already does, this binding doesn't need to). Plain INavDef
  /// therefore compiles unchanged with zero new
  /// obligation; only am4compat::NavRootDef (am4.h) overrides these three
  /// to forward into a real RunLoop<mainFn> — see that type's own doc
  /// comment.
  struct INav {
    virtual Depth level() const=0;
    virtual Sz sel() const=0;
    virtual NavMode navMode() const=0;

    /// @brief AM4 nav.root->navFocus-equivalent: "is this nav still the one
    /// actually in control." OneMenu has one INav per nav chain (no AM4
    /// -style multiple-navRoot registry), so this collapses onto "not
    /// currently backgrounded by an idle/dialog alternative" — derives
    /// from idling() alone, nothing separate to override.
    bool isFocused() const {return !idling();}

    virtual bool idling() const {return false;}
    virtual void idleOn(AltRunFn) {}
    virtual void idleOff() {}

    /// @brief level-mutating primitives (TreeNav::Part's own open/close/
    /// padOpen/doNav, above) — the ONLY way a nav chain's selection/depth
    /// actually changes. Not pure: a bare INav with no real TreeNav behind
    /// it (there is no such thing today, but a future non-tree nav shape
    /// might not have levels at all) safely no-ops rather than forcing every
    /// INav-family type to implement level bookkeeping it doesn't have.
    /// Needed so a type-erased item (IItem::_nav/_kbdNav, item.h — takes
    /// INav&, not a concrete TreeNav type) can itself open/close a level or
    /// move a selection, e.g. a virtual-dispatch item wrapping its own
    /// Menu-shaped/submenu body. Same "escape hatch capped at one boundary"
    /// pattern as idling()/idleOn()/idleOff() just above — purely additive,
    /// every existing INavDef<...> chain keeps compiling unchanged.
    virtual bool open() {return false;}
    virtual bool close() {return false;}
    virtual bool padOpen() {return false;}
    virtual bool doNav(CKE,Sz,bool) {return false;}
  };

  template<typename... II>
  struct INavDef:INav,DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...>;
    using Base::Base;
    // navMode() is overloaded on Base (TreeNav::Part: a getter AND a
    // setter, navMode(NavMode)) — declaring the getter override below would
    // otherwise hide the whole overload set by name (ordinary C++ name
    // hiding, same rule TickFocus's own comment already documents for
    // changed()), breaking every navMode(NavMode) setter call reached
    // through an INavDef&/INav& (e.g. EditField::Part::nav(), item.h).
    using Base::navMode;
    Depth level() const override {return Base::level();}
    Sz sel() const override {return Base::sel();}
    NavMode navMode() const override {return Base::navMode();}
    // idling()/idleOn()/idleOff() deliberately NOT overridden here — the
    // inherited INav no-op default stays; am4compat::NavRootDef (am4.h) is
    // the type that binds them to a real RunLoop<mainFn>, not this one.

    // open()/close()/padOpen()/doNav() DO forward here — unlike idling()'s
    // family above, every real INavDef<...> chain composes TreeNav (or
    // IndexGo/EventDispatch above it), which already implements these for
    // real; there's no "AM4-flavored, sketch-specific" axis to defer here.
    bool open() override {return Base::open();}
    bool close() override {return Base::close();}
    bool padOpen() override {return Base::padOpen();}
    bool doNav(CKE cke,Sz len,bool w) override {return Base::doNav(cke,len,w);}
  };

  /// @brief binds a nav chain to an external menu instance as the navigation root
  template<typename T,T& menu>
  struct Root {
    template<typename N>
    struct Part:N {
      using Base=N;
      using Root=std::remove_reference_t<decltype(menu)>;
      static constexpr Root& root() {return menu;}
      static constexpr const Depth depth() {return Root::depth();}
    };
  };

  /// @brief embeds a menu instance directly inside the nav object (value ownership)
  template<typename M>
  struct StaticRoot {
    template<typename N>
    struct Part:N {
      using Base=N;
      M m_menu;
      inline constexpr M& root() {return m_menu;}
      static constexpr const Depth depth() {return M::depth();}
    };
  };

  /// @brief fuses input+output for one nav: the small "poll everything, redraw if
  ///        changed" cycle every sketch otherwise hand-writes itself in loop()/run().
  ///        Composes like any other nav component instead of needing its own
  ///        separately-built Nav type to derive from.
  ///
  ///        Deliberately generic over InG/OutG, NOT type-erased (IIn&/IOut&) —
  ///        m_in.doInput(...)/m_out.doOutput(...) are both template calls, so
  ///        plugging in plain compile-time InDef<...>/OutDef<...> costs nothing
  ///        extra, same as composing them directly. A device set that's
  ///        heterogeneous but still fixed at compile time (AM4-compat's
  ///        MENU_INPUTS/MENU_OUTPUTS) uses InGroup/OutGroup (in.h/out.h) —
  ///        also zero vtable cost, a compile-time pack that keeps every
  ///        device's own concrete type (needed: Menu::Part::printMenu is
  ///        templated on the item type, so it can't cross a type-erased IOut&
  ///        boundary at all). InList<N>/OutList<N> (genuinely runtime-picked/
  ///        hot-swappable devices) stay usable as InG, but OutList can't
  ///        drive a real menu tree for the same reason — see its own comment
  ///        (out.h).
  ///
  ///        Deliberately narrow: owns just this one fan-out+redraw fusion, not
  ///        idle timers or which nav is "active" (examples/fields' activeRun
  ///        state machine already does that, unaffected — Pool doesn't try to
  ///        replace it, just gives it a reusable primitive to build multiple
  ///        Pool instances from instead of hand-rolling each one's cycle).
  ///
  ///        Must be the FIRST component in the chain (`INavDef<Pool<...>,
  ///        EventDispatch, TreeNav, Root<...>> id(in,out);`) for its constructor
  ///        to be reachable: hapi::Chain forwards a component's own constructor
  ///        upward via `using Base::Base;` at every wrapping layer (same
  ///        mechanism Menu<T,B,OO...>::Part's 2-arg constructor already relies
  ///        on), but only the outermost link's constructor is what a plain
  ///        `using Base::Base;` two layers up (DefinedNav/INavDef, this file)
  ///        actually exposes at the top.
  template<typename InG,typename OutG>
  struct Pool {
    template<typename N>
    struct Part:N {
      using Base=N;
      InG& m_in;
      OutG& m_out;
      Part(InG& i,OutG& o):m_in(i),m_out(o) {}
      // m_out.doOutput(Base::obj()) — the OutG side drives Nav::doOutput, not
      // the other way around. OutDef/IOutDef/OutGroup (out.h) each provide
      // their own doOutput(Nav&) one-liner-or-loop that internally calls
      // nav.doOutput(*device) with the device's own concrete type intact —
      // required, not stylistic: Menu::Part::printMenu (menu.h) needs that
      // concrete type to run the templated print walk, and Base::obj() is
      // exactly what supplies "the fully-assembled Nav" as that argument.
      // Base::obj() itself (hapi::CRTP, reachable via ordinary inheritance
      // from the API terminal every Part in the chain derives from) is
      // needed here rather than `*this` alone for a *different*, unrelated
      // reason: doOutput() lives on DefinedNav, which *wraps* the whole
      // component chain from outside (nav.h, top of file), not inside it —
      // Base=N only reaches downward into the rest of the II... pack, so a
      // plain `*this` call would statically bind to this scope and miss
      // doOutput entirely. obj() casts to the fully-assembled
      // type, which *does* inherit doOutput, so `nav.doOutput(*device)`
      // inside OutG's own doOutput(Nav&) resolves correctly once Base::obj()
      // is what gets passed in as `nav`.
      bool poll(Sz maxCount=8) {
        bool i=m_in.doInput(*this,maxCount);
        bool o=m_out.doOutput(Base::obj());
        return i||o;
      }
    };
  };

  /// @brief hierarchical tree navigator: tracks path, level, selection, and scroll position
  struct TreeNav {
    template<typename N>
    struct Part:N {
      using Base=N;
      using Root=typename Base::Root;
      using Base::root;
      using Base::depth;

      Path path() {return m_path;}
      Path focus(Sz i) {return m_path.focusAt(i);}
      Depth level() const {return m_level;}
      Sz sel() const {return m_path[m_level];}
      /// @brief selection index at an arbitrary depth (not just the current level) —
      /// needed by EventDispatch to reach items inside nested submenus.
      Sz pathSel(Depth d) const {return m_path[d];}

      void navMode(NavMode m) {m_navMode.set(m);}
      const NavMode navMode() const {return m_navMode.get();}

      // entering/leaving a level swaps the whole displayed page for unrelated content (a
      // submenu's items don't correspond 1:1 with the parent's) — doOutput() needs this to
      // force a real clear+full redraw instead of the normal per-item selective Update pass.
      bool levelChanged() const {return m_level.changed();}

      void sync() {
        m_level.sync();
        m_navMode.sync();
        m_prevSel=sel();
      }

      template<typename Out>
      void sync(Out& out) {
        sync();
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Sync);
        printTo(out);
        out.lockMode(om);
      }

      bool changed() const {
        return m_level.changed()
          ||m_navMode.changed()
          ||m_prevSel!=sel();//however items will check later if the are on focus or just blur (Ctx sel and prev. sel) and signal a draw (on update)
      }

      template<typename Out>
      bool changed(Out& out) {
        if(changed()) return true;
        // changed() must never modify the output — it has to remain faithful to the print process
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Changed);
        bool r=printTo(out);    // probe: Gate suppresses all hardware, m_at drift is harmless
        out.lockMode(om);
        return r;
      }

      template<typename Out>
      bool printTo(Out& out) {
        // Re-anchor to the declared origin before each frame — printMenu()/TitlePrinter's
        // own fmtStop only ever call a *relative* nl(), assuming the cursor already sits at
        // (orgX,orgY). That's true after a plain sequential frame, but a FullScreen item
        // (item.h) deliberately pads all the way to the *bottom* of the page, so the next
        // frame would otherwise inherit that stale bottom position as its baseline — one
        // relative nl() from there lands nowhere near row 1, and everything downstream
        // (ScrollBodyPrinter's own anchor capture included) silently corrupts from there.
        // resume() alone doesn't fix this: it re-syncs the *physical* device to whatever
        // the *logical* position (m_at) already is — useless when m_at itself is wrong.
        if constexpr(hapi::query<IsCursor,typename Out::Types>) out.setPos({out.orgX(),out.orgY()});
        ///track scroll top for each level, this is output device specific
        static Sz tops[root().depth()]{0};
        Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,tops,0,m_prevSel};
        bool r=root().printMenu(out,ctx);
        out.flush();
        return r;
      }

      template<bool isKbd>
      bool doCmd(Cmd cmd,Key k=0, bool e=false) {
        CKE cke{cmd,k,e};
        bool r=root().template nav<isKbd>(Base::obj(),cke,focus(m_level+1));
        if(!r&&cmd==Cmd::Esc) return close();  // items get first chance; close only if unhandled
        return r;
      }

      // aux function for items to call and complete a navigation request with default actions
      // Data<Sz&> would dangle: Data constructor takes by-value, binds reference to local.
      // Use Data<Sz> (owned copy) + writeback through the actual sel reference.
      bool doNav(CKE cke,Sz len,bool w) {
        // dout<<colors<GREEN,BLACK><<" len:"<<len<<" wraps:"<<w;
        Sz& sel=m_path.data[(int)level()];
        oneData::DataDef<NumRange<Sz>,oneData::Data<Sz>> at(0,len-1,w,sel);
        switch(cke.cmd) {
          case Cmd::Up:
          case Cmd::Left:  at.up();   break;
          case Cmd::Down:
          case Cmd::Right: at.down(); break;
          default: return false;
        }
        sel=at.get();
        return true;
      }

      template<typename In>
      bool in(In& in) {
        CKE cke = in.cmd();
        if (cke.cmd == Cmd::None) return false;
        // Base::obj() (hapi::CRTP) routes through the fully-assembled chain type instead
        // of a bare doCmd() call statically bound to TreeNav::Part's own scope — lets a
        // more-derived doCmd() override (e.g. EventDispatch, composed above TreeNav) see
        // real input-driven nav.in()/poll() calls, not just direct nav.enter()/etc. calls.
        // See EventDispatch's own doc comment.
        return cke.kbd ? Base::obj().template doCmd<true> (cke.cmd, cke.key, cke.ext)
                       : Base::obj().template doCmd<false>(cke.cmd, cke.key, cke.ext);
      }

      void go(Sz i,Depth delta=0) {
        assert(m_level+delta<depth());
        m_path.data[m_level+delta]=i;
      }

      bool padOpen() {
        if(m_level.get()<depth()) {
          m_level.set(m_level+1);
          m_path.data[m_level]=0;
          if(m_level.get()<m_print_level) m_print_level=m_level;
          return true;
        } else return false;
      }
      bool open() {
        if(padOpen()) {
          m_print_level++;//=m_level;
          return true;
        } else return false;
      }
      
      bool close() {
        navMode(NavMode::Nav);
        if(m_level) {
          m_level.set(m_level-1);
          if(m_print_level>m_level) m_print_level=m_level;
          return true;
        } else return false;
      }

    protected: 
      Sz m_prevSel{};
      PathData<depth()+1> m_path{};
      oneData::DataDef<Watch<oneData::Data<Depth>>> m_level{0};
      Depth m_print_level{0};
      oneData::DataDef<Watch<oneData::Data<NavMode>>> m_navMode{NavMode::Nav};
    };
  };

  // Handles Cmd::Go from IdParser: jumps to item N at current level then enters it.
  // Place above TreeNav in the nav chain:  NavDef<IndexGo, TreeNav, Root<...>>
  // NOTE: must override in(), not doCmd() — TreeNav::Part::in() uses static dispatch
  // for doCmd and cannot reach a more-derived doCmd override without CRTP. Its own
  // internal doCmd calls route through Base::obj() (hapi::CRTP) for the same reason —
  // so a more-derived doCmd() override (e.g. EventDispatch, composed above IndexGo) is
  // still reached even though it dispatches from here, not from TreeNav::Part::in().
  // While a field is being edited (NavMode::Edit), digit keys are redelivered as a
  // literal Cmd::Key instead of jumping to item N — see idParser.h/item.h's NumField
  // for the other two pieces of this.
  /// @brief nav component that handles Cmd::Go: jumps to item N at the current level by index
  struct IndexGo {
    template<typename N>
    struct Part : N {
      using Base = N;
      template<typename In>
      bool in(In& src) {
        CKE cke = src.cmd();
        if (cke.cmd == Cmd::None) return false;
        if (cke.cmd == Cmd::Go) {
          // Typing a digit while editing a field must deliver it as a
          // literal Cmd::Key, not jump to item N — IdParser has no nav
          // context to gate this itself (see idParser.h), so the redirect
          // lives here, the first point in the chain with navMode() available.
          if (Base::navMode() == NavMode::Edit)
            return Base::obj().template doCmd<true>(Cmd::Key, Key('0' + cke.key), false);
          Base::go(Sz(cke.key) - 1);  // IdParser emits 1-based; go() is 0-based
          return Base::obj().template doCmd<false>(Cmd::Enter);
        }
        // '0' is tagged (not encoded as Cmd::Go — see idParser.h); only
        // reinterpret it here, and only while editing, so every other
        // Cmd::Esc (a real Escape key, or '0' outside edit mode) falls
        // through unchanged below, identical to today's behavior.
        if (cke.cmd == Cmd::Esc && cke.key == Key('0') && Base::navMode() == NavMode::Edit)
          return Base::obj().template doCmd<true>(Cmd::Key, Key('0'), false);
        return cke.kbd ? Base::obj().template doCmd<true> (cke.cmd, cke.key, cke.ext)
                       : Base::obj().template doCmd<false>(cke.cmd, cke.key, cke.ext);
      }
    };
  };

  /// @brief AM4-parity event dispatch: detects nav-level state transitions (selection
  /// and level changes) and raises EventMask events to the affected item's onEvent()
  /// hook (item.h). Place above TreeNav: NavDef<EventDispatch, TreeNav, Root<...>>.
  ///
  /// v1 scope: only Enter/Exit/Focus/Blur are raised; selFocus/selBlur/updateEvent/
  /// activateEvent are real AM4 features not implemented here yet.
  /// - Dispatches at any level, walking root().body down through the live path to reach
  ///   items inside nested submenus (see detail::eventVisit below).
  /// - Wraps doCmd(), NOT in() — real input-device-driven nav.in()/poll() calls (and
  ///   IndexGo's own in(), when composed) route through Base::obj() (hapi::CRTP) before
  ///   calling doCmd, specifically so a more-derived doCmd() override like this one is
  ///   still reached from those call sites — see TreeNav::Part::in()/IndexGo::Part::in().
  /// - Enter/Exit fire regardless of whether they also changed level, matching AM4.
  /// - Enter/Exit are gated on the target item's enabled() (AM4 parity); Focus/Blur are
  ///   not — a disabled item is still selectable (Menu::Part::nav's plain Up/Down never
  ///   consults enabled() at all), so gating Focus/Blur too would suppress an event kind
  ///   the rest of the codebase treats as enabled-agnostic.
  namespace detail {
    // Detects "does this item have a nested .body" (i.e. it's an ItemDef<Menu<...>>) —
    // std::void_t/declval come from HAPI's avr_std.h shim on AVR (no <type_traits> there
    // at all), from real <type_traits> elsewhere.
    template<typename T, typename = void> struct HasBody : std::false_type {};
    template<typename T> struct HasBody<T, std::void_t<decltype(std::declval<T&>().body)>> : std::true_type {};

    // True when Nav's own obj() (hapi::CRTP, reached via ordinary inheritance
    // from the terminal API at the bottom of Nav's own component chain)
    // resolves to something that IS-A INav. True for every real INavDef<...>/
    // am4compat::NavRootDef<...> chain; false for a plain NavDef<...> chain,
    // which has no INav anywhere in it. Gates whether fireAt's dispatch even
    // attempts the nav-carrying path — EventDispatch must keep compiling
    // unchanged under a plain NavDef<...>.
    template<typename Nav, typename = void> struct ObjIsINav : std::false_type {};
    template<typename Nav> struct ObjIsINav<Nav, std::void_t<decltype(std::declval<Nav&>().obj())>>
      : std::is_base_of<INav, std::remove_reference_t<decltype(std::declval<Nav&>().obj())>> {};

    // HasNavOnEvent lives in item.h (included before this file in the
    // aggregate, oneMenu.h) — it's fundamentally an item-side concern
    // (detects a real onEvent(EventMask,INav&) on the item's own OO...
    // chain), reused here unqualified via oneMenu::HasNavOnEvent, not
    // redefined. See its own doc comment there for the full rationale.
    template<typename Item, typename Nav>
    bool dispatch(Item& item, Nav& nav, EventMask e) {
      if constexpr (ObjIsINav<Nav>::value && HasNavOnEvent<Item>::value)
        return item.onEvent(e, static_cast<INav&>(nav.obj()));
      else
        return item.onEvent(e);
    }

    // Walks body down d=0..level (using nav's *current* pathSel(d) for every level
    // except the final one, where idx is used instead — the final level's selection may
    // be the old or new value depending on which event is being raised, not necessarily
    // what's live in the path right now) and invokes fn on whatever item is found there.
    template<typename Body, typename Nav, typename Fn>
    void eventVisit(Body& body, Nav& nav, Depth d, Depth level, Sz idx, Fn&& fn) {
      Sz i = (d==level) ? idx : nav.pathSel(d);
      body.visit(i, [&](auto& item) {
        if(d==level) fn(item, nav);
        else if constexpr (HasBody<std::decay_t<decltype(item)>>::value)
          eventVisit(item.body, nav, (Depth)(d+1), level, idx, std::forward<Fn>(fn));
      });
    }
  }

  struct EventDispatch {
    template<typename N>
    struct Part : N {
      using Base = N;
      template<typename Fn>
      void fireAt(Depth level, Sz idx, Fn&& fn) {
        detail::eventVisit(Base::root().body, static_cast<Base&>(*this), (Depth)0, level, idx, std::forward<Fn>(fn));
      }
      template<bool isKbd>
      bool doCmd(Cmd cmd, Key k=0, bool e=false) {
        Sz oldSel = Base::sel();
        Depth oldLevel = Base::level();
        bool r = Base::template doCmd<isKbd>(cmd, k, e);
        // NOTE: events fire on cmd *type* (Enter/Esc/index-change), not gated on r — a
        // plain item with no Action/submenu legitimately returns r=false for Enter, but
        // AM4's enterEvent fires unconditionally on Enter regardless of whether the item
        // does anything with it.
        Sz newSel = Base::sel();
        Depth newLevel = Base::level();
        if(newLevel==oldLevel && newSel!=oldSel) {
          fireAt(oldLevel, oldSel, [](auto& item, auto& nav){ detail::dispatch(item, nav, EventMask::Blur); });
          fireAt(oldLevel, newSel, [](auto& item, auto& nav){ detail::dispatch(item, nav, EventMask::Focus); });
        }
        if(cmd==Cmd::Enter) fireAt(oldLevel, oldSel, [](auto& item, auto& nav){ if(item.enabled()) detail::dispatch(item, nav, EventMask::Enter); });
        if(cmd==Cmd::Esc) {
          Depth targetLevel = newLevel<oldLevel ? newLevel : oldLevel;
          Sz targetIdx = newLevel<oldLevel ? newSel : oldSel;
          fireAt(targetLevel, targetIdx, [](auto& item, auto& nav){ if(item.enabled()) detail::dispatch(item, nav, EventMask::Exit); });
        }
        return r;
      }
    };
  };

  /// @brief the "alternative poll handler" pattern: a single active handler,
  /// swapped by idleOn()/idleOff() to show an idle screen, a dialog, or
  /// anything else that should take over the poll loop. `run()` always
  /// calls `alternative()` unconditionally — no null-check, since
  /// `alternative` always points at a valid function. "Restoring normal
  /// operation" means reassigning it back to `mainFn`, not to null —
  /// simpler than AM4's own internal `sleepTask`, which does use NULL+a
  /// check.
  ///
  /// `mainFn` is a compile-time NTTP (the common case never changes at
  /// runtime, so it costs nothing to fix at compile time — same convention
  /// as EventFunc/IdleFunc); the *alternative* is the one genuinely
  /// runtime-swappable slot, since which alternative runs (idle screen,
  /// which dialog, ...) is a real runtime decision. Each handler is just an
  /// arbitrary `bool()` function — it can drive the *same* nav, a
  /// completely separate nav+menu+device (AM4's own dialog pattern), or
  /// nothing nav-related at all. RunLoop itself doesn't know or care what a
  /// handler does — same "cap, not a spreading cost" shape as this
  /// codebase's other escape hatches: composing it costs one function
  /// pointer, nothing else.

  template<RunFn mainFn>
  struct RunLoop {
    static inline AltRunFn alternative = mainFn;
    /// @brief call every frame — runs whichever handler is currently active.
    static bool run() { return alternative(); }
    /// @brief AM4's `nav.idleOn(fn)` — swap in fn (idle screen, a dialog,
    /// anything) as what run() calls from now on.
    static void idleOn(AltRunFn fn) { alternative = fn; }
    /// @brief AM4-style "restore": reassign back to mainFn, not to null
    /// (see this type's own doc comment for why).
    static void idleOff() { alternative = mainFn; }
    static bool active() { return alternative != mainFn; }
  };

  /// @brief drives per-frame animation (AM22's `TickFocus`). Every output poll,
  /// dispatches tick() (item.h) to whichever item is currently focused —
  /// reusing EventDispatch's own detail::eventVisit walker, so composition
  /// order relative to EventDispatch doesn't matter, neither depends on the
  /// other.
  ///
  /// Overrides changed(Out&), NOT doOutput() — unlike IndexGo/EventDispatch (which
  /// override doCmd()/in(), methods DefinedNav actually delegates to), doOutput() is
  /// defined directly on DefinedNav itself (this file, above) and never calls
  /// Base::doOutput() at all, so a chain component overriding doOutput() would be
  /// silently unreachable (a plain override binds statically to this scope, not
  /// to whatever DefinedNav actually calls). changed(Out&) IS one of the primitives
  /// DefinedNav::doOutput actually composes from Base, so overriding it here correctly
  /// makes tick()-driven animation feed the same redraw decision a real data change would.
  /// Reports true if tick() advanced anything, in addition to (not instead of) whatever
  /// Base::changed(out) already found — an animating item's own changed() (item.h's
  /// TextRoll) must independently report true too, since Update-mode's per-row gate
  /// (printers.h's ItemPrinter) only unlocks a row when *that item's* changed() is true,
  /// not just the nav-level aggregate probed here.
  ///
  /// Place above TreeNav: NavDef<TickFocus, TreeNav, Root<...>>.
  struct TickFocus {
    template<typename N>
    struct Part : N {
      using Base = N;
      // Defining changed(Out&) here hides ALL of Base's changed overloads by name, not
      // just that one signature (ordinary C++ name hiding, not overload resolution) —
      // the no-arg changed() (nav.h's own TreeNav::Part::changed(), and changed(Out&)'s
      // own internal `if(changed())` short-circuit just below) would otherwise vanish
      // from this chain's scope entirely. Bring it back unchanged.
      using Base::changed;
      template<typename Out>
      bool changed(Out& out) {
        bool ticked=false;
        detail::eventVisit(Base::root().body, static_cast<Base&>(*this), (Depth)0,
          Base::level(), Base::sel(), [&](auto& item, auto&){ if(item.tick()) ticked=true; });
        return Base::changed(out)||ticked;
      }
    };
  };

  /// @brief "esc to common base, then go to target" — the OneMenu
  /// equivalent of AM4's escTo()+menuNode::async() composition. Walks from
  /// wherever `nav` currently is to
  /// `target` (an absolute path from root, `len` entries): finds the deepest common
  /// ancestor level, escapes back to it, then descends to `target` one level at a time.
  /// Deliberately built entirely from doCmd()-routed primitives (up/down/enter/esc, all
  /// already observed correctly by EventDispatch) rather than AM4's O(1) index-jump
  /// (go()) + hand-fired events — trades walk speed (O(steps) instead of O(1) per level)
  /// for zero new event-firing code, since every step is a primitive already proven to
  /// fire the right event. A faster jump-based version is possible later (mirroring how
  /// IndexGo's Cmd::Go already jumps via go()) but would need to fire events itself
  /// rather than getting them for free from doCmd.
  template<typename Nav>
  bool gotoPath(Nav& nav, const Sz* target, Depth len) {
    // target[] indexed via (int) casts: Depth is `char` on AVR (base.h, memory-optimized),
    // and a char-typed array subscript trips -Wchar-subscripts even though the values are
    // always small non-negative indices — no actual bug, just satisfying the warning.
    Depth common = 0;
    while(common<nav.level() && common<len && nav.pathSel(common)==target[(int)common]) common++;
    while(nav.level()>common) if(!nav.esc()) return false;
    for(Depth d=common; d<len; d++) {
      while(nav.sel()!=target[(int)d]) if(!(nav.sel()<target[(int)d] ? nav.up() : nav.down())) return false;
      if(d<len-1) if(!nav.enter()) return false;
    }
    return true;
  }

};// namespace oneMenu