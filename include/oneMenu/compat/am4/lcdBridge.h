/**
 * @file lcdBridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4 output-driver reuse, text group — adapts OneMenu's own output
 *        calls onto AM4's real `Menu::liquidCrystalOut` (menuIO/
 *        liquidCrystalOut.h), instead of writing a fresh native OneMenu
 *        backend for HD44780-compatible character LCDs.
 *
 * Rui's direction (2026-07-09, notes.md "AM4 compat layer" — "AM4 output
 * device wrapper bridges"): AM4's ~30 `menuIO` driver wrapper headers already
 * solve real hardware/font integration work; keep them as-is (GPL,
 * user-contributed — don't touch the driver code itself) and build one
 * translation component per device *group* (text, gfx) that adapts
 * OneMenu's own rendering calls onto the existing AM4 wrapper's public API,
 * instead of a from-scratch native backend per display (the `oneIO::display
 * ::U8g2Spi`/`UcgSpi` precedent — real and working, but substantial
 * low-level HAL work repeated per display library).
 *
 * `LcdBridge<W,H>` satisfies `oneMenu::LcdDisplay<LCD>`'s own contract
 * (`OneMenu/include/oneMenu/menu/IO/IOP/lcdOut.h`: `print(char)`,
 * `print(const char*)`, `setCursor(col,row)`, `clear()`, static `cols`/
 * `rows`) — that contract already exists, generic, unrelated to AM4;
 * nothing new was needed on the OneMenu side. `LcdBridge` itself is the
 * only new piece: it forwards each of those calls onto a real, already
 * -constructed `Menu::liquidCrystalOut` instance (AM4's own class, backed
 * by Arduino's bundled `LiquidCrystal` library).
 *
 * `Menu::liquidCrystalOut`'s constructor needs `idx_t* tops` + a
 * `panelsList&` — AM4's own multi-panel/scroll bookkeeping. This bridge
 * never calls AM4's own high-level `printMenu()`/paging logic (OneMenu's
 * own `NoTitleScrollPrinter`/`Cursor`/`TextFmt` chain, composed by
 * `LcdOut`, already does all of that) — only the driver's *leaf* virtuals
 * (`write`/`setCursor`/`clear`). So `tops_`/`panels_` below exist purely to
 * satisfy the constructor; their content is never read once construction
 * succeeds. A single full-screen dummy panel is the minimal equivalent of
 * what AM4's own `LIQUIDCRYSTAL_OUT(id,maxDepth,n,device,...)` macro would
 * build via its `PANELS(...)` helper for one panel.
 *
 * Real AM4 dependency, not this repo's own am4.h shim: `Menu::` here is the
 * *real* upstream ArduinoMenu library's namespace (`menuIO/liquidCrystalOut
 * .h` and what it transitively includes), not am4.h's own thin syntax-compat
 * `namespace Menu{ enum EventMask...; }` shim — a project embedding this
 * bridge needs the real AM4 library as a dependency (same
 * `lib_deps=ArduinoMenu=...` convention already used by every `.RnD/
 * AM4check` "-original" harness), separate from (and in addition to) am4.h
 * itself. Deliberately kept out of am4.h's own unconditional include chain
 * — am4.h has no hard dependency on AM4's own driver library or on any
 * particular hardware library today; only a sketch that actually wants this
 * specific reuse includes this header.
 */
#pragma once

#include <LiquidCrystal.h>
#include <menuIO/liquidCrystalOut.h>
#include <oneMenu/menu/IO/IOP/lcdOut.h>

namespace am4compat {

  /// @brief satisfies oneMenu::LcdDisplay<LCD>'s contract, backed by AM4's
  ///        real Menu::liquidCrystalOut. W/H are the display's character
  ///        grid size (columns, rows) — matches LiquidCrystal::begin(W,H).
  template<uint8_t W, uint8_t H>
  struct LcdBridge {
    static constexpr uint8_t cols = W;
    static constexpr uint8_t rows = H;

    // dummy single-panel bookkeeping — see file header comment: never read,
    // only exists to satisfy Menu::liquidCrystalOut's own constructor.
    static inline Menu::panel panel_{0, 0, (Menu::idx_t)W, (Menu::idx_t)H};
    static inline Menu::navNode* nodes_[1] = {nullptr};
    static inline Menu::panelsList panels_{&panel_, nodes_, 1};
    static inline Menu::idx_t tops_[1] = {0};

    static inline Menu::liquidCrystalOut* driver = nullptr;

    /// @brief binds this bridge to a real, already-begin()'d LiquidCrystal
    ///        instance. Call once, before the first nav.printTo()/OutDef use.
    static void begin(LiquidCrystal& lcd) {
      static Menu::liquidCrystalOut d(lcd, tops_, panels_);
      driver = &d;
    }

    static void print(char c)        { driver->write((uint8_t)c); }
    static void print(const char* s) { while(*s) driver->write((uint8_t)*s++); }
    static void setCursor(uint8_t x, uint8_t y) { driver->setCursor((Menu::idx_t)x, (Menu::idx_t)y); }
    static void clear()              { driver->clear(); }
  };

  /// @brief ready-made OutDef for an AM4-liquidCrystalOut-backed character
  ///        LCD — same "just declare and go" shape as oneMenu::LcdOut.
  ///        Usage: `Am4LcdOut<20,4> devOut; LcdBridge<20,4>::begin(lcd);`
  ///        (lcd already real, already lcd.begin(20,4)'d by the caller) —
  ///        begin() is called on the bridge TYPE directly, same pattern
  ///        oneIO::display::U8g2Spi's own `Oled::begin()` already uses
  ///        (called on the underlying device trait, not through the
  ///        assembled OutDef instance — see examples/u8g2Oled/src/main.cpp).
  template<uint8_t W, uint8_t H>
  using Am4LcdOut = oneMenu::LcdOut<LcdBridge<W, H>>;

} // namespace am4compat
