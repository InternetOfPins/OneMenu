#pragma once
#include "oneMenu/menu/out.h"

namespace oneMenu {

  // UartSerialOut<Uart>: streaming serial output for bare-metal IOP UART components.
  // Same slot as ArduinoSerialOut — place inside OutDef, swap freely.
  // Uart must be an all-static IOP UART chain (e.g. chip::Serial0<9600>).
  template<typename Uart>
  struct UartSerialOut : aRawDevice {
    template<typename O>
    struct _Part : O {
      using Base = O;
      static void put(char c)              { Uart::write((uint8_t)c); Base::put(c); }
      static void put(const char* s)       { Uart::print(s); Base::put(s); }
      static void put(const char* s, Sz n) {
        for (Sz i = 0; i < n && s[i]; i++) Uart::write((uint8_t)s[i]);
        Base::put(s, n);
      }
      static void nl()                     { Uart::println(""); Base::nl(); }
      static void setPos(const Pos&)       {}
      static constexpr void flush()        {}
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

} // namespace oneMenu
