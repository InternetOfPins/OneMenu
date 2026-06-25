#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // UartSerialIn<Uart>: UART serial input adapter for bare-metal IOP UART components.
  // Same slot as ArduinoSerialIn — place inside InDef, swap freely.
  // Uart must be an all-static IOP UART chain (e.g. chip::Serial0<9600>).
  template<typename Uart>
  struct UartSerialIn {
    template<typename In>
    struct Part : In {
      static bool available() { return In::available() || Uart::available(); }
      static CKE cmd() {
        if (In::available()) return In::cmd();
        return Uart::available() ? In::parseKey(Key(Uart::getch())) : In::cmd();
      }
    };
  };

} // namespace oneMenu
