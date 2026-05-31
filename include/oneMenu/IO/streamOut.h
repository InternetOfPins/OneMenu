#pragma once

#include "oneMenu/out.h"
#include <iostream>

template<typename Out,Out& out>
struct StreamOut {
  template<typename O>
  struct RawPart:oneOutput::StreamOut<Out,out>::Part<O> {
    static constexpr const Out& device{out};
    static constexpr void nl() {endl(out);}
    static constexpr void flush() {out.flush();}
    template<typename T> static constexpr void put(const T& o) {out<<o;O::put(o);}
  };
  template<typename O> using Part=typename Raw::template Part<RawPart<O>>;
};

using ConsoleOut=StreamOut<decltype(std::cout), std::cout>;
