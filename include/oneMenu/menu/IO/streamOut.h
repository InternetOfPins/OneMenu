#pragma once

#include "oneMenu/menu/out.h"
#include <iostream>

namespace oneMenu {
  template<typename Out,Out& out>
  struct StreamOut {
    template<typename O>
    struct RawPart:oneOutput::StreamOut<Out,out>::Part<O> {
      using Base=typename oneOutput::StreamOut<Out,out>::template Part<O>;
      using Base::put;
      static constexpr const Out& device{out};
      static constexpr void nl() {endl(out);}
      static constexpr void flush() {out.flush();}
      template<typename T> static constexpr void put(const T& o) {out<<o;O::put(o);}
      static constexpr void put(const char* s,Sz n) {out.write(s,n);}
    };
    template<typename O> using Part=typename Raw::template Part<RawPart<O>>;
  };

  using ConsoleOut=StreamOut<decltype(std::cout), std::cout>;
  using DeviceOut=ConsoleOut;
}