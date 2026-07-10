/**
 * @file am4.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4-syntax compatibility macros over OneMenu's compile-time composition.
 * @version 0 (v1 scope — see notes.md "AM4 compat layer")
 *
 * Canonical, uniquely-named location (not include/menu.h) so other packages
 * can forward `<menu.h>` to this file by exact path without a self-named
 * #include<menu.h> collision — see include/menu.h (this repo, a thin forward
 * for direct OneMenu consumers) and the ArduinoMenu AM5 branch's src/menu.h
 * (same forward, for real AM4-sketch consumers).
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
 *    layer" for the full plan). OP()'s `fn`/`mask` auto-dispatch on fn's own
 *    signature (am4compat::opItem) — bool(EventMask,IItem&) gets real event
 *    dispatch (EventActionItem, at IItemDef's virtual-dispatch cost), anything
 *    else falls back to the original zero-cost Action<fn> binding; events are
 *    opt-in per call site, not a caller-facing flag (see OP()'s own doc comment).
 *    FIELD()'s `fn`/`mask` are wired to a real EventCall<mask,fn> (fn must be a plain
 *    void() — AM4's most common FIELD handler shape, zero-cost, no IItemDef needed).
 *    MENU()'s own `fn`/`mask` (title-level events) auto-dispatch the same way
 *    (am4compat::menuDefStyle) — a bool(EventMask) fn gets a real
 *    EventAction<mask,fn> spliced into the menu's own component pack (Menu<T,B,MM...>'s
 *    MM..., since the title itself is a plain data member HAPI traversal never
 *    reaches — see menuDefStyle's own doc comment); PADMENU()'s fn/mask work the
 *    same way (am4compat::padDefStyle), but deliberately without style/WrapNav
 *    involvement (see padDefStyle's doc comment).
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
 * ── Known semantic gap: SUBMENU(id)/OBJ(id) are move-only, not shared ──────
 * AM4's SUBMENU(id) references a separately-declared object by pointer — the
 * same instance can be shared/re-referenced elsewhere. OneMenu items are
 * owned by value inside their parent's StaticBody, so SUBMENU(id) here does
 * `std::move(id)`: the submenu is spliced into the parent, and `id` is left
 * moved-from afterward. Don't reference `id` again once it's been SUBMENU()'d
 * into a parent (AM4's serialio.ino-style reuse of one TOGGLE as a SUBMENU in
 * two places does not port as-is). OBJ(id) — AM4's macro for splicing any
 * other hand-declared item object (not necessarily a submenu) into a body —
 * has the identical gap for the identical reason; see OBJ()'s own doc comment.
 */
#pragma once

