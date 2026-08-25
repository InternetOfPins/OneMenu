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

#include <oneChip/clock.h>

namespace oneMenu {

  // PartialDraw::Part<O> (out.h) marks partial-draw capability via a member typedef
  // (HasPartialUpdate), inherited transitively once ANY component in the chain composes
  // it as a base (Cursor, ANSIOut do) — NOT via the bare PartialDraw struct itself
  // appearing anywhere in the real inheritance chain (nothing derives from it directly,
  // only from PartialDraw::Part<O>, a distinct type per O). hapi::TagIs<PartialDraw>
  // (is_base_of-based) can therefore never find it, on Out or Out::Types either —
  // same root cause as ScrollBodyPrinter's tops[] bug, different failure shape (a
  // marker only reachable via a nested Part<O>, not a directly-Types-listed component).
  // Plain member lookup on the fully-assembled Out type sees inherited members
  // regardless of nesting depth, so a simple SFINAE presence check works correctly here.
  template<typename T, typename = void>
  struct HasPartialUpdate : std::false_type {};
  template<typename T>
  struct HasPartialUpdate<T, std::void_t<typename T::HasPartialUpdate>> : std::true_type {};

  // Same fix, same root cause, applied to the tops[] bug this comment already
  // named above: hapi::query<TagIs<ScrollBodyPrinter>,Out::Types> (formerly used
  // by printTo(), below) never actually finds it — ScrollBodyPrinter::Part's own
  // HasScrollBody marker typedef sidesteps that the same way HasPartialUpdate does.
  template<typename T, typename = void>
  struct HasScrollBody : std::false_type {};
  template<typename T>
  struct HasScrollBody<T, std::void_t<typename T::HasScrollBody>> : std::true_type {};

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
    [[nodiscard]] bool up() {return Base::template doCmd<false>(Cmd::Up);}
    [[nodiscard]] bool down() {return Base::template doCmd<false>(Cmd::Down);}
    [[nodiscard]] bool enter() {return Base::template doCmd<false>(Cmd::Enter);}
    [[nodiscard]] bool esc() {return Base::template doCmd<false>(Cmd::Esc);}

    // AM4's poll(){doInput();doOutput();} half: self-gating output step — redraws (and syncs)
    // only if something visible actually changed, else a no-op. Callers needing to react to
    // *why* it redrew (e.g. gating a secondary output device on position vs. value changes)
    // should check Base::changed() themselves before calling this, since sync() below clears
    // the flags changed() reads.
    //
    // Split into drawOutput()/syncOutput() (doOutput() below is just the two called back to
    // back) so a MULTI-device driver (OutGroup, out.h) can draw every device first, then sync
    // every device — not draw+sync device 1 to completion before device 2 even starts. sync()
    // mutates SHARED, nav-wide state (m_level/m_navMode/m_prevSel below, and any item's own
    // Watch<>/Dirty<>-tracked field value) — not per-device — so interleaving draw+sync per
    // device lets device 1's sync() collapse the "did anything change" signal before device 2
    // ever gets to see it, silently dropping device 2's redraw of the very same change. Mirrors
    // upstream AM4's own outputsList::printMenu/clearChanged split (real AM4 source, confirmed
    // unchanged from its first release through its last: printMenu draws every device against
    // the still-dirty shared flag in one loop, clearChanged only runs — in a separate, later
    // loop — once every device has had its own chance to see it dirty). For a single device
    // (doOutput() below, or an OutGroup of one) this split is exactly today's behavior, just
    // spelled as two calls instead of one — nothing observable changes for the common case.
    template<typename Out>
    bool drawOutput(Out& out) {
      if(!Base::changed(out)) return false;
      // A level change (open/close a submenu — Select/Choose entering their choice list,
      // or Esc/close backing out) swaps in a whole different, unrelated set of items at the
      // same rows. The normal Update-mode pass only force-unlocks items whose ctx.idx equals
      // the old or new *selection index* (see ItemPrinter::printItem) — it has no idea the
      // page's entire content changed, so it keeps skipping/leaving stale rows from the other
      // level. changed() itself must stay a pure query (see its own comment), so the forced
      // full redraw belongs here, the actual output-driving step. levelChanged() is a pure
      // read (m_level.changed()) — untouched by sync() until syncOutput() below, so this
      // still correctly fires for every device in a multi-device draw phase, not just the
      // first one to run.
      if(Base::levelChanged()) {
        out.lockMode(LockMode::None);
        out.clear();
      }
      Base::printTo(out);
      return true;
    }
    template<typename Out>
    void syncOutput(Out& out) {
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
      out.lockMode(HasPartialUpdate<Out>::value ? LockMode::Update : LockMode::None);
    }
    template<typename Out>
    bool doOutput(Out& out) { bool r=drawOutput(out); syncOutput(out); return r; }

