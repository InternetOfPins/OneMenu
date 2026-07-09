/**
 * @file u8g2Bridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4 output-driver reuse, gfx group — adapts OneMenu's own output
 *        calls onto AM4's real `Menu::u8g2Out` (menuIO/u8g2Out.h), instead
 *        of writing a fresh native OneMenu backend for the U8g2 library.
 *
 * See lcdBridge.h's own file header for the full "keep AM4's drivers as-is,
 * translate OneMenu calls onto them" rationale (Rui, 2026-07-09) — this
 * file is the gfx-group counterpart, same reasoning, not repeated here.
 *
 * Real finding that shaped this bridge's design (checked directly against
 * AM4's own source, not assumed): `Menu::u8g2Out::setCursor(x,y,panelNr)`
 * multiplies `(x,y)` by `resX`/`resY` *internally*
 * (`gfx.tx=(p.x+x)*resX+...`, `menuIO/u8g2Out.h`) — i.e. AM4's own u8g2
 * wrapper already presents a **character-grid** abstraction over the pixel
 * hardware (each cell is `resX×resY` px), not a raw-pixel/tile one. That
 * means it maps onto OneMenu's existing `oneMenu::LcdDisplay`/`LcdOut`
 * contract (character-cell `setCursor(col,row)`, `OneMenu/include/oneMenu/
 * menu/IO/IOP/lcdOut.h`) — the *same* target contract the text-group bridge
 * (lcdBridge.h) already uses — not onto `oneMenu::OledOut`'s raw-pixel/
 * 8px-tile contract (`oledOut.h`), whose `StaticArea<kWidth,kHeight/8>`
 * assumes true pixel/tile units that don't apply here. This is a
 * first-pass, text-only bridge (no fillRect/box/cursor-rect chrome) —
 * `u8g2Out` does support those (`clearLine`/`box`/`rect`/`drawCursor`, all
 * in the same resX/resY character-grid units), so a richer `GfxFmt`-based
 * composition is a real follow-up, not attempted here.
 *
 * Real AM4 dependency, not this repo's own am4.h shim — see lcdBridge.h's
 * own file header for what that distinction means and why this stays a
 * separate, opt-in header, out of am4.h's own include chain.
 */
#pragma once

#include <U8g2lib.h>
#include <menuIO/u8g2Out.h>
#include <oneMenu/menu/IO/IOP/lcdOut.h>

namespace am4compat {

  /// @brief a plain monochrome default color table (nColors=6: bg/fg/val/
  ///        unit/cursor/title) — 0=off,1=on for every slot. u8g2Out's own
  ///        constructor requires one; most real AM4 u8g2 sketches declare
  ///        an equivalent table themselves for a 1-bit display.
  inline const Menu::colorDef<uint8_t> defaultMonoColors[Menu::nColors] = {
    {{0,0},{1,1,1}}, {{0,0},{1,1,1}}, {{0,0},{1,1,1}},
    {{0,0},{1,1,1}}, {{0,0},{1,1,1}}, {{0,0},{1,1,1}}
  };

  /// @brief satisfies oneMenu::LcdDisplay<LCD>'s contract, backed by AM4's
  ///        real Menu::u8g2Out. W/H are the display's CHARACTER grid size
  ///        (columns, rows), not pixels — matches u8g2Out's own resX/resY
  ///        character-cell convention (see file header comment).
  template<uint8_t W, uint8_t H, uint8_t ResX = 6, uint8_t ResY = 9>
  struct U8g2Bridge {
    static constexpr uint8_t cols = W;
    static constexpr uint8_t rows = H;

    // dummy single-panel bookkeeping — same reason as LcdBridge's own
    // panel_/nodes_/panels_/tops_ (lcdBridge.h's file header comment):
    // never read, only exists to satisfy u8g2Out's own constructor.
    static inline Menu::panel panel_{0, 0, (Menu::idx_t)W, (Menu::idx_t)H};
    static inline Menu::navNode* nodes_[1] = {nullptr};
    static inline Menu::panelsList panels_{&panel_, nodes_, 1};
    static inline Menu::idx_t tops_[1] = {0};

    static inline Menu::u8g2Out* driver = nullptr;

    /// @brief binds this bridge to a real, already-begin()'d U8G2 instance.
    ///        Call once, before the first nav.printTo()/OutDef use.
    static void begin(U8G2& gfx, const Menu::colorDef<uint8_t> (&colors)[Menu::nColors] = defaultMonoColors) {
      static Menu::u8g2Out d(gfx, colors, tops_, panels_, ResX, ResY);
      driver = &d;
    }

    static void print(char c)        { driver->write((uint8_t)c); }
    static void print(const char* s) { while(*s) driver->write((uint8_t)*s++); }
    static void setCursor(uint8_t x, uint8_t y) { driver->setCursor((Menu::idx_t)x, (Menu::idx_t)y); }
    static void clear()              { driver->clear(); }
  };

  /// @brief ready-made OutDef for an AM4-u8g2Out-backed graphics display,
  ///        rendered as a plain character grid (first pass — see file
  ///        header comment on GfxFmt chrome as a real follow-up). Usage:
  ///        `Am4U8g2Out<16,4> devOut; U8g2Bridge<16,4>::begin(u8g2);`
  ///        (begin() called on the bridge TYPE directly — see lcdBridge.h's
  ///        own doc comment for why).
  template<uint8_t W, uint8_t H, uint8_t ResX = 6, uint8_t ResY = 9>
  using Am4U8g2Out = oneMenu::LcdOut<U8g2Bridge<W, H, ResX, ResY>>;

} // namespace am4compat
