#pragma once
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/textFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // LcdOut<LCD>: raw output device for HD44780-compatible character LCDs.
  // LCD is a pure-static IOP type (e.g. oneIO::display::I2cLcd<TwiMaster, Addr>).
  // LCD must provide static: print(char), print(const char*), setCursor(col, row), clear().
  template<typename LCD>
  struct LcdOut : aRawDevice {
    template<typename O>
    struct _Part : O {
      inline static uint8_t _row = 0;

      static void put(char c)              { LCD::print(c); }
      static void put(const char* s)       { LCD::print(s); }
      static void put(const char* s, Sz n) { for(Sz i = 0; i < n && s[i]; i++) LCD::print(s[i]); }
      static void nl()                     { LCD::setCursor(0, ++_row); O::nl(); }
      static void setPos(const Pos& p)     { _row = p.y; LCD::setCursor(p.x, p.y); }
      static void clear()                  { LCD::clear(); _row = 0; O::clear(); }
      static void resume()                 { _row = 0; LCD::setCursor(0, 0); O::resume(); }
      static constexpr void flush()        {}
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // LcdDisplay<Lcd, cols, rows>: ready-made OutDef for character LCDs.
  // Usage:
  //   using MyLcd = oneIO::display::I2cLcd<AvrTwiMaster<>, 0x27>;
  //   LcdDisplay<MyLcd, 20, 4> lcdDisplay;
  //   // setup: MyLcd::begin();
  template<typename Lcd, Sz cols, Sz rows>
  using LcdDisplay = OutDef<
    FullPrinter,
    TextFmt,
    DataParser<>,
    Cursor<1, 1>,
    LcdOut<Lcd>,
    StaticPos<0, 0>,
    StaticArea<cols, rows>
  >;

} // namespace oneMenu
