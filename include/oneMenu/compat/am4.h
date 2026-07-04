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
 *  - AM4's eventMask now has a real counterpart (EventMask, item.h/enums.h/nav.h —
 *    Enter/Exit/Focus/Blur, dispatched by EventDispatch; see notes.md "AM4 compat
 *    layer" for the full plan). OP()'s `fn` fires on Cmd::Enter via Action<fn> (the
 *    zero-cost default). FIELD()'s `fn`/`mask` are wired to a real EventCall<mask,fn>
 *    (fn must be a plain void() — AM4's most common FIELD handler shape). MENU()'s own
 *    `fn`/`mask` (title-level events) are still accepted syntactically but not wired to
 *    anything — nothing calls it yet, unlike FIELD/OP.
 *    selFocusEvent/selBlurEvent/updateEvent/activateEvent have no dispatch yet either
 *    (see nav.h EventDispatch's own scope comment).
 *  - AM4 handler signature `result(*)(eventMask)` (and the 2/3-arg overloads) beyond
 *    the plain `void()`/`bool(int)` shapes EventCall/Action cover. A macro can't
 *    synthesize an adapter function *inside* a nested expression (no local function
 *    definitions in C++) — bridging the richer AM4 signatures
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
// noEvent/enterEvent/exitEvent/focusEvent/blurEvent/anyEvent are defined to be
// bit-identical to oneMenu::EventMask's own values (not AM4's real bit positions —
// this is a source-compat shim, not binary compat with AM4) so that
// FIELD()'s mask arg (see EventCall wiring below) can be cast straight across with no
// translation table. activateEvent/returnEvent/selFocusEvent/selBlurEvent/updateEvent
// get distinct placeholder bits so call sites compile, but nothing dispatches them yet
// (see EventDispatch's v1 scope in nav.h).
namespace Menu {
  enum EventMask : int {
    noEvent       = (int)oneMenu::EventMask::None,
    enterEvent    = (int)oneMenu::EventMask::Enter,
    exitEvent     = (int)oneMenu::EventMask::Exit,
    focusEvent    = (int)oneMenu::EventMask::Focus,
    blurEvent     = (int)oneMenu::EventMask::Blur,
    activateEvent = 1 << 4,
    returnEvent   = 1 << 5,
    selFocusEvent = 1 << 6,
    selBlurEvent  = 1 << 7,
    updateEvent   = 1 << 8,
    anyEvent      = (int)oneMenu::EventMask::Any
  };
  enum SystemStyles : int { noStyle = am4compat::noStyle, wrapStyle = am4compat::wrapStyle };
  inline bool doNothing(int) noexcept { return false; }
  // NOTE: deliberately NOT also overloading doNothing() as void() here — tried it,
  // reverted. avr-g++ 7.3 rejects an *overloaded* function name used directly as a
  // void(&)() template argument ("not a valid template argument for type void(&)()...
  // must be the name of a function with external linkage") even though the same
  // overload set resolves fine on native g++ 13. FIELD()'s fn (wired to EventCall,
  // item.h) therefore needs a real, non-overloaded void() handler — which is what every
  // actual AM4 field handler already is in practice (e.g. Fielduino's updateWave); this
  // only ever bit placeholder/no-op field handlers, not real ports.
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
///        AM4's menuField). fn/mask now wired to a real EventCall<mask,fn> (fn must be
///        a plain void() — AM4's most common FIELD handler shape, e.g. Fielduino's
///        updateWave; see item.h's EventCall for why this is a separate component from
///        EventAction rather than one signature trying to cover every AM4 handler
///        shape). step/tune (AM4's accel) still accepted but ignored — always steps by
///        1. One known behavioral difference from AM4: fires on *both* directions of
///        the Cmd::Enter edit-mode toggle (entering AND leaving edit), not just on
///        commit — harmless for an idempotent handler like updateWave (redraws the same
///        still-unchanged value) but not identical to AM4's commit-only firing.
#define FIELD(var, label, unit, lo, hi, step, tune, fn, mask, style) \
  ::oneMenu::NumFieldDef< \
      ::oneMenu::AsLabel<::oneData::Text>, \
      ::oneMenu::NumField< \
          ::oneData::StaticNumRange<::oneData::StaticRange<(lo), (hi)>>, \
          ::oneMenu::AsField<::oneData::Watch<::oneData::DataRef<&(var)>>> \
      >, \
      ::oneMenu::AsUnit<::oneData::Text>, \
      ::oneMenu::EventCall<(::oneMenu::EventMask)(mask), fn> \
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
///        exactly like AM4's navRoot::poll(). Nav chain includes EventDispatch
///        (nav.h) so EventAction/onEvent (item.h) fire for macro-built menus —
///        found the hard way: NAVROOT predates the event system and silently
///        built a plain TreeNav chain with no event dispatch at all until this
///        was added (a real integration gap, not a logic bug in EventDispatch
///        itself — see notes.md "AM4 compat layer").
#define NAVROOT(id, menu, maxDepth, in, out) \
  ::am4compat::AM4Nav< \
      ::oneMenu::INavDef<::oneMenu::EventDispatch, ::oneMenu::TreeNav, ::oneMenu::Root<decltype(menu), menu>>, \
      std::remove_reference_t<decltype(in)>, std::remove_reference_t<decltype(out)> \
    > id(in, out)