    // No-arg twins: drive whatever OO... outputs TreeNav<OO...> owns directly (its own
    // OutGroup<OO...> member already implements this same draw-everything-then-sync-
    // everything discipline internally). Ordinary inheritance resolves these through to
    // TreeNav::Part's own no-arg drawOutput()/syncOutput()/doOutput() — same Base::
    // pattern as every other cross-component call in this chain, no CRTP needed since
    // TreeNav sits below DefinedNav in the real chain, not above it.
    bool drawOutput() { return Base::drawOutput(); }
    void syncOutput() { Base::syncOutput(); }
    bool doOutput()   { return Base::doOutput(); }
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

  /// @brief Virtual-dispatch nav interface, the nav-side twin of item.h's IItem and out.h's IOut. level()/sel()/navMode() are pure virtual; idling()/idleOn()/idleOff() default to a no-op.
  struct INav {
    virtual Depth level() const=0;
    virtual Sz sel() const=0;
    virtual NavMode navMode() const=0;

    /// @brief Whether this nav is still the one in control (not backgrounded by an idle/dialog alternative).
    [[nodiscard]] bool isFocused() const {return !idling();}

    virtual bool idling() const {return false;}
    virtual void idleOn(AltRunFn) {}
    virtual void idleOff() {}

    /// @brief Level-mutating primitives (open/close/padOpen/doNav) — the only way a nav chain's selection/depth changes. Default no-op (not pure).
    [[nodiscard]] virtual bool open() {return false;}
    [[nodiscard]] virtual bool close() {return false;}
    [[nodiscard]] virtual bool padOpen() {return false;}
    [[nodiscard]] virtual bool doNav(CKE,Sz,bool) {return false;}
  };

  template<typename... II>
  struct INavDef:INav,DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...>;
    using Base::Base;
    // Deliberately virtual (runtime-polymorphic dispatch, via the INav base
    // above) -- this is NOT an HLS synthesis target: no HLS backend can
    // synthesize a real vtable call (confirmed: Bambu segfaults on it, see
    // OneOutput/.RnD/hls/FINDINGS.md -- same mechanism applies here). No
    // static_assert here -- std::is_polymorphic_v<INavDef> on the injected
    // class name hits "incomplete type" this early in the class body, and
    // the property can't silently regress anyway: it's baked into the
    // INav base right above.
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
  template<auto& menu>
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

  /// @brief Attaches a compile-time-constant id to a nav chain, letting a runtime key select among several independent NavDef<...> instances.
  template<int Id>
  struct RootId {
    template<typename N>
    struct Part:N {
      static constexpr int rootId() {return Id;}
    };
  };

  template<typename... OO> struct TreeNav;  // forward decl — see full definition below

  /// @brief Expands an already-built OutGroup<Outs...> (e.g. from AM4-compat's
  /// MENU_OUTPUTS) into TreeNav<Outs...> — lets NAVROOT (am4.h) keep its exact
  /// byte-for-byte AM4 macro syntax while TreeNav itself owns the device pack.
  template<typename OutG> struct AsTreeNavT;
  template<typename... Outs> struct AsTreeNavT<OutGroup<Outs...>> { using type = TreeNav<Outs...>; };
  template<typename OutG> using AsTreeNav = typename AsTreeNavT<OutG>::type;

