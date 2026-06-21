#pragma once
#ifdef ARDUINO
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/gfxFmt.h"

namespace oneMenu {

  // OneMenu output for SSD1306-style char-based OLED displays.
  // Oled must provide: begin(), clear(), print(char), print(const char*), setCursor(col, row).
  // Place DataParser<> above in the chain to convert numeric types to chars.
  // resume() repositions to (0,0) without clearing — eliminates full-redraw blink.
  // GFX primitives (fillRect, drawRoundRect) exposed for GfxFmt — char coords, pixel conversion internal.
  template<typename Oled>
  struct OledOut : aRawDevice {
    template<typename O>
    struct _Part : PartialDraw::template Part<O> {
      using Base             = typename PartialDraw::template Part<O>;
      using HasFillRect      = std::true_type;
      using HasDrawRoundRect = std::true_type;
      static void put(char c)               { Oled::print(c); }
      static void put(const char* s)        { Oled::print(s); }
      static void put(const char* s, Sz n)  { for(Sz i=0;i<n&&s[i];i++) Oled::print(s[i]); }
      static void nl()                      { Oled::print('\n'); O::nl(); }
      static void setPos(const Pos& p)      { Oled::setCursor(p.x, p.y); }
      static void clear()                   { Oled::clear(); O::clear(); }
      static void resume()                  { Oled::setInverted(false); Oled::setCursor(0, 0); O::resume(); }
      static void setInverted(bool v)        { Oled::setInverted(v); }
      template<typename Cor>
      static void setColors(Cor, Cor)       {}
      static constexpr void flush()         {}
      // GFX primitives — char-unit coords converted to pixels internally (font5x8: 6px/char, 8px/row)
      static void fillRect(Sz col, Sz row, Sz w, Sz h, uint8_t byte=0x00) { Oled::fillRect(col*6, row, w*6, h, byte); }
      static void drawRoundRect(Sz col, Sz row, Sz w, Sz r)  { Oled::drawRoundRect(col*6, row, w*6, r); }
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

  // Ready-made OutDef for SSD1306-style OLED with GfxFmt selection highlight.
  // Usage: OledDisplay<MyOled> disp;  or  OledDisplay<MyOled,21,8,2,1> disp;
  template<typename Oled, Sz Cols=21, Sz Rows=8, Sz Radius=2, Sz Spacing=0>
  using OledDisplay = OutDef<
    FullPrinter,
    GfxFmt<Radius, Spacing>,
    DataParser<>,
    Cursor<>,
    OledOut<Oled>,
    StaticPos<0,0>,
    StaticArea<Cols, Rows>
  >;

} // namespace oneMenu
#endif
