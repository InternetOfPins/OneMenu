#pragma once
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/textFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // LcdDisplay<LCD>: raw output device for HD44780-compatible character LCDs.
  // Same slot as ConsoleOut/SerialOut — place inside OutDef, swap freely.
  // LCD must provide: print(char), print(const char*), setCursor(col,row), clear().
  // LCD must expose static constexpr cols and rows — satisfies IsArea.
  /// @brief raw output device adapter for HD44780-compatible character LCDs
  template<typename LCD>
  struct LcdDisplay : aRawDevice, anArea {
    template<typename O>
    struct _Part : O {
      using IsArea = std::true_type;
      static constexpr Sz width()  { return LCD::cols; }
      static constexpr Sz height() { return LCD::rows; }
      static constexpr Area area() { return {LCD::cols, LCD::rows}; }
      static constexpr Sz  orgX()  { return 0; }
      static constexpr Sz  orgY()  { return 0; }
      static constexpr Pos org()   { return {0, 0}; }

      inline static uint8_t _row = 0;

      static void put(char c)              { LCD::print(c); }
      static void put(const char* s)       { LCD::print(s); }
      static void put(const char* s, Sz n) { for(Sz i = 0; i < n && s[i]; i++) LCD::print(s[i]); }
      static void nl()                     { LCD::setCursor(0, ++_row); O::nl(); }
      static void setPos(const Pos& p)     { _row = p.y; LCD::setCursor(p.x, p.y); }
      static void clear()                  { LCD::clear(); _row = 0; O::clear(); }
      // static void resume()                 { _row = 0; LCD::setCursor(0, 0); O::resume(); }//Cursor will do this with setPos
      static constexpr void flush()        {}
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // LcdOut<Lcd, Printer>: composed OutDef for character LCDs — the central piece.
  // Printer defaults to NoTitleScrollPrinter; use NoTitlePrinter for tiny devices.
  template<typename Lcd, typename Printer = NoTitleScrollPrinter>
  struct LcdOut : OutDef<
    Printer,
    TextFmt,
    DataParser<>,
    Cursor<1, 1>,
    LcdDisplay<Lcd>,
    StaticPos<0, 0>
  > {};

} // namespace oneMenu