  /// @brief Drives input polling for one nav, throttled to `fps` (default 60Hz) via
  /// PeriodT<1000/fps> (default hw::Period, OneChip/clock.h — override PeriodT with a
  /// chip's own hardware SysTick period type, e.g. chip::SysTick0<>::Period, where
  /// timing precision matters more than the millis()-based software default).
  /// Must be the FIRST component in the chain (e.g. `INavDef<Poll<InG>, EventDispatch,
  /// TreeNav<Out>, Root<...>> id(in,&out);`) for its constructor to be reachable.
  /// Output is owned by TreeNav<OO...> (its own OutGroup<OO...> member) — see TreeNav's
  /// own doc comment for why the device list moved there instead of being a second
  /// caller-assembled parameter here.
  template<typename im,unsigned fps=60,template<uint32_t> class PeriodT=hw::Period>
  struct Poll {
    template<typename N>
    struct Part:N {
      using Base=N;
      im& m_in;
      // Forwards whatever the rest of the chain's own constructor wants (TreeNav<OO...>'s
      // OO*... pack, or its OutGroup<OO...> overload for the AM4 NAVROOT path) — resolved
      // by N's own overload set, not duplicated/disambiguated here.
      template<typename... Args>
      Part(im& i,Args&&... args):N(std::forward<Args>(args)...),m_in(i) {}
      bool poll(Sz maxCount=8) {
        static PeriodT<1000/fps> t;
        if(!t) { hw::delay_ms(t.when()-hw::millis()); return false; }
        // Base::obj() (hapi::CRTP) — doOutput() lives on DefinedNav, which *wraps* the
        // whole component chain from outside (nav.h, top of file), not inside it —
        // Base=N only reaches downward into the rest of the II... pack, so a plain
        // `*this` call would statically bind to this scope and miss doOutput entirely.
        bool i=m_in.doInput(*this,maxCount);
        bool o=Base::obj().doOutput();
        return i||o;
      }
    };
  };

  /// @brief hierarchical tree navigator: tracks path, level, selection, and scroll position.
  /// OO... are the output devices THIS nav owns and drives together — see drawOutput()/
  /// syncOutput()/doOutput() (no-arg) below, which iterate them via an owned OutGroup<OO...>
  /// the same two-phase draw-everything-then-sync-everything way out.h's OutGroup already
  /// does for any *externally* assembled group. TreeNav<> (empty pack) stays a fully
  /// backward-compatible, zero-cost spelling for navs driven purely through the existing
  /// per-call Out&-taking sync(Out&)/changed(Out&)/printTo(Out&) overloads below — those stay,
  /// unconditionally, for one-off/asymmetric devices no single owned pack can express (see
  /// e.g. webMenu's single nav pushing to two differently-triggered Out types).
  template<typename... OO>
  struct TreeNav {
    template<typename N>
    struct Part:N {
      using Base=N;
      using Root=typename Base::Root;
      using Base::root;
      using Base::depth;

      OutGroup<OO...> m_outs;
      Part(OO*... outs):m_outs(outs...) {}
      Part(OutGroup<OO...> og):m_outs(og) {}  // AM4 NAVROOT path: pre-built OutGroup handed in

      // No-arg twins of drawOutput(Out&)/syncOutput(Out&)/doOutput(Out&) (DefinedNav, above) —
      // drive the owned m_outs pack instead of one externally-passed device. Base::obj()
      // (hapi::CRTP) is the fully-assembled Nav, same reasoning as Poll::poll()'s own use of it:
      // OutGroup::drawAll/syncAll call back into nav.drawOutput(*p)/nav.syncOutput(*p) per
      // device, which only exist on the fully-assembled type, not on this Part<N>'s own scope.
      bool drawOutput() { return m_outs.drawAll(Base::obj()); }
      void syncOutput() { m_outs.syncAll(Base::obj()); }
      bool doOutput()   { return m_outs.doOutput(Base::obj()); }

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
      [[nodiscard]] bool levelChanged() const {return m_level.changed();}

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

      [[nodiscard]] bool changed() const {
        return m_level.changed()
          ||m_navMode.changed()
          ||m_prevSel!=sel();//however items will check later if the are on focus or just blur (Ctx sel and prev. sel) and signal a draw (on update)
      }

      template<typename Out>
      [[nodiscard]] bool changed(Out& out) {
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
        // HasScrollBody<Out>, not hapi::query<TagIs<ScrollBodyPrinter>,Out::Types> —
        // the latter never actually matched (ScrollBodyPrinter sits nested inside
        // MenuPrinter<...>'s own template args), so tops stayed permanently nullptr
        // and every real scroll-state write (base.h's Ctx::top(Sz), unguarded unlike
        // the read side) segfaulted the first time a body needed to scroll. See the
        // HasPartialUpdate comment above — same detection defect, same fix shape.
        if constexpr(HasScrollBody<Out>::value) {
          static Sz tops[root().depth()]{0};
          Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,tops,0,m_prevSel};
          bool r=root().printMenu(out,ctx);
          out.flush();
          return r;
        } else {
          Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,nullptr,0,m_prevSel};
          bool r=root().printMenu(out,ctx);
          out.flush();
          return r;
        }
      }

      template<bool isKbd>
      [[nodiscard]] bool doCmd(Cmd cmd,Key k=0, bool e=false) {
        CKE cke{cmd,k,e};
        bool r=root().template nav<isKbd>(Base::obj(),cke,focus(m_level+1));
        if(!r&&cmd==Cmd::Esc) return close();  // items get first chance; close only if unhandled
        return r;
      }

