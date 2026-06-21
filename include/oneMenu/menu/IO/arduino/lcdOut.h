#pragma once
#ifdef ARDUINO
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/textFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // LcdOut<LCD, lcd>: raw output device for HD44780-compatible character LCDs.
  // Templates on a reference to the user's LCD object (LiquidCrystal_I2C, hd44780, etc.)
  // — same pattern as StreamOut<Out, out>. No host buffer; streams directly to hardware.
  // LCD must provide: print(char), setCursor(col, row), clear().
  template<typename LCD, LCD& lcd>
  struct LcdOut : aRawDevice {
    template<typename O>
    struct _Part : O {
      inline static uint8_t _col = 0;
      inline static uint8_t _row = 0;

      static void put(char c) { lcd.print(c); _col++; }
      static void put(const char* s) { while(*s) { lcd.print(*s++); _col++; } }
      static void put(const char* s, Sz n) {
        for(Sz i = 0; i < n && s[i]; i++) put(s[i]);
      }
      static void nl() {
        _col = 0; _row++;
        lcd.setCursor(0, _row);
        O::nl();
      }
      static void setPos(const Pos& p) {
        _col = p.x; _row = p.y;
        lcd.setCursor(p.x, p.y);
      }
      static void clear()  { lcd.clear(); _col = 0; _row = 0; O::clear(); }
      static void resume() { _col = 0; _row = 0; lcd.setCursor(0, 0); O::resume(); }
      static constexpr void flush()        {}
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // LcdDisplay<LCD, lcd, cols, rows>: ready-made OutDef for character LCDs.
  // Usage:
  //   LiquidCrystal_I2C lcd(0x27, 20, 4);
  //   LcdDisplay<decltype(lcd), lcd, 20, 4> lcdDisplay;
  template<typename LCD, LCD& lcd, Sz cols, Sz rows>
  using LcdDisplay = OutDef<
    FullPrinter,
    TextFmt,
    DataParser<>,
    Cursor<1, 1>,
    LcdOut<LCD, lcd>,
    StaticPos<0, 0>,
    StaticArea<cols, rows>
  >;

} // namespace oneMenu
#endif
