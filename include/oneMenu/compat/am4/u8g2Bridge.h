/**
 * @file u8g2Bridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief Thin convenience constructor for `Menu::u8g2Out` (AM4's real U8g2
 *        graphics-display wrapper), on top of the generic
 *        `am4compat::MenuOutBridge` (menuOutBridge.h).
 *
 * The actual OneMenu-facing translation logic lives in menuOutBridge.h —
 * it's driver-agnostic (works over any `Menu::menuOut`-derived AM4 driver
 * via that base class's own virtual interface). This file only adds the
 * small, driver-specific piece: constructing a real `Menu::u8g2Out` (needs
 * a `U8G2&`, a color table, and AM4's own dummy-panel bookkeeping) and
 * binding it to the bridge.
 *
 * Real finding (checked directly against AM4's real source, not assumed):
 * `Menu::u8g2Out::setCursor(x,y,panelNr)` multiplies `(x,y)` by
 * `resX`/`resY` *internally* (`gfx.tx=(p.x+x)*resX+...`, `menuIO/
 * u8g2Out.h`) — i.e. AM4's own u8g2 wrapper already presents a
 * **character-grid** abstraction over the pixel hardware (each cell is
 * `resX×resY` px), not a raw-pixel one — which is exactly why it fits
 * `MenuOutBridge`'s `oneMenu::LcdDisplay`-shaped (character-cell) contract
 * directly, the same one every other AM4 driver (text or gfx) uses through
 * this same generic bridge.
 */
#pragma once

#include <U8g2lib.h>
#include <menuIO/u8g2Out.h>
#include <oneMenu/compat/am4/menuOutBridge.h>

namespace am4compat {

  /// @brief a plain monochrome default color table (nColors=6: bg/fg/val/
  ///        unit/cursor/title) — 0=off,1=on for every slot. u8g2Out's own
  ///        constructor requires one; most real AM4 u8g2 sketches declare
  ///        an equivalent table themselves for a 1-bit display.
  inline const Menu::colorDef<uint8_t> defaultMonoColors[Menu::nColors] = {
    {{0,0},{1,1,1}}, {{0,0},{1,1,1}}, {{0,0},{1,1,1}},
    {{0,0},{1,1,1}}, {{0,0},{1,1,1}}, {{0,0},{1,1,1}}
  };

  /// @brief constructs a real Menu::u8g2Out over `gfx` (already real,
  ///        already `gfx.begin()`'d by the caller) and binds it to
  ///        `MenuOutBridge<W,H>`. W/H are the display's CHARACTER grid size
  ///        (columns, rows), not pixels — matches u8g2Out's own resX/resY
  ///        character-cell convention (see file header comment). Call
  ///        once, before the first nav.printTo()/OutDef use.
  template<uint8_t W, uint8_t H, uint8_t ResX = 6, uint8_t ResY = 9>
  void beginU8g2(U8G2& gfx, const Menu::colorDef<uint8_t> (&colors)[Menu::nColors] = defaultMonoColors) {
    using Panel = DummyPanel<W, H>;
    static Menu::u8g2Out driver(gfx, colors, Panel::tops, Panel::panels, ResX, ResY);
    MenuOutBridge<W, H>::begin(driver);
  }

} // namespace am4compat