#include <oneMenu/oneMenu.h>
// NOTE: no <tuple>/<type_traits> — avr-gcc ships no libstdc++ headers at all.
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

  // Detects "callable as bool(EventMask)" — EventAction's shape, distinct
  // from IsEventFn below (EventActionItem's bool(EventMask,IItem&)). Backs
  // MENU()/PADMENU()'s own fn/mask auto-dispatch (menuDefStyle/padDefStyle,
  // below) and TOGGLE/SELECT/CHOOSE's (toggleDef/selectDef/chooseDef,
  // further down). Declared here, ahead of menuDefStyle, deliberately:
  // menuDefStyle's body names IsPlainEventFn unqualified, and that lookup
  // resolves via ordinary (non-ADL) lookup at menuDefStyle's own definition
  // point, not at instantiation — even though the argument (decltype(fn)) is
  // itself dependent, ADL doesn't apply to a class-template name, so
  // IsPlainEventFn must already be visible here (confirmed empirically: a
  // trait referenced before its own declaration in this exact shape is a
  // hard compile error, not deferred lookup).
  template<typename F, typename = void>
  struct IsPlainEventFn : std::false_type {};
  template<typename F>
  struct IsPlainEventFn<F, std::void_t<decltype(std::declval<F>()(std::declval<oneMenu::EventMask>()))>>
    : std::true_type {};

  /// @brief AM4-compat factory backing MENU()'s style/fn/mask. mask/fn are the
  /// menu's own title-level Enter/Exit/Focus/Blur handler — auto-dispatch on
  /// fn's own signature, same "events optional" principle as opItem/toggleDef/
  /// selectDef/chooseDef: a bool(EventMask) fn gets a real EventAction<mask,fn>
  /// spliced into the menu's own MM... component pack (Menu<T,B,MM...>'s title
  /// is a plain data member, NOT part of the HAPI chain — see menu.h; MM...
  /// is the only slot an event component attached to the menu itself can ride
  /// on — same mechanism examples/handlers' subMenu now uses via this factory,
  /// originally proven by hand before this existed). Any other fn shape (e.g.
  /// AM4-legacy bool(int) placeholders like Menu::doNothing) keeps today's
  /// original no-op behavior, byte-for-byte — no IItemDef, no vtable, nothing
  /// new for every existing MENU() call site in this codebase.
  template<int style, oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto menuDefStyle(T&& t, B&& b) {
    if constexpr ((style & wrapStyle) != 0) {
      if constexpr (IsPlainEventFn<decltype(fn)>::value)
        return oneMenu::menuDef<oneMenu::WrapNav, oneMenu::EventAction<mask,fn>>(
            std::forward<T>(t), std::forward<B>(b));
      else
        return oneMenu::menuDef<oneMenu::WrapNav>(std::forward<T>(t), std::forward<B>(b));
    } else {
      if constexpr (IsPlainEventFn<decltype(fn)>::value)
        return oneMenu::menuDef<oneMenu::EventAction<mask,fn>>(std::forward<T>(t), std::forward<B>(b));
      else
        return oneMenu::menuDef<>(std::forward<T>(t), std::forward<B>(b));
    }
  }

  /// @brief AM4-compat factory backing PADMENU's fn/mask — same auto-dispatch
  /// as menuDefStyle, routed through padDef instead of menuDef. Deliberately
  /// single-axis (event-shaped or not only, no style/WrapNav branch): PadDraw
  /// (menu.h) is a pure rendering tag (flips isPad() for the renderer, no
  /// logic of its own) and WrapNav is an orthogonal nav-boundary concern — a
  /// pad has no multi-item nav boundary to wrap, so PADMENU never honored
  /// style's wrapStyle bit and this fix doesn't start (style stays an
  /// accepted-but-inert macro parameter, same as before).
  template<oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto padDefStyle(T&& t, B&& b) {
    if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::padDef<oneMenu::EventAction<mask,fn>>(std::forward<T>(t), std::forward<B>(b));
    else
      return oneMenu::padDef<>(std::forward<T>(t), std::forward<B>(b));
  }

  // Detects "callable as bool(EventMask,IItem&)" — hand-rolled, not
  // std::is_invocable_r_v: avr-gcc has no <type_traits> at all (see
  // notes.md/project_avr_no_libstdcxx), and that trait isn't in HAPI's own
  // minimal avr_std.h shim either. Same std::void_t/declval idiom nav.h's
  // HasBody trait already uses successfully on AVR. Backs OP()'s auto-dispatch
  // below (Rui, 2026-07-09: "can we make events optional for AM5?") — real
  // event handlers (IItemDef+EventActionItem, vtable cost) only for OP() call
  // sites that actually pass one; every existing bool(int) handler keeps the
  // original zero-cost Action<fn> binding, unchanged, no call-site syntax
  // change needed either way.
  template<typename F, typename = void>
  struct IsEventFn : std::false_type {};
  template<typename F>
  struct IsEventFn<F, std::void_t<decltype(std::declval<F>()(
      std::declval<oneMenu::EventMask>(), std::declval<oneMenu::IItem&>()))>>
    : std::true_type {};

  // Detects "callable as bool(EventMask,INav&,IItem&)" — AM4's real 3-arg
  // callback shape (result(eventMask,navNode&,prompt&), menuBase.h),
  // parameter order preserved (event,nav,item). Real AM4 source checked
  // directly (git show master:src/menuBase.h): the `action` class's own
  // constructor overload for this exact 3-arg shape is commented out — AM4
  // itself doesn't wire this through its own OP()-equivalent either
  // (SDCard.ino's real 3-arg handler, filePick, binds to a hand-declared
  // custom object's own constructor instead, spliced via SUBMENU()).
  // OneMenu's OP() is already a uniform auto-dispatch cascade though, so
  // adding a 3rd branch is a strict ergonomic improvement over AM4's own
  // limitation, at zero cost to any existing OP() call site.
  template<typename F, typename = void>
  struct IsEventFnNav : std::false_type {};
  template<typename F>
  struct IsEventFnNav<F, std::void_t<decltype(std::declval<F>()(
      std::declval<oneMenu::EventMask>(), std::declval<oneMenu::INav&>(), std::declval<oneMenu::IItem&>()))>>
    : std::true_type {};

  using EventFuncItemNav = bool(&)(oneMenu::EventMask, oneMenu::INav&, oneMenu::IItem&);

  /// @brief the nav-carrying sibling of item.h's EventActionItem — AM4's
  /// real (event,nav,item) parameter order preserved. Lives here, not
  /// item.h: AM4-signature-matching is compat-only (Rui, 2026-07-09:
  /// "AM4-port machinery should stay AM5/compat-side unless it brings value
  /// to OneMenu itself"). Same "vtable-cost sibling of EventAction, opt-in"
  /// shape as EventActionItem, requires IItemDef (needs Base::obj()), same
  /// reason. Deliberately does NOT try to fold into a further Base::onEvent
  /// (e,n) call the way the 1-arg components fold into Base::onEvent(e) —
  /// item.h's HasNavOnEvent is what makes that safe to omit: nothing below
  /// this component in a real chain would ever have a genuine 2-arg
  /// onEvent to fold into (this is the only nav-aware component in
  /// practice), and `using Base::onEvent;` is what keeps this component's
  /// own 1-arg onEvent(EventMask) reachable (this class only declares the
  /// 2-arg form — without the using-declaration it would hide the
  /// inherited 1-arg one by ordinary C++ name hiding, breaking
  /// IItemDef::onEvent(EventMask)'s own Base::onEvent(e) call; confirmed
  /// empirically with a standalone prototype before this was written).
  template<oneMenu::EventMask mask, EventFuncItemNav fn>
  struct EventActionItemNav {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      using Base::onEvent;  // required — see this type's own doc comment
      bool onEvent(oneMenu::EventMask e, oneMenu::INav& n) {
        return (e & mask) ? fn(e, n, static_cast<oneMenu::IItem&>(Base::obj())) : false;
      }
    };
  };

  /// @brief OP()'s real factory (see OP()'s own doc comment, below). Picks
  /// between the three OP() bindings based on fn's own signature — auto-dispatch,
  /// not a caller-facing flag: nothing to opt into or out of at the call site.
  template<oneMenu::EventMask mask, auto& fn, typename T>
  constexpr auto opItem(T&& text) {
    if constexpr (IsEventFnNav<decltype(fn)>::value)
      return oneMenu::IItemDef<EventActionItemNav<mask,fn>, oneData::Text>{text};
    else if constexpr (IsEventFn<decltype(fn)>::value)
      return oneMenu::IItemDef<oneMenu::EventActionItem<mask,fn>, oneData::Text>{text};
    else
      return oneMenu::ItemDef<oneMenu::Action<fn>, oneData::Text>{text};
  }

  // TOGGLE/SELECT/CHOOSE stay plain ItemDef either way — no IItemDef, no
  // vtable — since real AM4 sketches overwhelmingly reuse the same simple
  // showEvent(eventMask)-shaped handler across OP/TOGGLE/VALUE alike (see
  // serialio.ino/handlers.ino), and there's no existing "index-based" v1
  // binding here to preserve for backward compat the way OP() had
  // (Action<fn>) — today, any fn passed to TOGGLE/SELECT/CHOOSE is simply
  // ignored, so the fallback (no event component at all) is exactly that
  // same, already-shipped no-op behavior, not a new cheap path to design.
  // (IsPlainEventFn itself now lives above, ahead of menuDefStyle/padDefStyle
  // — same trait, reused here.)

  // TOGGLE/SELECT/CHOOSE builders — SyncValue<W> (item.h) on top of a Behave
  // component wrapping Menu<T,B,...>. Each option in the body is a plain
  // ItemDef<EnumValue<val>,Text> (VALUE() below); SyncValue::nav() reads the
  // currently-selected option's EnumValue<val>::value() via
  // RecallNavPos::visit() and writes it into W (a DataRef<&var>, zero-copy,
  // same binding style FIELD already uses) on every Enter. fn/mask
  // auto-dispatch the same way OP() does (Rui, 2026-07-09: "list next
  // targets" -> "#1"): a bool(EventMask) fn gets a real EventAction<mask,fn>
  // spliced into the item's own component pack (fires Enter/Exit/Focus/Blur
  // on the TOGGLE/SELECT/CHOOSE item itself, same as any other item); any
  // other fn shape keeps today's total no-op.
  // Three near-identical functions instead of one generalized one — each
  // Behave needs its own fixed Menu<...> modifier list (ParentDraw+WrapNav
  // for Toggle, EditField+ParentDraw for Select, none for Choose), and a
  // template pack can't sit in the middle of a template parameter list next
  // to the other explicit-only params (W) these need — matches this
  // codebase's existing "duplication over cross-cutting generalization"
  // precedent (InDef/InList's independent doInput, etc.).
  template<typename W, oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto toggleDef(T&& t, B&& b) {
    if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::EventAction<mask,fn>, oneMenu::ToggleBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::ParentDraw, oneMenu::WrapNav>
        >{std::forward<T>(t), std::forward<B>(b)};
    else
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::ToggleBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::ParentDraw, oneMenu::WrapNav>
        >{std::forward<T>(t), std::forward<B>(b)};
  }
  template<typename W, oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto selectDef(T&& t, B&& b) {
    if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::EventAction<mask,fn>, oneMenu::SelectBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::EditField, oneMenu::ParentDraw>
        >{std::forward<T>(t), std::forward<B>(b)};
    else
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::SelectBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::EditField, oneMenu::ParentDraw>
        >{std::forward<T>(t), std::forward<B>(b)};
  }
  template<typename W, oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto chooseDef(T&& t, B&& b) {
    if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::EventAction<mask,fn>, oneMenu::RecallNavPos,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>>
        >{std::forward<T>(t), std::forward<B>(b)};
    else
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
  //
  // `Menu::doNothing` (bool(int)) IS a valid OP() placeholder again — OP()'s
  // fn auto-dispatches on its own signature (am4compat::opItem), and bool(int)
  // falls back to the original zero-cost Action<fn> binding, same as v1. It
  // hits the *same* avr-g++ 7.3 overloaded/inline-as-NTTP limitation documented
  // above, though, so it's still not real-AVR-safe as an OP() placeholder —
  // every real port needing a no-op OP() handler still needs its own local
  // non-inline function, same shape as FIELD()'s noField()/action::noField()
  // workaround (bool(int) if you want the cheap path, bool(EventMask,IItem&)
  // if you specifically want the placeholder to exercise real event dispatch).
}

