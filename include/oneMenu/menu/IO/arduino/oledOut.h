#pragma once
#ifdef ARDUINO
#include "oneMenu/menu/out.h"

namespace oneMenu {

  // OneMenu raw device for SSD1306-style OLED displays.
  // Oled must provide: begin(), clear(), print(char), print(const char*), setCursor(col, row).
  // Place DataParser<> above in the chain to convert numeric types to chars.
  template<typename Oled>
  struct OledOut : aRawDevice {
    template<typename O>
    struct _Part : O {
      using Base = O;
      static void put(char c)               { Oled::print(c); }
      static void put(const char* s)        { Oled::print(s); }
      static void put(const char* s, Sz n)  { for(Sz i=0;i<n&&s[i];i++) Oled::print(s[i]); }
      static void nl()                      { Oled::print('\n'); O::nl(); }
      static void setPos(const Pos& p)      { Oled::setCursor(p.x, p.y); }
      static void clear()                   { Oled::clear(); O::clear(); }
      static void resume()                  { Oled::clear(); O::resume(); }
      static constexpr void flush() {}
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

} // namespace oneMenu
#endif
