/**
 * @file nativeOutBridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief The reverse direction of `menuOutBridge.h`: instead of OneMenu's own
 *        rendering driving a real AM4 hardware driver, this lets a real,
 *        possibly still-unported AM4 sketch's own rendering (its own
 *        `printMenu()`/paging logic, genuinely built against the real
 *        upstream `Menu::menuOut` interface) drive a native OneMenu output
 *        backend instead — text or graphics, any `IOutDef<...>`-built chain.
 *
 * Direction check, since the name alone doesn't signal it (same `am4compat`
 * namespace as `MenuOutBridge`/`MenuInBridge`, deliberately paired naming:
 * `Menu*` = targets AM4, `Native*` = targets OneMenu-native):
 *   - `MenuOutBridge<W,H>` (menuOutBridge.h): OneMenu rendering -> real AM4
 *     hardware driver (`Menu::menuOut&`), reached through `oneMenu::
 *     LcdDisplay<LCD>`'s static-trait contract.
 *   - `NativeOutBridge<W,H>` (this file): real AM4 rendering -> native
 *     OneMenu output (`oneMenu::IOut&`), reached by literally BEING a real
 *     `Menu::menuOut`-derived object, constructed with a target `IOut&`.
 *
 * Confirmed directly against AM4's real `menuIo.h`/`menuIo.cpp`: `Menu::
 * cursorOut` already implements `clearLine()`/`clear(idx_t panelNr)`/
 * `fill()` in terms of `setCursor()`+`write()` — deriving from `cursorOut`
 * (not raw `Menu::menuOut`) means only THREE real overrides are needed here
 * (`write`, `setCursor`, `clear()` 0-arg); `drawCursor()`/`startCursor()`/
 * `endCursor()` are also inherited unmodified — their AM4 default bodies do
 * real, useful work (selection cursor / edit brackets) purely via `write()`,
 * which already routes correctly through this bridge.
 *
 * Fidelity boundary, by explicit scope decision (not extending `oneMenu::
 * IOut` with new virtuals this round): `setColor`/`rect`/`box` have nowhere
 * to go — `IOut` has no color/box/rect primitive (all color/cursor/pixel
 * richness in OneMenu's own output chain is absorbed by the compile-time
 * `Chain<>` *before* the type-erasure boundary; nothing extra ever crosses
 * it, text or graphics backend alike). Overridden here as EXPLICIT empty
 * bodies rather than left to inherit AM4's own already-empty defaults —
 * third-party base-class defaults aren't a contract and could change shape
 * between AM4 versions; an explicit override here makes the gap visible
 * exactly where a reader would look for it.
 *
 * Object-based (constructor-injected `IOut&`), not `MenuOutBridge`'s static-
 * trait/`begin()` idiom — deliberate: `MenuOutBridge` has to match `oneMenu::
 * LcdDisplay<LCD>`'s static-trait contract (every IOP device trait in this
 * codebase is called on the type, no instance); `NativeOutBridge` has to BE
 * a real, instantiated, address-taken `Menu::menuOut` object instead (placed
 * by pointer into a real AM4 sketch's own `menuOut*[]` device array or
 * `MENU_OUTPUTS(...)`) — there's no static-trait shape to match on that
 * side. This also lifts `MenuOutBridge`'s "one binding at a time" limitation
 * — multiple `NativeOutBridge` instances can coexist, each wrapping a
 * different target.
 *
 * Composition contract: `target` must be an `IOutDef<...>`-built `IOut&`,
 * not a plain `OutDef<...>` (no vtable) — the same "compat-only vtable
 * cost, opt-in" price `am4.h`'s own header comment already documents for
 * this kind of runtime-dispatch bridging.
 *
 * Usage:
 *   oneMenu::IOutDef<... any real OutDef chain ...> devOut;
 *   am4compat::NativeOutBridge<40,10> bridge(devOut);
 *   MENU_OUTPUTS(out, MAX_DEPTH, &bridge, NONE);   // AM4-side wiring
 */
#pragma once

// <menuDefs.h>, NOT <menu.h> or a bare <nav.h>:
//  - Not <menu.h>: this repo's OWN OneMenu/include/menu.h (the AM5 forward
//    shim) shares that exact filename with the real upstream AM4 library's
//    own src/menu.h — angle-bracket lookup between two identically-named
//    headers on the same include path is fragile (already flagged, see
//    project_am4_compat_layer memory's "AM5 — the real drop-in package"
//    entry) and this file sits inside OneMenu itself, right where that
//    collision would bite hardest.
//  - Not a bare <nav.h>: nav.h's own menuNode/navTarget/etc. are only
//    complete once menuDefs.h's include chain (macros.h/menuBase.h/
//    shadows.h/items.h/menuIo.h/nav.h, in that order) has already run —
//    confirmed empirically (a bare `#include <nav.h>` first hits nav.h
//    with menuNode still incomplete). menuOutBridge.h's own bare
//    `#include <nav.h>` only compiles because every existing consumer
//    (lcdBridge.h, u8g2Bridge.h) already pulls in a driver header that
//    goes through the proper chain first; this file has no such
//    prerequisite, so it needs the real chain itself. <menuDefs.h> is the
//    real upstream library's own uniquely-named entry point for that chain
//    (its own <menu.h> is just menuDefs.h + itemsTemplates.hpp, the latter
//    only needed for building AM4 items via MENU()/FIELD(), not for this
//    file's own IOut-level bridging).
#include <menuDefs.h>  // Menu::cursorOut, Menu::colorDefs/status, Menu::panel/panelsList
#include <oneMenu/compat/am4/menuOutBridge.h>  // reuse am4compat::DummyPanel<W,H>
#include <oneMenu/menu/out.h>                   // oneMenu::IOut

namespace am4compat {

  template<uint8_t W, uint8_t H>
  struct NativeOutBridge : Menu::cursorOut {
    oneMenu::IOut& target;

    explicit NativeOutBridge(oneMenu::IOut& t)
      : Menu::cursorOut(DummyPanel<W,H>::tops, DummyPanel<W,H>::panels), target(t) {}

    size_t write(uint8_t c) override { target.put((char)c); return 1; }

    void setCursor(Menu::idx_t x, Menu::idx_t y, Menu::idx_t /*panelNr*/ = 0) override {
      target.setPos({(oneMenu::Sz)x, (oneMenu::Sz)y});
    }

    void clear() override { target.clear(); }
    // clear(idx_t panelNr) / clearLine(...) / fill(...): inherited from
    // Menu::cursorOut, built from setCursor()+write() above — see file
    // header comment.

    // ── LCD fidelity boundary: nothing on IOut to forward these to ───────
    void setColor(Menu::colorDefs, bool = false, Menu::status = Menu::enabledStatus, bool = false) override {}
    void rect(Menu::idx_t, Menu::idx_t, Menu::idx_t, Menu::idx_t = 1, Menu::idx_t = 1,
              Menu::colorDefs = Menu::bgColor, bool = false, Menu::status = Menu::enabledStatus, bool = false) override {}
    void box(Menu::idx_t, Menu::idx_t, Menu::idx_t, Menu::idx_t = 1, Menu::idx_t = 1,
             Menu::colorDefs = Menu::bgColor, bool = false, Menu::status = Menu::enabledStatus, bool = false) override {}
  };

} // namespace am4compat