// ── item-tree macros — each expands to a value expression ──────────────────
// (unlike AM4's own OP/EXIT/FIELD, which expand to a declaration + a pointer)

/// @brief AM4 OP(text,fn,mask) — fn's own signature decides the binding
///        (am4compat::opItem, above), events are opt-in per call site, not a
///        flag (Rui, 2026-07-09: "can we make events optional for AM5?"):
///        - fn shaped bool(EventMask,IItem&) -> real event dispatch
///          (EventActionItem, item.h) matching AM4's actual callback shape
///          (menuBase.h: result(*)(eventMask,navNode&,prompt&); confirmed
///          against serialio.ino's own `action1(eventMask e)`) — costs
///          IItemDef's virtual dispatch (real AM4 items are always virtual
///          too, via prompt&, so this matches AM4's own cost model; see
///          notes.md for the measured AVR delta — mainly RAM, not flash,
///          since the vtable itself lands in .data on this toolchain).
///        - any other fn shape (the original v1 binding — bool(int), mask
///          ignored) -> plain Action<fn>, zero-cost, no IItemDef at all.
///        No caller-facing syntax difference either way — same OP(text,fn,
///        mask) call, the compiler picks. mask is ignored in the bool(int)
///        case (matches v1; nothing to mask when there's no dispatch).
#define OP(text, fn, mask) \
  ::am4compat::opItem<(::oneMenu::EventMask)(mask), fn>(text)

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

