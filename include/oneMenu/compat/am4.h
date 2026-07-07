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
// it elsewhere).

// oneMenu.h alone isn't self-sufficient for a real menu tree — every existing
// example (native and AVR) also pulls in these four sibling libraries by hand
// (color palette/DefaultPalette for ansiFmt.h, item/data machinery MENU/FIELD
// expand into). Pulled in here too, for the same "genuinely zero extra
// includes" reason as the backend headers below.
#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>

// Backend headers ANSI_OUT/SERIAL_OUT/Menu::serialIn need — pulled in here
// (not left for the calling sketch to add) so that swapping AM4's own
// #include block for this single header is genuinely a zero-change drop-in,
// not "zero change plus remember these four extra includes."
// Order matters: ansiOut.h pulls in ansiCodes.h (BLUE/WHITE/etc, global
// constants) that ansiFmt.h's own DefaultPalette needs already visible —
// ansiOut.h must come first, matching the order every working example
// (examples/am4compat/src/main.cpp) already uses.
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/idParser.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#ifdef ARDUINO
  #include <oneMenu/menu/IO/arduino/serialIn.h>
  #include <oneMenu/menu/IO/arduino/serialOut.h>
#else
  // streamOut.h (ConsoleOut, ANSI_OUT's device) pulls in <iostream> — doesn't
  // exist on AVR at all (no libstdc++ — see notes.md/project_avr_no_libstdcxx),
  // and ANSI_OUT itself is a native/desktop-console-only macro anyway.
  #include <oneMenu/menu/IO/streamOut.h>
