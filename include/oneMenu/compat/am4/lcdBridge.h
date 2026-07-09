/**
 * @file lcdBridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief Thin convenience constructor for `Menu::liquidCrystalOut`
 *        (AM4's real HD44780-compatible character LCD wrapper), on top of
 *        the generic `am4compat::MenuOutBridge` (menuOutBridge.h).
 *
 * The actual OneMenu-facing translation logic lives in menuOutBridge.h —
 * it's driver-agnostic (works over any `Menu::menuOut`-derived AM4 driver
 * via that base class's own virtual interface, see its own file header
 * comment for why). This file only adds the small, driver-specific piece:
 * constructing a real `Menu::liquidCrystalOut` (needs a `LiquidCrystal&`
 * plus AM4's own dummy-panel bookkeeping) and binding it to the bridge.
 */
#pragma once

#include <LiquidCrystal.h>
#include <menuIO/liquidCrystalOut.h>
#include <oneMenu/compat/am4/menuOutBridge.h>

namespace am4compat {

  /// @brief constructs a real Menu::liquidCrystalOut over `lcd` (already
  ///        real, already `lcd.begin(W,H)`'d by the caller) and binds it to
  ///        `MenuOutBridge<W,H>`. Call once, before the first
  ///        nav.printTo()/OutDef use — same "begin() on the bridge type"
  ///        pattern menuOutBridge.h's own doc comment describes.
  template<uint8_t W, uint8_t H>
  void beginLcd(LiquidCrystal& lcd) {
    using Panel = DummyPanel<W, H>;
    static Menu::liquidCrystalOut driver(lcd, Panel::tops, Panel::panels);
    MenuOutBridge<W, H>::begin(driver);
  }

} // namespace am4compat
