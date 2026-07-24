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
      // Bounded string print — needed by TextField's own cursor-split
      // rendering (sys/fields.h's PartEnd::printItem, "&text[0],i" /
      // "text,sz") and by Gate's own put(const char*,Sz) forwarding
      // (out.h) when composed above this device. A plain single-arg
      // put(T) doesn't cover this: Arduino's Print (HardwareSerial's own
      // base) already provides a length-bounded write(), reused here
      // rather than looping char-by-char.
      static void put(const char* s, Sz n) {dev.write((const uint8_t*)s,n);O::put(s,n);}
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


