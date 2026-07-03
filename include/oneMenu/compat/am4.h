/**
 * @file am4.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4-syntax compatibility macros over OneMenu's compile-time composition.
 * @version 0 (v1 scope — see notes.md "AM4 compat layer")
 *
 * AM4 (github.com/neu-rah/ArduinoMenu) declares each item as a *named static
 * object* (MENU()/FIELD()/OP() each emit a declaration plus a pointer collected
 * into an array — see AM4's macros.h). OneMenu's staticBody() wants the same
 * items as anonymous temporaries nested in one expression tree instead. These
 * macros bridge AM4's call syntax onto that expression-tree shape: MENU(id,...)
 * expands to a single `auto id = menuDef(...)` statement whose entire body is
 * one nested expression, not a sequence of declare-then-collect statements.
 *
 * ── What this v1 covers ──────────────────────────────────────────────────
 *   MENU, PADMENU, OP, EXIT, FIELD, SUBMENU  — the item/menu-tree macros only.
 *
 * ── What this v1 deliberately does NOT cover (tracked in notes.md) ────────
 *  - MENU_INPUTS/MENU_OUTPUTS/NAVROOT device wiring. AM4 builds a genuinely
 *    dynamic runtime menuOut*[]/menuIn*[] list; OneMenu's InDef/OutDef are
 *    compile-time chains. Bridging that needs IOutDef/IOut's runtime-dispatch
 *    escape hatch — a *compat-only* vtable cost, opt-in, never the default
 *    OneMenu path. Not built yet. Declare `in`/`out` the native OneMenu way
 *    (InDef<...>/OutDef<...>) and bind them to `nav` yourself for now.
 *  - Full AM4 eventMask (activate/enter/exit/return/focus/blur/selFocus/
 *    selBlur/update). OneMenu's Action<fn> only fires on Cmd::Enter, which is
 *    the one AM4 event class handled here (via OP's `fn`). Real focus/blur
 *    parity needs TreeNav itself to detect the transition (it already tracks
 *    m_prevSel/levelChanged() for its own redraw logic) and dispatch to the
 *    newly-focused item — nav-level plumbing, not just a new item Part. MENU()
 *    and FIELD()'s own `fn`/`mask`/`style` handler args are therefore accepted
 *    syntactically (so real AM4 call sites compile without editing arg lists)
 *    but ignored — only OP()'s `fn` is actually wired, via Action<fn>.
 *  - AM4 handler signature `result(*)(eventMask)` (and the 2/3-arg overloads).
 *    OP()'s `fn` must already match OneMenu's ActionFunc: `bool(&)(int)`. A
 *    macro can't synthesize an adapter function *inside* a nested expression
 *    (no local function definitions in C++) — bridging real AM4 signatures
 *    needs a named wrapper declared before the MENU() call, out of scope here.
 *  - FIELD()'s `step`/`tune` (accel) params are accepted but ignored — value
 *    always steps by 1. AM4's per-key step isn't wired to NumField yet.
 *
 * ── Known semantic gap: SUBMENU(id) is move-only, not shared ─────────────
 * AM4's SUBMENU(id) references a separately-declared object by pointer — the
 * same instance can be shared/re-referenced elsewhere. OneMenu items are
 * owned by value inside their parent's StaticBody, so SUBMENU(id) here does
 * `std::move(id)`: the submenu is spliced into the parent, and `id` is left
 * moved-from afterward. Don't reference `id` again once it's been SUBMENU()'d
 * into a parent (AM4's serialio.ino-style reuse of one TOGGLE as a SUBMENU in
 * two places does not port as-is).
 */
#pragma once

#include <oneMenu/oneMenu.h>
// NOTE: no <tuple>/<type_traits> — avr-gcc ships no libstdc++ headers at all
// (confirmed empirically: neither exists anywhere under the AVR toolchain).
// std::remove_reference_t etc. are already available via HAPI's own
// hapi/platform/avr/avr_std.h shim (included transitively through
// oneMenu.h -> hapi.h -> base.h on __AVR__ builds; real <type_traits> covers
// it elsewhere). The pointer-pack holder below (PtrPack) is hand-rolled for
// the same reason InGroup/OutGroup can't use std::tuple/std::apply.

namespace am4compat {
  // subset of AM4's systemStyles actually wired up (menuBase.h's full enum has
  // more bits — _canNav/_menuData/etc — that are structural in AM4 and simply
  // don't apply to OneMenu's type-driven composition).
  enum Style : int { noStyle = 0, wrapStyle = 1 << 0 };

  template<int style, typename T, typename B>
  constexpr auto menuDefStyle(T&& t, B&& b) {
    if constexpr ((style & wrapStyle) != 0)
      return oneMenu::menuDef<oneMenu::WrapNav>(std::forward<T>(t), std::forward<B>(b));
    else
      return oneMenu::menuDef<>(std::forward<T>(t), std::forward<B>(b));
  }
}

