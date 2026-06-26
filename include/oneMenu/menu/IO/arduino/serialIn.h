#pragma once

#include "oneMenu/menu/in.h"
#ifdef ARDUINO
  #include <Arduino.h>
#endif

namespace oneMenu {

  // Generic UART input: Dev must expose .available() and .read()
  template<typename Dev, Dev& dev>
  struct UartIn {
    template<typename In>
    struct Part : In {
      static bool available() { return In::available() || dev.available(); }
      static CKE cmd() {
        if (In::available()) return In::cmd();
        return available() ? In::parseKey(Key(dev.read())) : In::cmd();
      }
    };
  };

  // Arduino Serial input: inherits UartIn on device; on PC uses peek() for test streams
  template<typename Dev, Dev& dev>
  struct ArduinoSerialIn {
    template<typename In>
    struct Part : UartIn<Dev, dev>::template Part<In> {
      #ifndef ARDUINO
        static bool available() { return In::available() || dev.peek(); }
        static CKE cmd() {
          if (In::available()) return In::cmd();
          return available() ? In::parseKey(Key(dev.read())) : In::cmd();
        }
      #endif
    };
  };

  using SerialIn = ArduinoSerialIn<decltype(Serial), Serial>;

} // namespace oneMenu
