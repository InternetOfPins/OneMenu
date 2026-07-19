#pragma once
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/gfxFmt.h"

namespace oneMenu {

  // OneMenu output for SSD1306-style OLED displays.
  // All coordinates are in device-native units: pixels horizontally, pages (8px) vertically.
  // Oled must provide: begin(), clear(), print(char), setCursor(col_px, page),
  //   fillRect(col_px, page, w_px, h_pages, byte), setInverted(bool), charWidth(), lineSpacing().
  // resume() resets invert state and repositions to (0,0) without clearing.
  /// @brief raw output device adapter for SSD1306-style OLED and Nokia 5110 pixel displays
  template<typename Oled>
  struct OledOut : aRawDevice, aFillRect {
    template<typename O>
    struct _Part : O {
      using Base             = O;
      using HasDrawRoundRect = std::true_type;
      static void put(char c)               { Oled::print(c); }
      static void put(const char* s)        { Oled::print(s); }
      static void put(const char* s, Sz n)  { for(Sz i=0;i<n&&s[i];i++) Oled::print(s[i]); }
      static void nl()                      { Oled::print('\n'); O::nl(); }
      static void setPos(const Pos& p)      { Oled::setCursor(p.x, p.y); }
      static void setBigFont(bool v)        { Oled::setBigFont(v); }
      static void clear()                   { Oled::clear(); Oled::setCursor(0,0); O::clear(); }
      static void setInverted(bool v)       { Oled::setInverted(v); }
      template<typename Cor>
      static void setColors(Cor, Cor)       {}
      static constexpr void flush()         {}
      // native-unit GFX primitives — col/w in pixels, row/h in pages
      static void fillRect(Sz col, Sz row, Sz w, Sz h, uint8_t byte=0x00) { Oled::fillRect(col, row, w, h, byte); }
      static void drawRoundRect(Sz col, Sz row, Sz w, Sz r)  { Oled::drawRoundRect(col, row, w, r); }
      // font metrics forwarded from driver — used by Cursor<> for position tracking
      static constexpr Sz charWidth()   { return Oled::charWidth(); }
      static constexpr Sz lineSpacing() { return Oled::lineSpacing(); }
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

  // Ready-made OutDef for SSD1306-style OLED.
  // Cursor advances and default area are derived from the Oled driver (kWidth/kHeight/charWidth/lineSpacing).
  // Extra... overrides position and/or area (earlier entry in HAPI chain wins over the defaults):
  //   OledDisplay<MyOled>                              — full display
  //   OledDisplay<MyOled, StaticArea<128, 4>>          — top 4 pages
  //   OledDisplay<MyOled, StaticPos<0,2>, StaticArea<128, 4>>  — sub-region at page 2
  // For non-default Radius/Spacing write a custom OutDef with an explicit GfxFmt<R,S>.
  // GfxFmtT: pass GfxFmt<Radius,Spacing,BigTitle> to customise; default is plain GfxFmt<>
  template<typename Oled, typename GfxFmtT=GfxFmt<>, typename... Extra>
  using OledDisplay = OutDef<
    FullPrinter,
    GfxFmtT,
    DataParser<>,
    Cursor<Oled::charWidth(), Oled::lineSpacing()>,
    OledOut<Oled>,
    Extra...,
    StaticPos<0, 0>,
    StaticArea<Oled::kWidth, Oled::kHeight / 8>
  >;

  // Nokia 5110 (PCD8544) — same contract as OledDisplay, 84×48, 6 pages.
  // Lcd: a PCD8544<Transport, Contrast> instance.
  template<typename Lcd, typename GfxFmtT=GfxFmt<>, typename... Extra>
  using Nokia5110Display = OutDef<
    FullPrinter,
    GfxFmtT,
    DataParser<>,
    Cursor<Lcd::charWidth(), Lcd::lineSpacing()>,
    OledOut<Lcd>,
    Extra...,
    StaticPos<0, 0>,
    StaticArea<84, 6>
  >;

  // VendorGfxOut<Oled>/VendorGfxDisplay<Oled,...>: for pixel-addressed GFX
  // vendor libraries (U8g2Vendor/AdaGfxVendor — OneIO/include/oneIO/
  // display/), NOT Ssd1306/PCD8544-style page-addressed native drivers.
  // Two real differences from OledOut/OledDisplay, both load-bearing, not
  // cosmetic:
  //  - Y is in raw pixels throughout (StaticArea<kWidth,kHeight>, not
  //    kHeight/8) — u8g2/Adafruit_GFX's own setCursor()/drawBox()/
  //    fillRect() all take plain pixel coordinates; there is no hardware
  //    page-addressing concept at this level (that's a Ssd1306-specific
  //    GDDRAM detail, already fully hidden inside the vendor library
  //    itself for these two).
  //  - flush() forwards to Oled::flush() (pushes a local RAM framebuffer to
  //    hardware, e.g. u8g2's own sendBuffer()) rather than OledOut's own
  //    no-op (correct only for a streaming, direct-to-GDDRAM driver like
  //    Ssd1306, where every write already reaches the display immediately).
  //    nav.h calls out.flush() exactly once per doOutput()/printTo() pass —
  //    the correct, and only, hook for a real "push the frame" call.
  /// @brief like OledOut<Oled>, but pixel-addressed (not page-addressed) and forwards flush() — for buffered vendor GFX libraries
  template<typename Oled>
  struct VendorGfxOut : aRawDevice, aFillRect {
    template<typename O>
    struct _Part : O {
      using Base             = O;
      using HasDrawRoundRect = std::true_type;
      static void put(char c)               { Oled::print(c); }
      static void put(const char* s)        { Oled::print(s); }
      static void put(const char* s, Sz n)  { for(Sz i=0;i<n&&s[i];i++) Oled::print(s[i]); }
      static void nl()                      { Oled::print('\n'); O::nl(); }
      static void setPos(const Pos& p)      { Oled::setCursor(p.x, p.y); }
      static void setBigFont(bool v)        { Oled::setBigFont(v); }
      static void clear()                   { Oled::clear(); Oled::setCursor(0,0); O::clear(); }
      static void setInverted(bool v)       { Oled::setInverted(v); }
      template<typename Cor>
      static void setColors(Cor, Cor)       {}
      static void flush()                   { Oled::flush(); }
      // native-unit GFX primitives — x/y/w/h all in raw pixels
      static void fillRect(Sz x, Sz y, Sz w, Sz h, uint8_t byte=0x00) { Oled::fillRect(x, y, w, h, byte); }
      static void drawRoundRect(Sz x, Sz y, Sz w, Sz r)  { Oled::drawRoundRect(x, y, w, r); }
      static constexpr Sz charWidth()   { return Oled::charWidth(); }
      static constexpr Sz lineSpacing() { return Oled::lineSpacing(); }
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

  // Ready-made OutDef for buffered, pixel-addressed vendor GFX devices.
  // Same shape as OledDisplay, using VendorGfxOut instead of OledOut and a
  // full-pixel-height StaticArea (not kHeight/8).
  template<typename Oled, typename GfxFmtT=GfxFmt<>, typename... Extra>
  using VendorGfxDisplay = OutDef<
    FullPrinter,
    GfxFmtT,
    DataParser<>,
    Cursor<Oled::charWidth(), Oled::lineSpacing()>,
    VendorGfxOut<Oled>,
    Extra...,
    StaticPos<0, 0>,
    StaticArea<Oled::kWidth, Oled::kHeight>
  >;

} // namespace oneMenu
