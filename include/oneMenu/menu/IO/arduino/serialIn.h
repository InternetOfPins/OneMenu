#pragma once

#include "oneMenu/menu/in.h"

template<typename Dev, Dev& dev>
struct ArduinoSerialIn {
  template<typename In>
  struct Part : In {
    #ifdef ARDUINO
      static bool available() { return In::available() || dev.available(); }
    #else
      static bool available() { return In::available() || dev.peek(); }
    #endif

    static oneMenu::CKE cmd() {
      if (In::available()) return In::cmd();  // drain inner pending first
      return available() ? In::parseKey(oneMenu::Key(dev.read())) : In::cmd();
    }
  };
};

using SerialIn = ArduinoSerialIn<decltype(Serial), Serial>;
