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

    // Counterpart for a secondary/hidden-content device (e.g. a footer showing the focused
    // item's Hidden<> text via printHiddenTo — see out.h/item.h). Gated on Base::changed()
    // (position/editMode) rather than doOutput()'s changed(out) (any visible data): hidden
    // content only depends on which item is focused, not on unrelated items' values ticking —
    // conflating the two gates re-clears/redraws it every time anything changes anywhere.
    template<typename Out>
    bool doHiddenOutput(Out& out) {
      if(!Base::changed()) return false;
      out.resume();
      out.clear();
      Base::printHiddenTo(out);
      return true;
    }
  };

  /// @brief compose a navigation chain from nav components (TreeNav, Root, IndexGo, etc.)
  template<typename... II>
  struct NavDef:DefinedNav<NavAPI<hapi::CRTP<NavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<NavDef<II...>>>,II...>;
    using Base::Base;
  };

  struct INav {};

  template<typename... II>
  struct INavDef:INav,DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...>;
    using Base::Base;
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
  ///        changed" cycle every sketch otherwise hand-writes itself in loop()/run()
  ///        (see notes.md "AM4 compat layer" — this used to live only as
  ///        am4compat::AM4Nav, a wrapper *outside* the nav chain; promoted here so
  ///        it composes like any other nav component instead of needing its own
  ///        separately-built Nav type to derive from).
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
  ///        boundary at all — tried, see notes.md "AM4 compat layer" for the
  ///        detour). InList<N>/OutList<N> (genuinely runtime-picked/
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
      // Base=N only reaches downward into the rest of the II... pack, the
      // same static-dispatch trap documented elsewhere in this codebase
      // (feedback_hapi_static_dispatch). obj() casts to the fully-assembled
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
        //changed should not modify the output NEVER, it has to remain faithful to the print  process
        // if(m_level.changed()) {
        //   out.lockMode(LockMode::None);
        //   out.clear();           // resets m_at to {0,0} + hardware clear
        //   return true;
        // }
        // if(m_navMode.changed()||m_prevSel!=sel()) {
        //   // out.resume();          // resets m_at to {orgX,orgY} + device state (invert, cursor)
        //   out.lockMode(hapi::TagIs<PartialDraw>::Check<Out>::value?LockMode::Update:LockMode::None); // resume forces None; set Update after
        //   return true;
        // }
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Changed);
        bool r=printTo(out);    // probe: Gate suppresses all hardware, m_at drift is harmless
        // out.setPos({0,0});//this is clear() job
        // if(r) out.resume();     // data changed: reset for full redraw; resume sets None
        // else 
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
        static Sz tops[root().depth()]{0};//TODO: check if ScrollBody is in output part, or store this there with an API call fallback.
        Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,tops,0,m_prevSel};
        bool r=root().printMenu(out,ctx);
        out.flush();
        return r;
      }

      // Renders the focused item's Hidden<> content to out (pull-based footer).
      template<typename Out>
      void printHiddenTo(Out& out) {
        out.resume();
        static Sz tops[root().depth()+1]{0};
        Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,tops,0,m_prevSel};
        root().printHiddenMenu(out,ctx);
        out.flush();
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
        return cke.kbd ? doCmd<true> (cke.cmd, cke.key, cke.ext)
                       : doCmd<false>(cke.cmd, cke.key, cke.ext);
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
      PathData<depth()+1> m_path{};//TODO: why do we need +1? check depth calc!
      oneData::DataDef<Watch<oneData::Data<Depth>>> m_level{0};
      Depth m_print_level{0};
      oneData::DataDef<Watch<oneData::Data<NavMode>>> m_navMode{NavMode::Nav};
    };
  };

  // Handles Cmd::Go from IdParser: jumps to item N at current level then enters it.
  // Place above TreeNav in the nav chain:  NavDef<IndexGo, TreeNav, Root<...>>
  // NOTE: must override in(), not doCmd() — TreeNav::Part::in() uses static dispatch
  // for doCmd and cannot reach a more-derived doCmd override without CRTP.
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
          Base::go(Sz(cke.key) - 1);  // IdParser emits 1-based; go() is 0-based
          return Base::template doCmd<false>(Cmd::Enter);
        }
        return cke.kbd ? Base::template doCmd<true> (cke.cmd, cke.key, cke.ext)
                       : Base::template doCmd<false>(cke.cmd, cke.key, cke.ext);
      }
    };
  };

  /// @brief AM4-parity event dispatch: detects nav-level state transitions (selection
  /// and level changes) and raises EventMask events to the affected item's onEvent()
  /// hook (item.h) — see notes.md "AM4 compat layer" for the full plan and which AM4
  /// event kinds this targets. Place above TreeNav: NavDef<EventDispatch, TreeNav,
  /// Root<...>>.
  ///
  /// v1 scope/limitations (deliberate, not oversights — see notes.md for the plan this
  /// implements):
  /// - Only Enter/Exit/Focus/Blur are raised. selFocus/selBlur (AM4 fires these on the
  ///   *parent* when a *child's* selection changes — confirmed live in AM4's nav.cpp
  ///   despite its own enum's stale "TODO" comment) and updateEvent/activateEvent are
  ///   real AM4 features but not implemented here yet.
  /// - Dispatches at any level (walks root().body down through the live path to reach
  ///   items inside nested submenus — see detail::eventVisit below). AM4 itself has no
  ///   separate multi-level event mechanism to match here: its own "walk back" (escTo(),
  ///   nav.cpp:310) is just a while(level>lvl) loop calling the single-level exit()
  ///   repeatedly, and its "walk to" (menuNode::async(), items.cpp:83) is a recursive
  ///   per-level index-select-then-enter — both compositions of the *same* single-level
  ///   primitives already wrapped here, called repeatedly/recursively. See gotoPath()
  ///   below for the OneMenu equivalent of that composition.
  /// - Wraps doCmd(), NOT in() — unlike IndexGo just above (which *must* override in(),
  ///   per its own comment, because TreeNav::Part::in() calls doCmd from its own scope,
  ///   unreachable by a more-derived override). doCmd works correctly for direct
  ///   nav.up()/down()/enter()/esc() calls (DefinedNav calls Base::doCmd from outside
  ///   TreeNav, which *does* reach a more-derived override). BUT real
  ///   input-device-driven nav.in()/poll() calls go through TreeNav::in()'s *internal*
  ///   doCmd call, bound to TreeNav's own scope — EventDispatch will NOT see those.
  ///   Events currently only fire for direct nav method calls, not real polled input.
  ///   Fixing this the way IndexGo does (owning in() itself) would need EventDispatch
  ///   and IndexGo to cooperate on a single in() instead of each independently trying to
  ///   own one — deferred, not done.
  /// - Enter/Exit fire regardless of whether they also happened to change level (matches
  ///   AM4: node().event(enterEvent) always fires on Enter, independently of whether the
  ///   item also happens to be a submenu that then gets descended into).
  namespace detail {
    // Detects "does this item have a nested .body" (i.e. it's an ItemDef<Menu<...>>) —
    // std::void_t/declval come from HAPI's avr_std.h shim on AVR (no <type_traits> there
    // at all — see project_avr_no_libstdcxx), from real <type_traits> elsewhere.
    template<typename T, typename = void> struct HasBody : std::false_type {};
    template<typename T> struct HasBody<T, std::void_t<decltype(std::declval<T&>().body)>> : std::true_type {};

    // Walks body down d=0..level (using nav's *current* pathSel(d) for every level
    // except the final one, where idx is used instead — the final level's selection may
    // be the old or new value depending on which event is being raised, not necessarily
    // what's live in the path right now) and invokes fn on whatever item is found there.
    template<typename Body, typename Nav, typename Fn>
    void eventVisit(Body& body, Nav& nav, Depth d, Depth level, Sz idx, Fn&& fn) {
      Sz i = (d==level) ? idx : nav.pathSel(d);
      body.visit(i, [&](auto& item) {
        if(d==level) fn(item);
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
        // plain item with no Action/submenu legitimately returns r=false for Enter
        // (nothing "handles" it in the OneMenu nav<>() sense), but AM4's enterEvent
        // fires unconditionally on Enter regardless of whether the item goes on to do
        // anything with it. Found the hard way: an early `if(!r) return r;` here
        // silently ate every Enter/Exit event for exactly the plain-item case this is
        // most needed for. Known v1 simplification: unlike AM4, this doesn't gate on
        // the target item's enabled() — see file comment.
        Sz newSel = Base::sel();
        Depth newLevel = Base::level();
        if(newLevel==oldLevel && newSel!=oldSel) {
          fireAt(oldLevel, oldSel, [](auto& item){ item.onEvent(EventMask::Blur); });
          fireAt(oldLevel, newSel, [](auto& item){ item.onEvent(EventMask::Focus); });
        }
        if(cmd==Cmd::Enter) fireAt(oldLevel, oldSel, [](auto& item){ item.onEvent(EventMask::Enter); });
        if(cmd==Cmd::Esc) {
          Depth targetLevel = newLevel<oldLevel ? newLevel : oldLevel;
          Sz targetIdx = newLevel<oldLevel ? newSel : oldSel;
          fireAt(targetLevel, targetIdx, [](auto& item){ item.onEvent(EventMask::Exit); });
        }
        return r;
      }
    };
  };

  /// @brief drives per-frame animation (AM22's `TickFocus`, the one working prior-art
  /// implementation across the whole AM4/AM5/AM22-AM26 lineage — see notes.md
  /// "Animation"). Every output poll, dispatches tick() (item.h) to whichever item is
  /// currently focused — reusing EventDispatch's own detail::eventVisit walker, so
  /// composition order relative to EventDispatch doesn't matter, neither depends on the
  /// other.
  ///
  /// Overrides changed(Out&), NOT doOutput() — unlike IndexGo/EventDispatch (which
  /// override doCmd()/in(), methods DefinedNav actually delegates to), doOutput() is
  /// defined directly on DefinedNav itself (this file, above) and never calls
  /// Base::doOutput() at all, so a chain component overriding doOutput() is silently
  /// unreachable (the same static-dispatch trap documented elsewhere in this codebase —
  /// see feedback_hapi_static_dispatch). changed(Out&) IS one of the primitives
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
      // doHiddenOutput() calls the no-arg changed(), which would otherwise vanish from
      // this chain's scope entirely. Bring it back unchanged.
      using Base::changed;
      template<typename Out>
      bool changed(Out& out) {
        bool ticked=false;
        detail::eventVisit(Base::root().body, static_cast<Base&>(*this), (Depth)0,
          Base::level(), Base::sel(), [&](auto& item){ if(item.tick()) ticked=true; });
        return Base::changed(out)||ticked;
      }
    };
  };

  /// @brief "esc to common base, then go to target" (Rui, 2026-07-03) — the OneMenu
  /// equivalent of AM4's escTo()+menuNode::async() composition (see EventDispatch's file
  /// comment above for the source references). Walks from wherever `nav` currently is to
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