// AM4's `using namespace Menu;` surface — just enough for existing call sites
// (MENU(...,Menu::doNothing,Menu::noEvent,Menu::wrapStyle,...)) to compile.
// None of eventMask's bits beyond Cmd::Enter are dispatched yet (see file
// comment above) — activateEvent/exitEvent/focusEvent/etc are placeholders.
namespace Menu {
  enum EventMask : int {
    noEvent = 0,
    activateEvent = 1 << 0,
    enterEvent    = 1 << 1,
    exitEvent     = 1 << 2,
    returnEvent   = 1 << 3,
    focusEvent    = 1 << 4,
    blurEvent     = 1 << 5,
    selFocusEvent = 1 << 6,
    selBlurEvent  = 1 << 7,
    updateEvent   = 1 << 8,
    anyEvent      = ~0
  };
  enum SystemStyles : int { noStyle = am4compat::noStyle, wrapStyle = am4compat::wrapStyle };
  inline bool doNothing(int) noexcept { return false; }  // placeholder — never invoked in v1
}

// ── item-tree macros — each expands to a value expression ──────────────────
// (unlike AM4's own OP/EXIT/FIELD, which expand to a declaration + a pointer)

/// @brief AM4 OP(text,fn,mask) — fn must already be OneMenu-shaped: bool(&)(int).
///        mask is accepted but ignored (only Cmd::Enter is dispatched, via Action<fn>).
#define OP(text, fn, mask) \
  ::oneMenu::ItemDef<::oneMenu::Action<fn>, ::oneData::Text>{text}

/// @brief AM4 EXIT(text) — plain item; OneMenu's body-level nav already closes
///        the level on Enter when nothing else claims it (see menu.h Menu::Part::nav).
#define EXIT(text) \
  ::oneMenu::ItemDef<::oneData::Text>{text}

/// @brief AM4 FIELD(var,label,unit,lo,hi,step,tune,fn,mask,style) — var is bound
///        directly via DataRef (zero-copy, same edit-by-reference semantics as
///        AM4's menuField). step/tune/fn/mask/style accepted but ignored (v1).
#define FIELD(var, label, unit, lo, hi, step, tune, fn, mask, style) \
  ::oneMenu::NumFieldDef< \
      ::oneMenu::AsLabel<::oneData::Text>, \
      ::oneMenu::NumField< \
          ::oneData::StaticNumRange<::oneData::StaticRange<(lo), (hi)>>, \
          ::oneMenu::AsField<::oneData::Watch<::oneData::DataRef<&(var)>>> \
      >, \
      ::oneMenu::AsUnit<::oneData::Text> \
    >{label, unit}