namespace am4compat {
  /// @brief AM4 EDIT()'s real factory — buf bound zero-copy via
  /// oneMenu::TextBufRef (fields.h), the TextField-storage counterpart to
  /// FIELD()'s DataRef<&(var)>. validators is bridged directly via
  /// CharMask::PosSet (charMask.h) — no per-position type-building needed,
  /// PosSet indexes the array at runtime inside chk/up/down (an earlier
  /// design tried building N distinct CharMask::Set<> types at compile time,
  /// one per position — doesn't compile in C++17: a pointer computed via
  /// V+I inside a nested template isn't "the address of an object", even
  /// though it's the same address a named &arr[I] would give; confirmed
  /// empirically, see notes.md "AM4 compat layer").
  ///
  /// EventCallT is an ALREADY-BUILT oneMenu::EventCall<mask,fn> type, built
  /// by the EDIT() macro directly (matching FIELD()'s own exact shape) —
  /// NOT a separate `EventMask mask, VoidFunc fn` NTTP pair re-templated in
  /// here. Confirmed empirically (real avr-g++ 7.3 build, not assumed):
  /// passing fn as editItem's own template parameter and re-using it one
  /// level deeper inside EventCall<mask,fn> here fails with "not a valid
  /// template argument... must be the name of a function with external
  /// linkage" — the same family of function-reference-as-NTTP quirk already
  /// hit by am4compat::IdleTimeout (re-deriving RunLoop's RunFn one level
  /// deep) — even for a plain, named, non-overloaded fn. Building
  /// EventCall<mask,fn> at the macro's own call site sidesteps it entirely,
  /// same fix shape as IdleTimeout's Run-as-a-type parameter.
  template<char* buf, int sz, oneData::CText* validators, int n,
           typename EventCallT, typename T>
  constexpr auto editItem(T&& label) {
    using MaskT   = CharMask::PosSet<validators,n>;
    using Storage = oneMenu::TextBufRef<buf,sz>;
    return oneMenu::NumFieldDef<
        oneMenu::AsLabel<oneData::Text>,
        oneMenu::ParentDraw,
        oneMenu::AsField<oneMenu::TextField<sz,MaskT,Storage>>,
        EventCallT
      >{std::forward<T>(label)};
  }
}