      // aux function for items to call and complete a navigation request with default actions
      // Data<Sz&> would dangle: Data constructor takes by-value, binds reference to local.
      // Use Data<Sz> (owned copy) + writeback through the actual sel reference.
      [[nodiscard]] bool doNav(CKE cke,Sz len,bool w) {
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
      [[nodiscard]] bool in(In& in) {
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

      [[nodiscard]] bool padOpen() {
        if(m_level.get()<depth()) {
          m_level.set(m_level+1);
          m_path.data[m_level]=0;
          if(m_level.get()<m_print_level) m_print_level=m_level;
          return true;
        } else return false;
      }
      [[nodiscard]] bool open() {
        if(padOpen()) {
          m_print_level++;//=m_level;
          return true;
        } else return false;
      }

      [[nodiscard]] bool close() {
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

  // Handles Cmd::Go from IdxParser: jumps to item N at current level then enters it.
  // Place above TreeNav in the nav chain:  NavDef<IndexGo, TreeNav<...>, Root<...>>
  // NOTE: must override in(), not doCmd() — TreeNav::Part::in() uses static dispatch
  // for doCmd and cannot reach a more-derived doCmd override without CRTP. Its own
  // internal doCmd calls route through Base::obj() (hapi::CRTP) for the same reason —
  // so a more-derived doCmd() override (e.g. EventDispatch, composed above IndexGo) is
  // still reached even though it dispatches from here, not from TreeNav::Part::in().
  // While a field is being edited (NavMode::Edit), digit keys are redelivered as a
  // literal Cmd::Key instead of jumping to item N — see idxParser.h/item.h's NumField
  // for the other two pieces of this.
  /// @brief nav component that handles Cmd::Go: jumps to item N at the current level by index
  struct IndexGo {
    template<typename N>
    struct Part : N {
      using Base = N;
      // Forwards whatever constructor N (e.g. TreeNav<OO...>) exposes — needed since
      // Poll<im,fps>'s own constructor now constructs N(outs...) directly (TreeNav<OO...>
      // is no longer unconditionally default-constructible once OO... is non-empty), and
      // hapi::Chain's own `using Base::Base;` (chain.h) only forwards ONE level, not through
      // an intermediate pass-through component like this one.
      using Base::Base;
      template<typename In>
      bool in(In& src) {
        CKE cke = src.cmd();
        if (cke.cmd == Cmd::None) return false;
        if (cke.cmd == Cmd::Go) {
          // Typing a digit while editing a field must deliver it as a
          // literal Cmd::Key, not jump to item N — IdxParser has no nav
          // context to gate this itself (see idxParser.h), so the redirect
          // lives here, the first point in the chain with navMode() available.
          if (Base::navMode() == NavMode::Edit)
            return Base::obj().template doCmd<true>(Cmd::Key, Key('0' + cke.key), false);
          Base::go(Sz(cke.key) - 1);  // IdxParser emits 1-based; go() is 0-based
          return Base::obj().template doCmd<false>(Cmd::Enter);
        }
        // '0' is tagged (not encoded as Cmd::Go — see idxParser.h); only
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

  /// @brief Event dispatch: detects nav-level state transitions (selection and level changes) and raises EventMask events to the affected item's onEvent() (item.h).
  /// Place above TreeNav: NavDef<EventDispatch, TreeNav<...>, Root<...>>. v1 scope: only Enter/Exit/Focus/Blur.
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
      // See IndexGo::Part's own comment — same constructor-forwarding requirement.
      using Base::Base;
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

  /// @brief "Alternative poll handler" pattern: a single active handler, swapped by idleOn()/idleOff() to show an idle screen, dialog, or similar.
  /// `mainFn` is fixed at compile time; `alternative` is the runtime-swappable slot, restored to `mainFn` (not null) by idleOff().
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

  /// @brief Drives per-frame animation: every output poll, dispatches tick() (item.h) to whichever item is currently focused.
  /// Place above TreeNav: NavDef<TickFocus, TreeNav<...>, Root<...>>.
  struct TickFocus {
    template<typename N>
    struct Part : N {
      using Base = N;
      // See IndexGo::Part's own comment (above TreeNav in file order — TickFocus is placed
      // above TreeNav in a real chain too) — same constructor-forwarding requirement.
      using Base::Base;
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

  /// @brief Navigates from wherever `nav` currently is to `target` (an absolute path from root, `len` entries): escapes to the deepest common ancestor, then descends.
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