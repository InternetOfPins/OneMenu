#pragma once
#ifdef ARDUINO
#include "oneMenu/menu/out.h"

namespace oneMenu {

  // OneMenu output for SSD1306-style char-based OLED displays.
  // Oled must provide: begin(), clear(), print(char), print(const char*), setCursor(col, row).
  // Place DataParser<> above in the chain to convert numeric types to chars.
  // resume() repositions to (0,0) without clearing — eliminates full-redraw blink.
  // setColors() is a stub for future invert/highlight support.
  template<typename Oled>
  struct OledOut : aRawDevice {
    template<typename O>
    struct _Part : PartialDraw::template Part<O> {
      using Base = typename PartialDraw::template Part<O>;
      static void put(char c)               { Oled::print(c); }
      static void put(const char* s)        { Oled::print(s); }
      static void put(const char* s, Sz n)  { for(Sz i=0;i<n&&s[i];i++) Oled::print(s[i]); }
      static void nl()                      { Oled::print('\n'); O::nl(); }
      static void setPos(const Pos& p)      { Oled::setCursor(p.x, p.y); }
      static void clear()                   { Oled::clear(); O::clear(); }
      static void resume()                  { Oled::setCursor(0, 0); O::resume(); }
      template<typename Cor>
      static void setColors(Cor, Cor)       {}  // stub: SSD1306 invert per-row when ready
      static constexpr void flush() {}
    };
    template<typename O> using Part = Raw::Part<DeviceCursor::template Part<_Part<O>>>;
  };

} // namespace oneMenu
#endif
