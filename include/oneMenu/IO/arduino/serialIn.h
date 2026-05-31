#pragma once

#include "menu/in.h"

template<typename Dev,Dev& dev>
struct ArduinoSerialIn {
  template<typename In>
  struct Part:In {
    #ifdef ARDUINO
      static bool available() {return dev.available();}
    #else
      static bool available() {return dev.peek();}
    #endif
    template<typename Nav>
    static bool cmd(Nav& nav) {
      return available()?In::parseKey(nav,dev.read()):In::cmd(nav);
    }
  };
};

using SerialIn=ArduinoSerialIn<decltype(Serial),Serial>;

