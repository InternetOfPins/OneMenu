/**
 * @file am4.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4-syntax compatibility macros over OneMenu's compile-time composition.
 * @version 0 (v1 scope)
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
 *   MENU, PADMENU, OP, EXIT, FIELD, SUBMENU — the item/menu-tree macros.
 *   MENU_INPUTS, MENU_OUTPUTS, NAVROOT, NAVROOT_IDLE — device wiring, byte-
 *   for-byte AM4 syntax, built on InGroup/OutGroup (compile-time device packs
 *   via recursive inheritance, in.h/out.h) rather than any runtime-dispatch
 *   escape hatch — no vtables anywhere in this path (see MENU_INPUTS' own doc
 *   comment for why InList<N>/OutList<N> specifically can't work here).
 *
 * ── What this v1 deliberately does NOT cover ───────────────────────────────
 *  - AM4's eventMask now has a real counterpart (EventMask, item.h/enums.h/nav.h —
 *    Enter/Exit/Focus/Blur, dispatched by EventDispatch). OP()'s `fn`/`mask` auto-dispatch on fn's own
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
 *  - AM4's real `result`-returning handler shapes (`Menu::result()`,
 *    `Menu::result(oneMenu::EventMask)`, `Menu::result(oneMenu::EventMask,IItem&)`,
 *    `Menu::result(oneMenu::EventMask,INav&,IItem&)`) are now covered by OP()'s
 *    own auto-dispatch (am4compat::opItem, IsResultFn0/1/IsResultFn/
 *    IsResultFnNav) — a real, unmodified AM4 handler returning `Menu::result`
 *    (not bool) can be dropped into OP() as-is; quit's real "close this
 *    level" effect is honored wherever INav& is reachable (the 0/1/3-arg
 *    tiers), silently discarded (documented) for the 2-arg tier, which has
 *    no INav& access. TOGGLE()/SELECT()/CHOOSE()'s own fn/mask now also
 *    auto-dispatch a `Menu::result(oneMenu::EventMask)` handler
 *    (am4compat::ResultEventAction1, reusing IsResultFn1/ResultFunc1) —
 *    quit suppresses that item's own default action (cycle/inline-open/
 *    nested-open) for that Enter, a deliberate, disclosed narrower semantic
 *    than AM4's own isMenu()-conditional exit()-on-quit branch (nav.cpp),
 *    not a bug.
 *    FIELD()/MENU()/PADMENU() remain bool-only — confirmed (by tracing
 *    each one's real nav() chain) that their fn/mask slot sits INSIDE or
 *    BELOW the component that already performs the default action
 *    (Menu<T,B,OO...>::Part's own nav(), menu.h, hardcodes
 *    n.open()/n.padOpen() for the p.len==0&&Enter case without ever
 *    consulting OO...; EditField, item.h, sits outer to FIELD()'s own
 *    EventCall slot and has already decided edit-mode entry by the time
 *    EventCall would run) — unlike OP()/TOGGLE()/SELECT()/CHOOSE(), where
 *    the fn slot sits directly above the component whose default action
 *    it needs to pre-empt. Honoring quit there would need an actual
 *    menu.h/item.h core hook, not a compat-side addition — a real scope
 *    decision, not attempted here.
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
#include <oneMenu/menu/IO/idxParser.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#ifdef ARDUINO
  #include <oneMenu/menu/IO/arduino/serialIn.h>
  #include <oneMenu/menu/IO/arduino/serialOut.h>
#else
  // streamOut.h (ConsoleOut, ANSI_OUT's device) pulls in <iostream> — doesn't
  // exist on AVR at all (no libstdc++), and ANSI_OUT itself is a
  // native/desktop-console-only macro anyway.
  #include <oneMenu/menu/IO/streamOut.h>
#endif

// Moved up from the main `namespace Menu { ... }` compat shim (further down
// this file, alongside SystemStyles/doNothing): am4compat's own
// IsResultFn*/ResultAction* template bodies below reference Menu::quit/
// Menu::enterEvent as non-dependent names, resolved at template-DEFINITION
// time (ordinary two-phase lookup) — a forward declaration isn't enough,
// the real enumerators must already be visible here, not just later in the
// file.
//
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
  // AM4's real handler return type (menuBase.h: `enum result {proceed=0,quit};`).
  // A plain (unscoped) enum, deliberately: real AM4 handlers written against
  // this exact name/values (`return proceed;`/`return quit;`) port unmodified.
  // See am4compat::opItem's own doc comment for how quit's real "close this
  // level" effect (AM4's nav.cpp: `if(go==quit&&!selected().isMenu()) exit();`)
  // gets honored here.
  enum result : int { proceed = 0, quit = 1 };
}

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
  // IsPlainEventFn must already be visible here: a trait referenced before
  // its own declaration in this exact shape is a hard compile error, not
  // deferred lookup.
  template<typename F, typename = void>
  struct IsPlainEventFn : std::false_type {};
  template<typename F>
  struct IsPlainEventFn<F, std::void_t<decltype(std::declval<F>()(std::declval<oneMenu::EventMask>()))>>
    : std::true_type {};

  /// AM4-compat factory backing MENU()'s style/fn/mask. A bool(EventMask) fn
  /// gets a real EventAction<mask,fn>; any other fn shape is a no-op.
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

  /// AM4-compat factory backing PADMENU's fn/mask, routed through padDef
  /// instead of menuDef. style's wrapStyle bit is not honored here.
  template<oneMenu::EventMask mask, auto& fn, typename T, typename B>
  constexpr auto padDefStyle(T&& t, B&& b) {
    if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::padDef<oneMenu::EventAction<mask,fn>>(std::forward<T>(t), std::forward<B>(b));
    else
      return oneMenu::padDef<>(std::forward<T>(t), std::forward<B>(b));
  }

  // Detects "callable as bool(EventMask,IItem&)" — hand-rolled, not
  // std::is_invocable_r_v: avr-gcc has no <type_traits> at all, and that
  // trait isn't in HAPI's own minimal avr_std.h shim either. Same
  // std::void_t/declval idiom nav.h's HasBody trait already uses
  // successfully on AVR. Backs OP()'s auto-dispatch below — real event
  // handlers (IItemDef+EventActionItem, vtable cost) only for OP() call
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
  // parameter order preserved (event,nav,item). In AM4's own source, the
  // `action` class's own constructor overload for this exact 3-arg shape is
  // commented out — AM4 itself doesn't wire this through its own
  // OP()-equivalent either (SDCard.ino's real 3-arg handler, filePick, binds
  // to a hand-declared
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

  using EventFuncItemNavPtr = bool(*)(oneMenu::EventMask, oneMenu::INav&, oneMenu::IItem&);

  /// Event handler receiving (EventMask,INav&,IItem&); requires IItemDef<...>.
  /// mask/fn are runtime constructor-set members.
  struct EventActionItemNav {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::onEvent;  // required — see this type's own doc comment
      oneMenu::EventMask mask;
      EventFuncItemNavPtr fn;
      template<typename... Rest>
      constexpr Part(oneMenu::EventMask m, EventFuncItemNavPtr f, Rest&&... rest)
        : Base(std::forward<Rest>(rest)...), mask(m), fn(f) {}
      bool onEvent(oneMenu::EventMask e, oneMenu::INav& n) {
        return (e & mask) ? fn(e, n, static_cast<oneMenu::IItem&>(Base::obj())) : false;
      }
    };
  };

  // ── Menu::result-returning handler shapes ───────────────────────────────
  // AM4's own real handler return type is `result` (proceed/quit), not bool —
  // every tier above (IsEventFn/IsEventFnNav/Action<fn>) requires a
  // bool-returning fn, so a real, unmodified AM4 handler
  // (`Menu::result myLedOn()`, `Menu::result doAlert(oneMenu::EventMask,
  // oneMenu::IItem&)`, etc.) still couldn't be dropped into OP() as-is
  // without these. Checked and dispatched BEFORE the bool-based tiers below:
  // a Menu::result-returning fn is *also* silently callable through those
  // (Menu::result is a plain enum, implicitly convertible to bool via its own
  // underlying int — proceed=0->false, quit=1->true), which would compile but
  // silently discard quit's real navigational effect (and invert its meaning
  // relative to what "true" means to the bool-based tiers' own "handled"
  // accumulator). Checking the return type explicitly (not just callability)
  // avoids that trap. Deliberately NOT AM4's own mechanism (menuBase.h's
  // `action` class raw-casts any handler's function pointer to a fixed 3-arg
  // `result(*)(eventMask,navNode&,prompt&)` type and calls it with all 3 real
  // arguments regardless of the handler's own declared arity — relying on the
  // target ABI tolerating a call with more arguments than the callee
  // declares, real UB even though AM4 has shipped on it for years) — this
  // compat layer instead detects each real shape via ordinary SFINAE and
  // calls fn with exactly the arguments it declares. No reinterpret casts,
  // no calling-convention assumptions.
  template<typename F, typename = void>
  struct IsResultFnNav : std::false_type {};
  template<typename F>
  struct IsResultFnNav<F, std::enable_if_t<std::is_same_v<
      decltype(std::declval<F>()(std::declval<oneMenu::EventMask>(),
                                  std::declval<oneMenu::INav&>(),
                                  std::declval<oneMenu::IItem&>())),
      Menu::result>>> : std::true_type {};

  template<typename F, typename = void>
  struct IsResultFn : std::false_type {};
  template<typename F>
  struct IsResultFn<F, std::enable_if_t<std::is_same_v<
      decltype(std::declval<F>()(std::declval<oneMenu::EventMask>(),
                                  std::declval<oneMenu::IItem&>())),
      Menu::result>>> : std::true_type {};

  template<typename F, typename = void>
  struct IsResultFn1 : std::false_type {};
  template<typename F>
  struct IsResultFn1<F, std::enable_if_t<std::is_same_v<
      decltype(std::declval<F>()(std::declval<oneMenu::EventMask>())),
      Menu::result>>> : std::true_type {};

  template<typename F, typename = void>
  struct IsResultFn0 : std::false_type {};
  template<typename F>
  struct IsResultFn0<F, std::enable_if_t<std::is_same_v<
      decltype(std::declval<F>()()), Menu::result>>> : std::true_type {};

  using ResultFunc0 = Menu::result(&)();
  using ResultFunc1 = Menu::result(&)(oneMenu::EventMask);

  /// Zero-cost OP() tier for a `Menu::result()` handler; only reachable on
  /// Cmd::Enter. Closes the level when fn returns quit.
  template<ResultFunc0 fn>
  struct ResultAction0 {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      template<bool isKbd, typename Nav>
      static bool nav(Nav& n, const oneMenu::CKE& cke, oneMenu::Path path) {
        if (cke.cmd != oneMenu::Cmd::Enter) return false;
        if (fn() == Menu::quit) n.close();
        return true;
      }
    };
  };

  /// Zero-cost OP() tier for a `Menu::result(EventMask)` handler; fn is
  /// always called with EventMask::Enter.
  template<ResultFunc1 fn>
  struct ResultAction1 {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      template<bool isKbd, typename Nav>
      static bool nav(Nav& n, const oneMenu::CKE& cke, oneMenu::Path path) {
        if (cke.cmd != oneMenu::Cmd::Enter) return false;
        if (fn(oneMenu::EventMask::Enter) == Menu::quit) n.close();
        return true;
      }
    };
  };

  using ResultFuncItemPtr    = Menu::result(*)(oneMenu::EventMask, oneMenu::IItem&);
  using ResultFuncItemNavPtr = Menu::result(*)(oneMenu::EventMask, oneMenu::INav&, oneMenu::IItem&);

  /// OP() tier for a `Menu::result(EventMask,IItem&)` handler; requires
  /// IItemDef<...>. fn's return value is discarded — quit is not honored
  /// (no INav& reachable at this arity); use ResultActionItemNav for that.
  struct ResultActionItem {
    template<typename I>
    struct Part : I {
      using Base = I;
      oneMenu::EventMask mask;
      ResultFuncItemPtr fn;
      template<typename... Rest>
      constexpr Part(oneMenu::EventMask m, ResultFuncItemPtr f, Rest&&... rest)
        : Base(std::forward<Rest>(rest)...), mask(m), fn(f) {}
      template<bool isKbd, typename Nav>
      bool nav(Nav& n, const oneMenu::CKE& cke, oneMenu::Path path) {
        if (cke.cmd != oneMenu::Cmd::Enter) return Base::template nav<isKbd>(n, cke, path);
        fn(oneMenu::EventMask::Enter, static_cast<oneMenu::IItem&>(Base::obj()));
        return true;
      }
    };
  };

  /// OP() tier for a `Menu::result(EventMask,INav&,IItem&)` handler;
  /// requires IItemDef<...>. Honors quit by closing the level.
  struct ResultActionItemNav {
    template<typename I>
    struct Part : I {
      using Base = I;
      oneMenu::EventMask mask;
      ResultFuncItemNavPtr fn;
      template<typename... Rest>
      constexpr Part(oneMenu::EventMask m, ResultFuncItemNavPtr f, Rest&&... rest)
        : Base(std::forward<Rest>(rest)...), mask(m), fn(f) {}
      template<bool isKbd, typename Nav>
      bool nav(Nav& n, const oneMenu::CKE& cke, oneMenu::Path path) {
        if (cke.cmd != oneMenu::Cmd::Enter) return Base::template nav<isKbd>(n, cke, path);
        if (fn(oneMenu::EventMask::Enter, n, static_cast<oneMenu::IItem&>(Base::obj())) == Menu::quit) n.close();
        return true;
      }
    };
  };

  /// OP()'s factory; picks the OP() binding to use based on fn's signature.
  template<oneMenu::EventMask mask, auto& fn, typename T>
  constexpr auto opItem(T&& text) {
    if constexpr (IsResultFnNav<decltype(fn)>::value)
      return oneMenu::IItemDef<ResultActionItemNav, oneData::Text>{mask, &fn, text};
    else if constexpr (IsResultFn<decltype(fn)>::value)
      return oneMenu::IItemDef<ResultActionItem, oneData::Text>{mask, &fn, text};
    else if constexpr (IsResultFn0<decltype(fn)>::value)
      return oneMenu::ItemDef<ResultAction0<fn>, oneData::Text>{text};
    else if constexpr (IsResultFn1<decltype(fn)>::value)
      return oneMenu::ItemDef<ResultAction1<fn>, oneData::Text>{text};
    else if constexpr (IsEventFnNav<decltype(fn)>::value)
      return oneMenu::IItemDef<EventActionItemNav, oneData::Text>{mask, &fn, text};
    else if constexpr (IsEventFn<decltype(fn)>::value)
      return oneMenu::IItemDef<oneMenu::EventActionItem, oneData::Text>{mask, &fn, text};
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

  /// Zero-cost TOGGLE/SELECT/CHOOSE tier for a `Menu::result(EventMask)`
  /// handler; `quit` suppresses the item's own default action for that Enter.
  template<oneMenu::EventMask mask, ResultFunc1 fn>
  struct ResultEventAction1 {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      template<bool isKbd, typename Nav>
      bool nav(Nav& n, const oneMenu::CKE& cke, oneMenu::Path path) {
        if (cke.cmd == oneMenu::Cmd::Enter && (mask & oneMenu::EventMask::Enter)
            && fn(oneMenu::EventMask::Enter) == Menu::quit)
          return true;
        return Base::template nav<isKbd>(n, cke, path);
      }
    };
  };

  // TOGGLE/SELECT/CHOOSE builders — SyncValue<W> (item.h) on top of a Behave
  // component wrapping Menu<T,B,...>. Each option in the body is a plain
  // ItemDef<EnumValue<val>,Text> (VALUE() below); SyncValue::nav() reads the
  // currently-selected option's EnumValue<val>::value() via
  // RecallNavPos::visit() and writes it into W (a DataRef<&var>, zero-copy,
  // same binding style FIELD already uses) on every Enter. fn/mask
  // auto-dispatch the same way OP() does: a bool(EventMask) fn gets a real EventAction<mask,fn>
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
    if constexpr (IsResultFn1<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, ResultEventAction1<mask,fn>, oneMenu::ToggleBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::ParentDraw, oneMenu::WrapNav>
        >{std::forward<T>(t), std::forward<B>(b)};
    else if constexpr (IsPlainEventFn<decltype(fn)>::value)
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
    if constexpr (IsResultFn1<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, ResultEventAction1<mask,fn>, oneMenu::SelectBehave,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::EditField, oneMenu::ParentDraw>
        >{std::forward<T>(t), std::forward<B>(b)};
    else if constexpr (IsPlainEventFn<decltype(fn)>::value)
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
    if constexpr (IsResultFn1<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, ResultEventAction1<mask,fn>, oneMenu::RecallNavPos<>,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::IsChoiceBody>
        >{std::forward<T>(t), std::forward<B>(b)};
    else if constexpr (IsPlainEventFn<decltype(fn)>::value)
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::EventAction<mask,fn>, oneMenu::RecallNavPos<>,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::IsChoiceBody>
        >{std::forward<T>(t), std::forward<B>(b)};
    else
      return oneMenu::ItemDef<
          oneMenu::SyncValue<W>, oneMenu::RecallNavPos<>,
          oneMenu::Menu<std::decay_t<T>, std::decay_t<B>, oneMenu::IsChoiceBody>
        >{std::forward<T>(t), std::forward<B>(b)};
  }
}

// AM4's `using namespace Menu;` surface — just enough for existing call sites
// (MENU(...,Menu::doNothing,Menu::noEvent,Menu::wrapStyle,...)) to compile.
// EventMask/result themselves are defined earlier in this file now (see the
// comment there for why — two-phase lookup in am4compat's own
// IsResultFn*/ResultAction* templates needs them visible before this point).
namespace Menu {
  enum SystemStyles : int { noStyle = am4compat::noStyle, wrapStyle = am4compat::wrapStyle };
  inline bool doNothing(int) noexcept { return false; }
  // NOTE: deliberately NOT also overloading doNothing() as void() here.
  // avr-g++ 7.3 rejects an *overloaded* function name used directly as a
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

/// AM4 OP(text,fn,mask) — creates an action item; fn's signature selects the
/// binding (am4compat::opItem). mask is ignored for a plain bool(int) fn.
#define OP(text, fn, mask) \
  ::am4compat::opItem<(::oneMenu::EventMask)(mask), fn>(text)

/// @brief AM4 EXIT(text) — plain item; OneMenu's body-level nav already closes
///        the level on Enter when nothing else claims it (see menu.h Menu::Part::nav).
#define EXIT(text) \
  ::oneMenu::ItemDef<::oneData::Text>{text}

/// AM4 FIELD(var,label,unit,lo,hi,step,tune,fn,mask,style) — numeric field
/// bound to var via DataRef; fn must be a plain void(), fires on both
/// entering and leaving edit mode. step/tune are accepted but ignored.
#define FIELD(var, label, unit, lo, hi, step, tune, fn, mask, style) \
  ::oneMenu::NumFieldDef< \
      ::oneMenu::AsLabel<::oneData::Text>, \
      ::oneMenu::NumField< \
          ::oneData::StaticNumRange<::oneData::StaticRange<(lo), (hi)>>, \
          ::oneMenu::AsField<::oneData::Dirty<::oneData::DataRef<&(var)>>> \
      >, \
      ::oneMenu::AsUnit<::oneData::Text>, \
      ::oneMenu::EventCall<(::oneMenu::EventMask)(mask), fn> \
    >{label, unit}

namespace am4compat {
  /// AM4 EDIT()'s factory; buf is bound zero-copy via TextBufRef, validators
  /// bridged via CharMask::PosSet. EventCallT must be an already-built
  /// oneMenu::EventCall<mask,fn> type.
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

/// AM4 EDIT(label,buf,validators,fn,mask,style) — text field bound to buf
/// via TextBufRef; validators is a per-position validator array, declared
/// WITHOUT trailing pointee-const. fn must be a plain void(); style is ignored.
#define EDIT(label, buf, validators, fn, mask, style) \
  ::am4compat::editItem< \
      (buf), (int)(sizeof(buf)-1), (validators), \
      (int)(sizeof(validators)/sizeof((validators)[0])), \
      ::oneMenu::EventCall<(::oneMenu::EventMask)(mask), fn> \
    >(label)

/// AM4 VALUE(label,val,fn,mask) — one option inside TOGGLE/SELECT/CHOOSE;
/// val must be a compile-time constant. fn/mask are accepted but ignored.
#define VALUE(label, val, fn, mask) \
  ::oneMenu::ItemDef<::oneMenu::EnumValue<(val)>, ::oneData::Text>{label}

/// AM4 TOGGLE(var,id,label,fn,mask,style,...values) — Enter cycles to the
/// next VALUE(...) and writes it into var (DataRef). fn auto-dispatches on
/// its signature (am4compat::toggleDef); style is ignored.
#define TOGGLE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::toggleDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// AM4 SELECT(var,id,label,fn,mask,style,...values) — Enter opens an inline
/// picker; a second Enter commits the highlighted VALUE(...) into var
/// (DataRef). fn auto-dispatches on its signature; style is ignored.
#define SELECT(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::selectDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneMenu::AsLabel<::oneData::Text>, ::oneMenu::AsEditMode<>>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// AM4 CHOOSE(var,id,label,fn,mask,style,...values) — Enter opens a nested
/// level to browse VALUE(...) options; entering one commits it into var
/// (DataRef). fn auto-dispatches on its signature; style is ignored.
#define CHOOSE(var, id, label, fn, mask, style, ...) \
  auto id = ::am4compat::chooseDef<::oneData::DataRef<&(var)>, (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{label}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// AM4 MENU(id,text,fn,mask,style,...items) — declares a submenu. fn/mask
/// auto-dispatch on fn's signature; style's wrapStyle bit adds WrapNav.
#define MENU(id, text, fn, mask, style, ...) \
  auto id = ::am4compat::menuDefStyle<(style), (::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// AM4 PADMENU(id,text,fn,mask,style,...items) — declares a single-line/
/// pad-style menu. fn/mask auto-dispatch like MENU(); style is not forwarded.
#define PADMENU(id, text, fn, mask, style, ...) \
  auto id = ::am4compat::padDefStyle<(::oneMenu::EventMask)(mask), fn>( \
      ::oneMenu::ItemDef<::oneData::Text, ::oneMenu::AsEditMode<>>{text}, \
      ::oneMenu::staticBody(__VA_ARGS__))

/// @brief AM4 SUBMENU(id) — splices a previously MENU()-declared submenu into
///        the enclosing body. Move-only: see file comment "Known semantic gap".
#define SUBMENU(id) std::move(id)

/// AM4 OBJ(id) — splices a previously hand-declared item object into the
/// enclosing body. Move-only, same as SUBMENU().
#define OBJ(id) std::move(id)

/**
 * Device-wiring macros (MENU_INPUTS/MENU_OUTPUTS/NAVROOT/NAVROOT_IDLE),
 * built on oneMenu's InGroup/OutGroup/Pool.
 */

/// @brief AM4 MENU_INPUTS(id,&dev1,&dev2,...) — byte-for-byte AM4 syntax.
///        Builds a native oneMenu::InGroup over the given devices.
#define MENU_INPUTS(id, ...) \
  auto id = ::oneMenu::InGroup{__VA_ARGS__}

/// AM4 MENU_OUTPUTS(id,maxDepth,&dev1,&dev2,...) — builds an oneMenu::OutGroup
/// over the given devices; maxDepth is accepted but ignored.
#define MENU_OUTPUTS(id, maxDepth, ...) \
  auto id = ::oneMenu::OutGroup{__VA_ARGS__}

/// AM4 NAVROOT(id,menu,maxDepth,in,out) — declares the nav root; maxDepth is
/// accepted but ignored. in/out must be InGroup/OutGroup (from
/// MENU_INPUTS/MENU_OUTPUTS). id.poll() works like AM4's navRoot::poll().
#define NAVROOT(id, menu, maxDepth, in, out) \
  ::oneMenu::INavDef< \
      ::oneMenu::Pool<decltype(in), decltype(out)>, \
      ::oneMenu::EventDispatch, ::oneMenu::TreeNav, ::oneMenu::Root<menu> \
    > id(in, out)

/* NAVROOT_IDLE: opt-in idle-control bridge, binding a RunLoop<mainFn> into a
 * nav's INav::idling()/idleOn()/idleOff() (no-op on plain NAVROOT). */
namespace am4compat {
  // Run is an already-built oneMenu::RunLoop<mainFn> TYPE, not mainFn
  // re-templated here — avr-g++ 7.3 rejects re-deriving a function-reference
  // NTTP through a nested template instantiation (the same quirk
  // IdleTimeout and EDIT()'s editItem also have to work around); taking Run
  // as a type sidesteps it, same fix shape as IdleTimeout's own Run
  // parameter.
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
    // Same forwarding as the six above — required the moment oneMenu::INav
    // gained these (nav.h), since NavRootDef inherits INav directly
    // (separately from its own real TreeNav-based Base chain); without an
    // explicit override here the two branches' same-named methods are an
    // ambiguous base-class member on any NavRootDef instance.
    bool open() override { return Base::open(); }
    bool close() override { return Base::close(); }
    bool padOpen() override { return Base::padOpen(); }
    bool doNav(oneMenu::CKE cke, oneMenu::Sz len, bool w) override { return Base::doNav(cke,len,w); }
  };
}

/// NAVROOT variant binding idling()/idleOn()/idleOff() to a specific
/// already-built oneMenu::RunLoop<mainFn> type, passed as Run.
#define NAVROOT_IDLE(id, menu, maxDepth, in, out, Run) \
  ::am4compat::NavRootDef<Run, \
      ::oneMenu::Pool<decltype(in), decltype(out)>, \
      ::oneMenu::EventDispatch, ::oneMenu::TreeNav, ::oneMenu::Root<menu> \
    > id(in, out)

/// AM4 NONE — empty placeholder satisfying MENU_OUTPUTS'/MENU_INPUTS'
/// "at least 2 devices" syntax quirk. Global, unscoped.
#define NONE

/// Output-device macro for OneMenu's ANSI/console backend; `ANSI_OUT(id,w,h)`
/// declares id as a ready-to-use device. Not byte-for-byte AM4 syntax (AM4
/// has no native console backend to match).
#define ANSI_OUT(id, w, h) \
  ::oneMenu::OutDef< \
      ::oneMenu::FullPrinter, ::oneMenu::ANSIFmt, ::oneMenu::DataParser<>, ::oneMenu::CtrlChars, \
      ::oneMenu::ColorTrack<int>, ::oneMenu::Cursor<>, ::oneMenu::Gate, \
      ::oneMenu::ANSIOut, ::oneMenu::ConsoleOut, ::oneMenu::StaticPos<0,0>, ::oneMenu::StaticArea<(w),(h)> \
    > id

#ifdef ARDUINO
/// AM4 SERIAL_OUT(port) — inline Arduino serial output device, usable
/// directly inside a MENU_OUTPUTS(...) argument list. port is accepted but
/// ignored (bound to the global Serial object). Fixed 40x6 area. Arduino-only.
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

/// AM4 ANSISERIAL_OUT(port,colors,panels...) — ANSI-colored serial output
/// device; port/panels are ignored. colors must be a compile-time
/// Color<int>::Table<...> type, not AM4's runtime colorDef<uint8_t>[6] array.
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
  /// AM4 serialIn(Stream&) — input device wrapper over the compile-time
  /// Serial+key-parser chain. The Stream& constructor arg is accepted for
  /// AM4 syntax fidelity but ignored (bound to the global Serial object).
  struct serialIn : ::oneMenu::InDef<::oneMenu::SerialIn, ::oneMenu::IdxParser, ::oneMenu::PCKbd> {
    serialIn(Stream&) {}
  };
}
#endif

namespace am4compat {
  /// Adapts oneMenu::RunLoop into AM4-style idle triggering. TimeoutMs is
  /// the inactivity window (ms); Run must be an already-built
  /// oneMenu::RunLoop<mainFn> type.
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
