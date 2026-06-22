#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include "oneMenu/menu/out.h"

namespace oneMenu {

  template<typename Dev, Dev& dev>
  struct ArduinoSerialOut : aRawDevice {
    template<typename O>
    struct _Part:O {
      using Base=O;
      using This=_Part<O>;
      using Device=This;
      template<typename T> static void put(T o) {dev.print(o);O::put(o);}
      static void nl()               {dev.println();O::nl();}
      static void setPos(const Pos&) {}  // streaming — positioning is a no-op
      static constexpr void flush()  {}
    };
    template<typename O> using Part=Raw::Part<_Part<O>>;
  };

  using SerialOut=ArduinoSerialOut<decltype(Serial),Serial>;
  using DeviceOut=SerialOut;

} // namespace oneMenu
#endif