/// @brief AM4 MENU(id,text,fn,mask,style,...items) — single statement, whole
///        body is one nested expression. fn/mask accepted but ignored (v1);
///        style's wrapStyle bit is honored (adds WrapNav).
#define MENU(id, text, fn, mask, style, ...) \
  auto id = ::am4compat::menuDefStyle<(style)>( \
      ::oneMenu::ItemDef<::oneData::Text>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 PADMENU(id,text,fn,mask,style,...items) — single-line/pad style menu.
#define PADMENU(id, text, fn, mask, style, ...) \
  auto id = ::oneMenu::padDef( \
      ::oneMenu::ItemDef<::oneData::Text, ::oneMenu::AsEditMode<>>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 SUBMENU(id) — splices a previously MENU()-declared submenu into
///        the enclosing body. Move-only: see file comment "Known semantic gap".
#define SUBMENU(id) std::move(id)

/**
 * ── v2: MENU_INPUTS / MENU_OUTPUTS / NAVROOT (device wiring) ───────────────
 *
 * AM4 needs a *runtime* menuOut*[]/menuIn*[] list here because every single
 * AM4 device — even a sketch with exactly one Serial output — is
 * virtual-dispatch based; that's just how AM4's menuOut/menuIn hierarchy is
 * built. OneMenu's device set is fixed at compile time in real usage, so
 * fanning a poll out to N devices doesn't need runtime dispatch at all — a
 * fold expression over a compile-time-fixed pack of already-declared native
 * InDef<...>/OutDef<...> objects gets the same "multiple devices in
 * parallel" feature AM4 advertises, at zero vtable cost. (This superseded an
 * earlier plan to bridge through IOutDef/IOut's runtime-dispatch escape
 * hatch — that stays available for a genuinely runtime-swappable device set,
 * but real AM4 sketches don't need it: see notes.md "AM4 compat layer".)
 *
 * Syntax fidelity note: MENU_INPUTS is byte-for-byte AM4 syntax — AM4's own
 * MENU_INPUTS already just takes addresses of pre-declared objects, same as
 * here. MENU_OUTPUTS is NOT byte-for-byte: AM4's per-backend device macros
 * (SERIAL_OUT(Serial), U8G2_OUT(...), TFT_eSPI_OUT(...), ...) each construct
 * a device inline — replicating those would need a matching OneMenu backend
 * adapter per AM4 output driver, out of scope here. Pre-declare each output
 * the native OneMenu way (`OutDef<...> out1;`) and pass `&out1` instead.
 */

namespace am4compat {
  // Minimal AVR-safe pointer-pack holder — recursive inheritance instead of
  // std::tuple/std::apply (neither exists on AVR; see file comment above).
  // Both doInput/doOutput evaluate every member unconditionally (not
  // short-circuited by ||) — "poll everything" semantics, matching how a
  // compile-time InDef<KK...> chain already fuses multiple physical sources.
  template<typename... Ins> struct InGroup {
    template<typename Nav> bool doInput(Nav&, oneMenu::Sz = 8) { return false; }
  };
  template<typename I, typename... Ins>
  struct InGroup<I, Ins...> : InGroup<Ins...> {
    I* p;
    InGroup(I* p_, Ins*... rest) : InGroup<Ins...>(rest...), p(p_) {}
    template<typename Nav>
    bool doInput(Nav& nav, oneMenu::Sz maxCount = 8) {
      bool a = p->doInput(nav, maxCount);
      bool b = InGroup<Ins...>::doInput(nav, maxCount);
      return a || b;
    }
  };
  template<typename... Ins> InGroup(Ins*...) -> InGroup<Ins...>;

  template<typename... Outs> struct OutGroup {
    template<typename Nav> bool doOutput(Nav&) { return false; }
  };
  template<typename O, typename... Outs>
  struct OutGroup<O, Outs...> : OutGroup<Outs...> {
    O* p;
    OutGroup(O* p_, Outs*... rest) : OutGroup<Outs...>(rest...), p(p_) {}
    template<typename Nav>
    bool doOutput(Nav& nav) {
      bool a = nav.doOutput(*p);
      bool b = OutGroup<Outs...>::doOutput(nav);
      return a || b;
    }
  };
  template<typename... Outs> OutGroup(Outs*...) -> OutGroup<Outs...>;

  /// @brief AM4 navRoot equivalent: joins IO (an InGroup + an OutGroup) with
  ///        a menu's Nav. Derives from the real Nav type (so id.up()/down()/
  ///        enter()/esc()/level()/... all keep working via plain inheritance)
  ///        and adds .poll() fusing doInput+doOutput across every device in
  ///        the bound InGroup/OutGroup.
  /// Members are named m_in/m_out, NOT in/out — TreeNav::Part already has an
  /// inherited `in(In&)` *method*; a same-named data member here would hide
  /// it entirely (C++ name-hiding applies across kind, not just signature),
  /// turning `nav.in(*this)` inside InDef::inBurst into a bogus "call the
  /// InGroup member like a function" — found the hard way, not theoretical.
  template<typename NavT, typename InG, typename OutG>
  struct AM4Nav : NavT {
    InG& m_in;
    OutG& m_out;
    AM4Nav(InG& i, OutG& o) : m_in(i), m_out(o) {}
    void poll() { m_in.doInput(*this); m_out.doOutput(*this); }
  };
}

/// @brief AM4 MENU_INPUTS(id,&dev1,&dev2,...) — byte-for-byte AM4 syntax.
#define MENU_INPUTS(id, ...) \
  auto id = ::am4compat::InGroup{__VA_ARGS__}

/// @brief AM4 MENU_OUTPUTS(id,maxDepth,&dev1,&dev2,...) — maxDepth accepted
///        but ignored (OneMenu derives depth from the menu type). Devices
///        must be addresses of pre-declared native OutDef<...> objects, not
///        AM4's inline *_OUT(...) constructor macros — see file comment.
#define MENU_OUTPUTS(id, maxDepth, ...) \
  auto id = ::am4compat::OutGroup{__VA_ARGS__}

/// @brief AM4 NAVROOT(id,menu,maxDepth,in,out) — maxDepth accepted but
///        ignored. in/out must be InGroup/OutGroup (from MENU_INPUTS/
///        MENU_OUTPUTS above), matching how real AM4 sketches always route
///        through those macros even for a single device. id.poll() works
///        exactly like AM4's navRoot::poll().
#define NAVROOT(id, menu, maxDepth, in, out) \
  ::am4compat::AM4Nav< \
      ::oneMenu::INavDef<::oneMenu::TreeNav, ::oneMenu::Root<decltype(menu), menu>>, \
      std::remove_reference_t<decltype(in)>, std::remove_reference_t<decltype(out)> \
    > id(in, out)
