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
      template<typename F>
      static void setFont(F)                {}
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

  // HasSetColors<T>: detects a real setColors(uint16_t,uint16_t) on the Oled
  // driver (AdaGfxVendor/AdaGfxBufferedVendor have one; U8g2Vendor doesn't) —
  // same local-trait idiom as HasBody (nav.h)/HasNavOnEvent (item.h). Gates
  // VendorGfxOut::setColors's forward below so a driver with no real color
  // support keeps no-op'ing safely instead of failing to compile.
  template<typename T, typename = void>
  struct HasSetColors : std::false_type {};
  template<typename T>
  struct HasSetColors<T, std::void_t<decltype(std::declval<T&>().setColors(uint16_t{}, uint16_t{}))>>
    : std::true_type {};

  // HasSetFont<T,F>: detects a real setFont(F) on the Oled driver (AdaGfxVendor/
  // AdaGfxBufferedVendor have one, taking a vendor font-pointer type; native
  // Ssd1306/PCD8544/U8g2Vendor don't) — same gated-forward idiom as HasSetColors,
  // generic over F so this header never has to name Adafruit_GFX's GFXfont type.
  template<typename T, typename F, typename = void>
  struct HasSetFont : std::false_type {};
  template<typename T, typename F>
  struct HasSetFont<T, F, std::void_t<decltype(std::declval<T&>().setFont(std::declval<F>()))>>
    : std::true_type {};

  // HasArea<T>: detects a real width() on T — used below to tell whether
  // something further down the chain (conventionally StaticArea, an
  // "settings leaf" composed last/innermost by every existing chain in this
  // codebase) already supplies IsArea, so VendorGfxOut's own default below
  // only kicks in when nothing else already did.
  template<typename T, typename = void>
  struct HasArea : std::false_type {};
  template<typename T>
  struct HasArea<T, std::void_t<decltype(T::width())>> : std::true_type {};

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
  struct VendorGfxOut : aRawDevice, aFillRect, anArea {
    // VendorGfxOut now doubles as the default area source (width()/height()
    // below default to Oled::kWidth/kHeight) — validate those same two
    // values here, matching StaticArea's own w>0/h>0 validation (out.h),
    // since a degenerate Oled<0,...> or Oled<...,0> would otherwise surface
    // as a much more confusing failure downstream (e.g. Cursor::free()
    // always reading negative).
    static_assert(Oled::kWidth  > 0, "VendorGfxOut<Oled>: Oled::kWidth must be positive");
    static_assert(Oled::kHeight > 0, "VendorGfxOut<Oled>: Oled::kHeight must be positive");
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
      static void setColors(Cor f, Cor b)   { if constexpr(HasSetColors<Oled>::value) Oled::setColors(f, b); }
      template<typename F>
      static void setFont(F f)              { if constexpr(HasSetFont<Oled,F>::value) Oled::setFont(f); }
      static void flush()                   { Oled::flush(); }
      // native-unit GFX primitives — x/y/w/h all in raw pixels
      static void fillRect(Sz x, Sz y, Sz w, Sz h, uint8_t byte=0x00) { Oled::fillRect(x, y, w, h, byte); }
      static void drawRoundRect(Sz x, Sz y, Sz w, Sz r)  { Oled::drawRoundRect(x, y, w, r); }
      static constexpr Sz charWidth()   { return Oled::charWidth(); }
      static constexpr Sz lineSpacing() { return Oled::lineSpacing(); }
      // Default area, straight from the real device — Oled::kWidth/kHeight
      // are the actual panel size, not a stand-in a caller has to remember
      // to retype correctly. Forwards to O::width()/height() first when
      // something further down the chain already provides them (an explicit
      // StaticArea<w,h>, the deliberate-smaller-sub-region case) — found on
      // real ST7789 hardware: content genuinely isn't clipped to the
      // declared area, so a StaticArea smaller than the real device left a
      // dead zone no redraw could ever reach to clear. Matching this to the
      // real device by default closes that class of bug at its root; an
      // explicit StaticArea composed anywhere in the chain still overrides
      // it exactly as before.
      using IsArea = std::true_type;
      static constexpr Sz width()  { if constexpr(HasArea<O>::value) return O::width();  else return Oled::kWidth;  }
      static constexpr Sz height() { if constexpr(HasArea<O>::value) return O::height(); else return Oled::kHeight; }
      static constexpr Area area() { return {width(), height()}; }
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

  // Ready-made OutDef for buffered, pixel-addressed vendor GFX devices.
  // Same shape as OledDisplay, using VendorGfxOut instead of OledOut. No
  // trailing StaticArea here (unlike OledDisplay) — VendorGfxOut itself
  // defaults width()/height() to Oled::kWidth/kHeight (see its own comment
  // above), so this alias does not need to repeat it. There is only ever one
  // real StaticArea source in the chain this way: an Extra...-supplied
  // StaticArea still wins (VendorGfxOut checks HasArea<O> and forwards to
  // it), with no risk of a second, redundant StaticArea coexisting unguarded.
  template<typename Oled, typename GfxFmtT=GfxFmt<>, typename... Extra>
  using VendorGfxDisplay = OutDef<
    FullPrinter,
    GfxFmtT,
    DataParser<>,
    Cursor<Oled::charWidth(), Oled::lineSpacing()>,
    VendorGfxOut<Oled>,
    Extra...,
    StaticPos<0, 0>
  >;

} // namespace oneMenu
