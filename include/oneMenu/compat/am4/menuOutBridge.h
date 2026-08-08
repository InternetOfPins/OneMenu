/**
 * @file menuOutBridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4 output-driver reuse — the generic core. Adapts OneMenu's own
 *        output calls onto AM4's real `Menu::menuOut` base interface,
 *        instead of a fresh native OneMenu backend per display library.
 *
 * Design direction: keep AM4's own driver wrapper classes as-is (GPL,
 * user-contributed — don't touch the driver code), and adapt OneMenu's own
 * rendering calls onto the existing AM4 wrapper's API, targeting the base
 * interface rather than one adapter per concrete driver type: AM4's own
 * `write`/`setCursor`/`clear`/`setColor`/`box`/`rect`/`clearLine` are
 * already **virtual on the `Menu::menuOut` base itself** (`menuIo.h` —
 * `write` via `Print`, the rest declared directly on `menuOut`;
 * `Menu::gfxOut`/`Menu::cursorOut` don't add new virtuals beyond that, just
 * state and overrides). That means ONE bridge, written against
 * `Menu::menuOut&`, works polymorphically for *every* AM4 `menuIO` driver —
 * `liquidCrystalOut`, `u8g2Out`, `adaGfxOut`, `UCGLibOut`, `TFT_eSPIOut`,
 * `SSD1306AsciiOut`, `OzOledAsciiOut`, ... — without a new adapter struct
 * per driver. Adding support for a new AM4 driver from here on is
 * "construct it, upcast to `Menu::menuOut&`, call
 * `MenuOutBridge<W,H>::begin(driver)`" — no new bridge code.
 *
 * `Menu::menuOut`-derived drivers all need `idx_t* tops` + `panelsList&` in
 * their constructor — AM4's own multi-panel/scroll bookkeeping. This bridge
 * never calls AM4's own high-level `printMenu()`/paging logic (OneMenu's
 * own `NoTitleScrollPrinter`/`Cursor`/`TextFmt` chain, composed by the
 * existing `oneMenu::LcdOut`, already does all of that) — only the leaf
 * virtuals (`write`/`setCursor`/`clear`). `DummyPanel<W,H>` below is the
 * shared, driver-agnostic minimal stand-in every concrete driver's
 * constructor needs; its content is never read once construction succeeds.
 *
 * Real AM4 dependency, not this repo's own am4.h shim: `Menu::` here is the
 * *real* upstream ArduinoMenu library's namespace, not am4.h's own thin
 * syntax-compat `namespace Menu{ enum EventMask...; }` shim — a project
 * embedding this bridge needs the real AM4 library as a dependency (e.g. a
 * `lib_deps=ArduinoMenu=...` entry), separate from am4.h itself.
 * Deliberately kept out of am4.h's own unconditional include chain — only a
 * sketch that actually wants this reuse includes it.
 */
#pragma once

#include <nav.h>  // Menu::menuOut/gfxOut/cursorOut, Menu::panel/panelsList/navNode, Menu::idx_t
#include <oneMenu/menu/IO/IOP/lcdOut.h>

namespace am4compat {

  /// Driver-agnostic dummy single-panel bookkeeping, to satisfy an AM4
  /// driver's own constructor; its content is never read.
  template<uint8_t W, uint8_t H>
  struct DummyPanel {
    static inline Menu::panel panel_{0, 0, (Menu::idx_t)W, (Menu::idx_t)H};
    static inline Menu::navNode* nodes_[1] = {nullptr};
    static inline Menu::panelsList panels{&panel_, nodes_, 1};
    static inline Menu::idx_t tops[1] = {0};
  };

  /// Satisfies oneMenu::LcdDisplay<LCD>'s contract, backed by any real,
  /// already-constructed AM4 Menu::menuOut-derived driver instance. W/H are
  /// the display's character grid size (columns, rows).
  template<uint8_t W, uint8_t H>
  struct MenuOutBridge {
    static constexpr uint8_t cols = W;
    static constexpr uint8_t rows = H;

    static inline Menu::menuOut* driver = nullptr;

    /// Binds this bridge to a real, already-begin()'d AM4 driver instance.
    /// Call once, before the first nav.printTo()/OutDef use.
    static void begin(Menu::menuOut& d) { driver = &d; }

    static void print(char c)        { driver->write((uint8_t)c); }
    static void print(const char* s) { while(*s) driver->write((uint8_t)*s++); }
    static void setCursor(uint8_t x, uint8_t y) { driver->setCursor((Menu::idx_t)x, (Menu::idx_t)y); }
    static void clear()              { driver->clear(); }
  };

  /// Ready-made OutDef for any AM4-menuOut-backed display. Usage:
  /// `Am4Out<20,4> devOut; MenuOutBridge<20,4>::begin(driver);`.
  template<uint8_t W, uint8_t H>
  using Am4Out = oneMenu::LcdOut<MenuOutBridge<W, H>>;

} // namespace am4compat
