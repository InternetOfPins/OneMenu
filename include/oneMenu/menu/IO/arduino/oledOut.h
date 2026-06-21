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

  // Ready-made OutDef for SSD1306-style OLED.
  // Cursor advances and default area are derived from the Oled driver's kWidth/kHeight/charWidth()/lineHeight().
  // Extra... lets the user override position and/or area (earlier entry in HAPI chain wins):
  //   OledDisplay<MyOled>                              — full display, default pos
  //   OledDisplay<MyOled, 2, 0, StaticArea<128, 4>>   — top 4 pages, default pos
  //   OledDisplay<MyOled, 2, 0, StaticPos<0,2>, StaticArea<128, 4>>  — sub-region at page 2
  template<typename Oled, Sz Radius=2, Sz Spacing=0, typename... Extra>
  using OledDisplay = OutDef<
    FullPrinter,
    GfxFmt<Radius, Spacing>,
    DataParser<>,
    Cursor<Oled::charWidth(), Oled::lineHeight()>,
    OledOut<Oled>,
    Extra...,
    StaticPos<0, 0>,
    StaticArea<Oled::kWidth, Oled::kHeight / 8>
  >;

} // namespace oneMenu
#endif
