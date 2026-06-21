#pragma once
#ifdef ARDUINO
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/gfxFmt.h"

namespace oneMenu {

  // OneMenu output for SSD1306-style OLED displays.
  // All coordinates are in device-native units: pixels horizontally, pages (8px) vertically.
  // Oled must provide: begin(), clear(), print(char), setCursor(col_px, page),
  //   fillRect(col_px, page, w_px, h_pages, byte), setInverted(bool), charWidth(), lineHeight().
  // resume() resets invert state and repositions to (0,0) without clearing.
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
      static void setInverted(bool v)       { Oled::setInverted(v); }
      template<typename Cor>
      static void setColors(Cor, Cor)       {}
      static constexpr void flush()         {}
      // native-unit GFX primitives — col/w in pixels, row/h in pages
      static void fillRect(Sz col, Sz row, Sz w, Sz h, uint8_t byte=0x00) { Oled::fillRect(col, row, w, h, byte); }
      static void drawRoundRect(Sz col, Sz row, Sz w, Sz r)  { Oled::drawRoundRect(col, row, w, r); }
      // font metrics forwarded from driver — used by Cursor<> for position tracking
      static constexpr Sz charWidth()  { return Oled::charWidth(); }
      static constexpr Sz lineHeight() { return Oled::lineHeight(); }
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

  // Ready-made OutDef for SSD1306-style OLED (font5x8: 6px/char, 1 page/line).
  // W and H are in device-native units: pixels wide, pages tall (128×8 = full 128×64 display).
  // Usage: OledDisplay<MyOled> disp;  or  OledDisplay<MyOled, 128, 8, 2, 1> disp;
  template<typename Oled, Sz W=128, Sz H=8, Sz Radius=2, Sz Spacing=0>
  using OledDisplay = OutDef<
    FullPrinter,
    GfxFmt<Radius, Spacing>,
    DataParser<>,
    Cursor<6, 1>,
    OledOut<Oled>,
    StaticPos<0,0>,
    StaticArea<W, H>
  >;

} // namespace oneMenu
#endif