#endif

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

  // TOGGLE/SELECT/CHOOSE builders — SyncValue<W> (item.h) on top of a Behave
  // component wrapping Menu<T,B,...>. Each option in the body is a plain
  // ItemDef<EnumValue<val>,Text> (VALUE() below); SyncValue::nav() reads the
  // currently-selected option's EnumValue<val>::value() via
  // RecallNavPos::visit() and writes it into W (a DataRef<&var>, zero-copy,
  // same binding style FIELD already uses) on every Enter. Verified this
  // actually works end to end (RecallNavPos::visit() had zero real call
  // sites anywhere before this — see notes.md "AM4 compat layer") with a
  // dedicated native test before wiring these macros, not assumed.
  // Three near-identical functions instead of one generalized one — each
  // Behave needs its own fixed Menu<...> modifier list (ParentDraw+WrapNav
  // for Toggle, EditField+ParentDraw for Select, none for Choose), and a
  // template pack can't sit in the middle of a template parameter list next
  // to the other explicit-only params (W) these need — matches this
  // codebase's existing "duplication over cross-cutting generalization"
  // precedent (InDef/InList's independent doInput, etc.).
  template<typename W, typename T, typename B>
  constexpr auto toggleDef(T&& t, B&& b) {
    return oneMenu::ItemDef<
        oneMenu::SyncValue<W>, oneMenu::ToggleBehave,
        oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::ParentDraw, oneMenu::WrapNav>
      >{std::forward<T>(t), std::forward<B>(b)};
  }
  template<typename W, typename T, typename B>
  constexpr auto selectDef(T&& t, B&& b) {
    return oneMenu::ItemDef<
        oneMenu::SyncValue<W>, oneMenu::SelectBehave,
        oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::EditField, oneMenu::ParentDraw>
      >{std::forward<T>(t), std::forward<B>(b)};
  }
  template<typename W, typename T, typename B>
  constexpr auto chooseDef(T&& t, B&& b) {
    return oneMenu::ItemDef<
        oneMenu::SyncValue<W>, oneMenu::RecallNavPos,
        oneMenu::Menu<std::decay_t<T>, std::decay_t<B>>
      >{std::forward<T>(t), std::forward<B>(b)};
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

/// @brief AM4 VALUE(label,val,fn,mask) — one option inside TOGGLE/SELECT/CHOOSE.
///        val must be a compile-time constant — same "static value on the leaf"
///        shape oneMenu::EnumValue<V> (item.h) already provides; TOGGLE/SELECT/
///        CHOOSE read it back via SyncValue on selection. fn/mask accepted but
///        ignored (v1, matching every other value-less-handler macro here).
#define VALUE(label, val, fn, mask) \
  ::oneMenu::ItemDef<::oneMenu::EnumValue<(val)>, ::oneData::Text>{label}

/// @brief AM4 TOGGLE(var,id,label,fn,mask,style,...values) — single Enter
///        cycles to the next VALUE(...) and writes it into var (DataRef,
///        zero-copy, same binding style as FIELD). fn/mask/style accepted but
///        ignored (v1).
#define TOGGLE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::toggleDef<::oneData::DataRef<&(var)>>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 SELECT(var,id,label,fn,mask,style,...values) — Enter opens an
///        inline picker (stays on the parent's display row); a second Enter
///        commits whichever VALUE(...) is highlighted into var (DataRef).
///        fn/mask/style accepted but ignored (v1).
#define SELECT(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::selectDef<::oneData::DataRef<&(var)>>( \
      ::oneMenu::ItemDef<::oneMenu::AsLabel<::oneData::Text>, ::oneMenu::AsEditMode<>>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 CHOOSE(var,id,label,fn,mask,style,...values) — Enter opens a
///        real nested level to browse VALUE(...) options; entering one
///        commits it into var (DataRef). fn/mask/style accepted but ignored
///        (v1).
#define CHOOSE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::chooseDef<::oneData::DataRef<&(var)>>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

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
 * Built directly on native oneMenu machinery, not a separate am4compat-local
 * reimplementation: InGroup/OutGroup (in.h/out.h) are oneMenu's own
 * compile-time-fixed device packs, and oneMenu::Pool<InG,OutG> (nav.h) is a
 * real nav chain component fusing one input source + one output sink for one
 * nav — "we will have to move that cycle into OneMenu so that we have a
 * compatible pool" (Rui, 2026-07-03; see notes.md "AM4 compat layer" for the
 * full design discussion).
 *
 * Went through a detour worth recording here (2026-07-07): first tried
 * building this on InList<N>/OutList<N> (a runtime, virtual-dispatch device
 * *list* — in.h/out.h — "the output fan-out is in itself an output"), which
 * seemed like the more unified answer since IOut let a whole pool be handed
 * anywhere a single Out is expected. It doesn't work for this: Menu::Part::
 * printMenu (menu.h) calls `out.printMenu(*this,ctx)`, templated on the
 * concrete item type — a template method can't be virtual, so IOut
 * fundamentally can't expose it, and once a device is behind IOut& its
 * concrete type (and with it, the ability to run a real templated print
 * walk) is gone for good, not recoverable even one device at a time.
 * InGroup/OutGroup avoid this by construction: recursive inheritance over a
 * compile-time pack keeps every device's concrete type, so nav.doOutput(*p)
 * always sees the real chain — same approach the original v1/v2
 * implementation already used, restored here after the detour. Devices stay
 * plain OutDef<...>/InDef<...> — no vtables needed anywhere in this path.
 *
 * Syntax fidelity: MENU_INPUTS/NAVROOT are byte-for-byte AM4 syntax. NONE
 * (AM4's own ">=2 items" placeholder) is an empty macro, same as AM4's own
 * VAR_HEAD_NONE/REF_HEAD_NONE (macros.h) — it disappears at the token level,
 * leaving a trailing comma before InGroup/OutGroup's closing brace; verified
 * empirically that a braced-init-list's trailing comma stays legal even when
 * it resolves to a variadic constructor call, not just aggregate init.
 */

/// @brief AM4 MENU_INPUTS(id,&dev1,&dev2,...) — byte-for-byte AM4 syntax.
///        Builds a native oneMenu::InGroup over the given devices.
#define MENU_INPUTS(id, ...) \
  auto id = ::oneMenu::InGroup{__VA_ARGS__}

/// @brief AM4 MENU_OUTPUTS(id,maxDepth,&dev1,&dev2,...) — maxDepth accepted
///        but ignored (OneMenu derives depth from the menu type). Builds a
///        native oneMenu::OutGroup over the given devices.
#define MENU_OUTPUTS(id, maxDepth, ...) \
  auto id = ::oneMenu::OutGroup{__VA_ARGS__}

/// @brief AM4 NAVROOT(id,menu,maxDepth,in,out) — maxDepth is accepted but
///        deliberately still ignored, not cross-checked. Tried static_assert-ing
///        it against `decltype(menu)::depth()` (OneMenu's own real nesting-depth
///        member, nav.h) and immediately hit a semantics mismatch, not a bug:
///        OneMenu's depth() counts differently from AM4's maxDepth (e.g. this
///        file's own `mainMenu` — one submenu deep by AM4's counting — reports
///        `depth()==3`, not 2). The two aren't the same quantity, so comparing
///        them isn't a meaningful correctness check, just a false alarm waiting
///        to happen for every real port. OneMenu doesn't need `maxDepth` for
///        anything anyway — `TreeNav`'s own buffers are already sized from its
///        own `depth()` internally (nav.h), independent of whatever the caller
///        passes here — so the argument stays a pure syntax-compat placeholder.
///        in/out must be InGroup/OutGroup (from MENU_INPUTS/MENU_OUTPUTS above),
///        matching how real AM4 sketches always route through those macros
///        even for a single device. id.poll() works exactly like AM4's
///        navRoot::poll() — now oneMenu::Pool's own poll(), reached through
///        ordinary chain inheritance instead of a separate wrapper type
///        deriving from an already-built Nav. Nav chain includes EventDispatch
///        (nav.h) so EventAction/onEvent (item.h) fire for macro-built menus —
///        found the hard way: NAVROOT predates the event system and silently
///        built a plain TreeNav chain with no event dispatch at all until this
///        was added (a real integration gap, not a logic bug in EventDispatch
///        itself — see notes.md "AM4 compat layer"). Pool must stay the
///        *first* component listed here for its constructor to be reachable —
///        see Pool's own doc comment (nav.h) for why.
#define NAVROOT(id, menu, maxDepth, in, out) \
  ::oneMenu::INavDef< \
      ::oneMenu::Pool<decltype(in), decltype(out)>, \
      ::oneMenu::EventDispatch, ::oneMenu::TreeNav, ::oneMenu::Root<decltype(menu), menu> \
    > id(in, out)

/// @brief AM4 NONE — placeholder satisfying MENU_OUTPUTS'/MENU_INPUTS' AM4-side
///        "at least 2 devices" macro quirk. Empty, same as AM4's own NONE
///        (macros.h's VAR_HEAD_NONE/REF_HEAD_NONE expand to nothing too) — the
///        slot just disappears, leaving InGroup/OutGroup's variadic constructor
///        with one fewer real argument. Global, unscoped (like AM4's own —
///        macros aren't namespace-scoped), so avoid this exact token elsewhere
///        in a TU that includes am4.h.
#define NONE

/// @brief AM4-style per-backend output-device macro for OneMenu's ANSI/console
///        backend — `ANSI_OUT(id,w,h)` declares `id` as a ready-to-use device,
///        same spirit as AM4's own `SERIAL_OUT(port)`/`ANSISERIAL_OUT(port,color,
///        panels...)` (each hides a concrete menuIO type behind a one-line call).
///        Not byte-for-byte AM4 syntax — AM4 has no console/native ANSI backend
///        to match against (`ANSISERIAL_OUT` is Serial-port-specific, taking a
///        port + explicit panel regions); this is the compat layer's own
///        backend adapter for the one output stack the native examples actually
///        use (`FullPrinter`/`ANSIFmt`/`ANSIOut`/`ConsoleOut`), following the
///        "components first, macros only for the AM4-call-site translation on
///        top" principle (see notes.md "AM4 compat layer"). Plain OutDef<...>,
///        not IOutDef<...> — MENU_OUTPUTS's OutGroup keeps each device's
///        concrete type itself, no virtual dispatch needed here.
#define ANSI_OUT(id, w, h) \
  ::oneMenu::OutDef< \
      ::oneMenu::FullPrinter, ::oneMenu::ANSIFmt, ::oneMenu::DataParser<>, ::oneMenu::CtrlChars, \
      ::oneMenu::ColorTrack<int>, ::oneMenu::Cursor<>, ::oneMenu::Gate, \
      ::oneMenu::ANSIOut, ::oneMenu::ConsoleOut, ::oneMenu::StaticPos<0,0>, ::oneMenu::StaticArea<(w),(h)> \
    > id

#ifdef ARDUINO
/// @brief AM4-style per-backend output-device macro, *inline* form — unlike
///        ANSI_OUT (which declares a named variable), `SERIAL_OUT(port)` is
///        used directly inside a MENU_OUTPUTS(...) argument list, matching
///        AM4's own SERIAL_OUT call shape exactly. Expands to a pointer to a
///        function-local static OutDef instance (Meyers-singleton) — C++
///        doesn't allow declaring a named variable inside an expression, so
///        this is what makes the inline placement work with zero change to
///        the calling AM4 source. `port` is accepted for AM4 syntax fidelity
///        but ignored: oneMenu::SerialOut (like every bottom-of-chain Arduino
///        IO component here) is a compile-time component bound to the global
///        Serial object (serialOut.h) — OneMenu's device set is fixed at
///        compile time, so a *runtime* stream argument has nothing to bind
///        to. Real AM4 sketches pass literal Serial here anyway; passing
///        Serial1/Serial2 would silently still target Serial. Area is a fixed
///        40x6 default (matches .RnD/am4compat's own SerialOut precedent) —
///        AM4's own SERIAL_OUT has no area parameter to thread through either.
///        Arduino-only (like the real oneMenu::SerialOut it wraps). Plain
///        OutDef<...>, not IOutDef<...> — see ANSI_OUT's comment above.
// NOTE: `return (dev);` — the extra parens are load-bearing, not decoration.
// decltype(auto) deduces from the return *expression as written*: an
// unparenthesized id-expression (`return dev;`) decltype's to the plain
// declared type (T, a copy of the static — &-ing that copy is exactly the
// "taking address of temporary" bug this tripped on first). Parenthesizing
// it (`return (dev);`) turns it into an lvalue expression, so decltype(auto)
// deduces T& instead — the actual persistent static, safe to take &of.
#define SERIAL_OUT(port) \
  (&[]() -> decltype(auto) { \
      static ::oneMenu::OutDef< \
          ::oneMenu::FullPrinter, ::oneMenu::TextFmt, ::oneMenu::DataParser<>, ::oneMenu::CtrlChars, \
          ::oneMenu::Cursor<>, ::oneMenu::Gate, ::oneMenu::SerialOut, \
          ::oneMenu::StaticPos<0,0>, ::oneMenu::StaticArea<40,6> \
        > dev; \
      return (dev); \
    }())

namespace Menu {
  /// @brief AM4 serialIn(Stream&) — input device wrapper, real AM4 call shape
  ///        (`serialIn serial(Serial); MENU_INPUTS(in,&serial);`). Wraps the
  ///        same compile-time Serial+key-parser chain shape
  ///        .RnD/am4compat/.RnD/fielduino hand-wire
  ///        (InDef<SerialIn,IdParser,PCKbd>) — MENU_INPUTS's InGroup keeps
  ///        each device's concrete type itself, no virtual dispatch needed
  ///        here (plain InDef<...>, not IInDef<...>). The Stream&
  ///        constructor arg is accepted for AM4 syntax fidelity but ignored —
  ///        like SerialOut, oneMenu::SerialIn (serialIn.h) is a compile-time
  ///        component bound to the global Serial object; a runtime Stream&
  ///        has nothing to bind to. Real AM4 sketches pass literal Serial
  ///        here anyway.
  struct serialIn : ::oneMenu::InDef<::oneMenu::SerialIn, ::oneMenu::IdParser, ::oneMenu::PCKbd> {
    serialIn(Stream&) {}
  };
}
#endif