/// @brief AM4 EDIT(label,buf,validators,fn,mask,style) — buf is bound
///        directly (zero-copy, in place) via oneMenu::TextBufRef, the
///        TextField-storage counterpart to FIELD()'s DataRef<&(var)>
///        binding; sz is buf's own sizeof(buf)-1, computed here at the call
///        site (AM4: "field will initialize its size by this string
///        length"). validators is AM4's real per-position validator array
///        (macros.h's EDIT_ — one entry per character position, confirmed
///        to repeat cyclically — pos % N — once N is shorter than the
///        buffer, e.g. TextField.ino's single-entry alphaNum[] reused
///        across all 30 positions of the Name field); bridged onto
///        CharMask::PosSet (charMask.h) directly from validators's own
///        address + entry count, both computed here for the same reason sz
///        is. KNOWN ADAPTATION (see file header's "Known semantic gap" —
///        SUBMENU — for the established precedent of documenting these):
///        unlike AM4's own `const char* const validData[]` idiom,
///        `validators` must be declared WITHOUT the trailing pointee-const
///        (`const char* validData[] = {...};`) — CharMask::PosSet<CText*>'s
///        existing NTTP parameter type is `const char**`, which a `const
///        char* const*` array's address does not convert to. fn/mask wired
///        through a real EventCall<mask,fn> — same choice as FIELD()
///        (EDIT()'s closest sibling), not OP()'s auto-dispatch; fn must be
///        a plain void() handler (see FIELD()'s own doc comment, above, for
///        the same avr-g++ NTTP-shape rationale). EventCall<mask,fn> is
///        built HERE, at the macro's own call site (matching FIELD()'s
///        exact shape), not inside editItem — see editItem's own doc
///        comment for why (a real avr-g++ 7.3 NTTP quirk, confirmed
///        empirically). style is accepted for AM4 call-site syntax fidelity
///        but ignored, same as FIELD()'s step/tune.
#define EDIT(label, buf, validators, fn, mask, style) \
  ::am4compat::editItem< \
      (buf), (int)(sizeof(buf)-1), (validators), \
      (int)(sizeof(validators)/sizeof((validators)[0])), \
      ::oneMenu::EventCall<(::oneMenu::EventMask)(mask), fn> \
    >(label)

/// @brief AM4 VALUE(label,val,fn,mask) — one option inside TOGGLE/SELECT/CHOOSE.
///        val must be a compile-time constant — same "static value on the leaf"
///        shape oneMenu::EnumValue<V> (item.h) already provides; TOGGLE/SELECT/
///        CHOOSE read it back via SyncValue on selection. fn/mask accepted but
///        ignored (v1, matching every other value-less-handler macro here).
#define VALUE(label, val, fn, mask) \
  ::oneMenu::ItemDef<::oneMenu::EnumValue<(val)>, ::oneData::Text>{label}

/// @brief AM4 TOGGLE(var,id,label,fn,mask,style,...values) — single Enter
///        cycles to the next VALUE(...) and writes it into var (DataRef,
///        zero-copy, same binding style as FIELD). fn auto-dispatches like
///        OP() (am4compat::toggleDef, above): bool(EventMask) gets a real
///        EventAction<mask,fn> on the item, anything else keeps the original
///        no-op (v1). style still ignored.
#define TOGGLE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::toggleDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 SELECT(var,id,label,fn,mask,style,...values) — Enter opens an
///        inline picker (stays on the parent's display row); a second Enter
///        commits whichever VALUE(...) is highlighted into var (DataRef).
///        fn auto-dispatches like OP() (am4compat::selectDef, above); style
///        still ignored.
#define SELECT(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::selectDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneMenu::AsLabel<::oneData::Text>, ::oneMenu::AsEditMode<>>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 CHOOSE(var,id,label,fn,mask,style,...values) — Enter opens a
///        real nested level to browse VALUE(...) options; entering one
///        commits it into var (DataRef). fn auto-dispatches like OP()
///        (am4compat::chooseDef, above); style still ignored.
#define CHOOSE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::chooseDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 MENU(id,text,fn,mask,style,...items) — single statement, whole
///        body is one nested expression. fn/mask auto-dispatch on fn's own
///        signature (am4compat::menuDefStyle, above) — a bool(EventMask) fn
///        gets real Enter/Exit/Focus/Blur dispatch on the menu item itself;
///        anything else (e.g. Menu::doNothing) is the original no-op, unchanged.
///        style's wrapStyle bit is still honored (adds WrapNav).
#define MENU(id, text, fn, mask, style, ...) \
  auto id = ::am4compat::menuDefStyle<(style), (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 PADMENU(id,text,fn,mask,style,...items) — single-line/pad style
///        menu. fn/mask auto-dispatch the same way MENU() does
///        (am4compat::padDefStyle, above); style is accepted (AM4 call-syntax
///        compat) but not forwarded — wrapStyle never applied to pads before
///        this fix and still doesn't (see padDefStyle's own doc comment).
#define PADMENU(id, text, fn, mask, style, ...) \
  auto id = ::am4compat::padDefStyle<(::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text, ::oneMenu::AsEditMode<>>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 SUBMENU(id) — splices a previously MENU()-declared submenu into
///        the enclosing body. Move-only: see file comment "Known semantic gap".
#define SUBMENU(id) std::move(id)

/// @brief AM4 OBJ(id) — splices a previously hand-declared item object (built
///        without any macro, e.g. a native TextFieldDef<...>/NumFieldDef<...>
///        composed directly) into the enclosing body — AM4's own separate
///        name for "any other pre-built object," as opposed to SUBMENU's "a
///        pre-built submenu" specifically. Mechanically identical in OneMenu:
///        AM4's real OBJ() takes the object's address and inserts it into a
///        runtime Menu::prompt*[] array (its body is a runtime polymorphic
///        pointer array); OneMenu's body is a compile-time heterogeneous type
///        chain instead (StaticBody's own head/tail cons storage, no vtables
///        — see item.h's own "IItem is a cap, not a cost that spreads" doc
///        comment, opt-in only for OP()'s event-shaped-handler case, never
///        used for the body itself), so a pre-declared item is already
///        exactly the right shape for staticBody(...)'s own parameter pack —
///        no type erasure needed, same move-splice SUBMENU() already does.
///        Move-only, same as SUBMENU: see file comment "Known semantic gap".
#define OBJ(id) std::move(id)

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
 * leaving a trailing comma before InGroup/OutGroup's closing brace — a
 * braced-init-list's trailing comma stays legal even when it resolves to a
 * variadic constructor call, not just aggregate init.
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
///        NAVROOT predates the event system and originally built a plain
///        TreeNav chain with no event dispatch at all (see notes.md "AM4
///        compat layer"). Pool must stay the
///        *first* component listed here for its constructor to be reachable —
///        see Pool's own doc comment (nav.h) for why.
#define NAVROOT(id, menu, maxDepth, in, out) \
  ::oneMenu::INavDef< \
      ::oneMenu::Pool<decltype(in), decltype(out)>, \
      ::oneMenu::EventDispatch, ::oneMenu::TreeNav, ::oneMenu::Root<decltype(menu), menu> \
    > id(in, out)

/*
 * ── NAVROOT_IDLE: idle-control bridge (opt-in) ──────────────────────────────
 *
 * Plain NAVROOT's INav::idling()/idleOn()/idleOff() stay the inherited
 * no-op (oneMenu::INav's own default, nav.h) — binding a *specific*
 * RunLoop<mainFn> into a *specific* nav's INav is AM4-flavored wiring (AM4's
 * own nav.root->idleOn()/idleOff(), menuBase.h), so it stays entirely
 * compat-side, mirroring oneMenu::NavAPI/INavDef exactly plus the Run
 * binding (Rui, 2026-07-09: "AM4-port machinery should stay AM5/compat-side
 * unless it brings value to OneMenu itself").
 */
namespace am4compat {
  // Run is an already-built oneMenu::RunLoop<mainFn> TYPE, not mainFn
  // re-templated here — avr-g++ 7.3 rejects re-deriving a function-reference
  // NTTP through a nested template instantiation (same quirk already hit
  // twice this session: IdleTimeout, EDIT()'s editItem); taking Run as a
  // type sidesteps it, same fix shape as IdleTimeout's own Run parameter.
  template<typename N, typename Run>
  struct NavRootAPI : N {
    using Base = N;
    using Base::Base;
    bool idling() const { return Run::active(); }
    void idleOn(oneMenu::AltRunFn fn) { Run::idleOn(fn); }
    void idleOff() { Run::idleOff(); }
  };

  // AM4-compat counterpart to oneMenu::INavDef — identical shape, plus the
  // Run binding.
  template<typename Run, typename... II>
  struct NavRootDef : oneMenu::INav,
      oneMenu::DefinedNav<NavRootAPI<hapi::CRTP<NavRootDef<Run,II...>>, Run>, II...> {
    using Base = oneMenu::DefinedNav<NavRootAPI<hapi::CRTP<NavRootDef<Run,II...>>, Run>, II...>;
    using Base::Base;
    using Base::navMode;  // required — see oneMenu::INavDef's own doc comment (nav.h)
    oneMenu::Depth level() const override { return Base::level(); }
    oneMenu::Sz sel() const override { return Base::sel(); }
    oneMenu::NavMode navMode() const override { return Base::navMode(); }
    bool idling() const override { return Base::idling(); }
    void idleOn(oneMenu::AltRunFn fn) override { Base::idleOn(fn); }
    void idleOff() override { Base::idleOff(); }
  };
}

/// @brief NAVROOT variant binding nav.idleOn()/idleOff() (callable from a
///        3-arg OP() handler) to a specific already-built RunLoop<mainFn> —
///        declare `using Run=oneMenu::RunLoop<mainFn>;` yourself first, same
///        convention as am4compat::IdleTimeout's own usage. NAVROOT itself
///        is untouched — every other port keeps the plain no-op idle
///        fallback, zero new cost.
#define NAVROOT_IDLE(id, menu, maxDepth, in, out, Run) \
  ::am4compat::NavRootDef<Run, \
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

/// @brief AM4 ANSISERIAL_OUT(port,colors,panels...) — real AM4 call shape
///        (`ANSISERIAL_OUT(Serial,colors,{1,1,26,10},{28,1,16,10},{46,1,16,10})`),
///        adapted onto OneMenu's own native ANSI stack rather than bridged:
///        `ansiSerialOut.h` (like every AM4 driver written by the same author
///        as this compat layer) has no licensing reason to stay untouched
///        the way third-party-contributed `menuIO/*.h` drivers do (Rui,
///        2026-07-10) — this reuses `ANSIFmt`+`ANSIOut` (already proven via
///        `ANSI_OUT`, console-only until now) over the real `SerialOut`
///        hardware sink (already proven via `SERIAL_OUT`, plain-text-only
///        until now) instead. Both are `aRawDevice`, so the swap is a
///        straight recomposition — zero new rendering code.
///
///        `port` is accepted but ignored, same "compile-time device, runtime
///        arg has nothing to bind to" precedent as `SERIAL_OUT(port)`/
///        `serialIn(Stream&)` above. `panels...` (multi-panel side-by-side
///        depth display) is accepted but dropped — same "OneMenu doesn't
///        need AM4's own panel/paging bookkeeping, a single output area is
///        enough" precedent `NAVROOT`'s own `maxDepth` already established;
///        a real multi-panel *OneMenu* layout isn't built (see notes.md).
///
///        `colors` is the one genuine call-shape adaptation: AM4's `colors`
///        is a *runtime* `colorDef<uint8_t>[6]` array (6 named roles ×
///        {disabled,enabled}×{normal,selected,editing} states); OneMenu's
///        `ColorTable<...>` needs a *compile-time* `Color<int>::Table<...>`
///        type (organized by state-path, not by role — see docs/oneMenu.md
///        "Cascading color/font tables"). These are different axes on the
///        same information, not a mechanical syntax swap — translating one
///        into the other is real per-sketch port work, same "known semantic
///        gap" category as `SUBMENU`'s move-only limitation. This macro
///        expects `colors` to already *be* that translated `Color<int>::
///        Table<...>` type (a `using colors = ...;` alias at the call site,
///        in place of AM4's runtime array declaration) — the macro call
///        itself then stays textually identical to the real AM4 source.
///        Area is a fixed 40x10 default, same "no area param to thread
///        through, pick a sensible fixed size" precedent as `SERIAL_OUT`'s
///        own 40x6.
#define ANSISERIAL_OUT(port, colors, ...) \
  (&[]() -> decltype(auto) { \
      static ::oneMenu::OutDef< \
          ::oneMenu::FullPrinter, ::oneMenu::ANSIFmt, ::oneMenu::DataParser<>, ::oneMenu::CtrlChars, \
          ::oneMenu::ColorTable<colors>, ::oneMenu::ColorTrack<int>, ::oneMenu::Cursor<>, ::oneMenu::Gate, \
          ::oneMenu::ANSIOut, ::oneMenu::SerialOut, \
          ::oneMenu::StaticPos<0,0>, ::oneMenu::StaticArea<40,10> \
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

namespace am4compat {
  /// @brief AM4's nav.timeOut/idleTask/nav.idleOn()/nav.sleepTask auto-idle
  /// trigger — adapted onto oneMenu::RunLoop (nav.h) purely from this compat
  /// layer, no OneMenu header touched (Rui, 2026-07-09: AM4-port machinery
  /// should stay AM5/compat-side unless it brings value to OneMenu itself —
  /// RunLoop already does, this doesn't need to). Platform-agnostic (unlike
  /// SERIAL_OUT/serialIn above, not gated on ARDUINO): RunLoop/hw::Timeout
  /// both already work natively too. TimeoutMs is AM4's nav.timeOut
  /// (inactivity window, ms); Run is an already-built oneMenu::RunLoop<mainFn>
  /// *type* (not mainFn itself, re-templated) — avr-g++ 7.3 rejects
  /// re-deriving a RunFn (bool(&)()) NTTP through a nested template
  /// instantiation like this one ("not a valid template argument... must be
  /// the name of a function with external linkage"), the same family of
  /// function-reference-as-NTTP quirk already documented elsewhere in this
  /// file (Menu::doNothing/OP(), FIELD()'s noField()) — confirmed empirically
  /// on real avr-g++ 7.3, not assumed; taking Run as a type sidesteps it
  /// entirely since there's nothing left to re-derive. AM4's real 2-arg
  /// idleTask(out,idleEvent) start/end signature isn't replicated — idleFn is
  /// a plain bool(), same AltRunFn shape every other RunLoop alternative
  /// already uses, matching this file's established "simplify the handler
  /// shape" precedent (Confirm's systemExit, OP()'s Action<fn>).
  ///
  /// Usage (see examples/fullIdle): `using Run=oneMenu::RunLoop<mainFn>;
  /// using Idle=am4compat::IdleTimeout<50,Run>;` then call
  /// tick(activity,idleFn) once per mainFn invocation, where `activity` is
  /// whatever already-real signal means "something happened this tick" (e.g.
  /// nav.poll()'s own bool return — no new activity-detection machinery
  /// needed, poll() already reports it). idleFn is whatever the sketch wants
  /// to run while suspended — same free-standing bool() shape as
  /// RunLoop::idleOn's own argument; it's the idleFn's own job to call
  /// Run::idleOff() once its own "wake up" condition is met (AM4's
  /// nav.idleOff()).
  template<unsigned long TimeoutMs, typename Run>
  struct IdleTimeout {
    static inline hw::Timeout<TimeoutMs> timer;
    static void tick(bool activity, oneMenu::AltRunFn idleFn) {
      if(activity) timer.reset();
      // hw::Timeout latches once fired — reset the moment we act on it, so a
      // stale fired-flag from a previous idle cycle can't immediately
      // re-trigger idleOn() again right after idleOff() restores mainFn.
      else if(timer) { timer.reset(); Run::idleOn(idleFn); }
    }
  };
}
