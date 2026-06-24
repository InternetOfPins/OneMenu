#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // OneInput encoder InDef adapter.
  //
  // HW is the assembled hardware chain (hapi::APIOf<InputDef, Encoder, AvrEncPins<...>>).
  // EncIn<HW, Steps> is a HAPI component that sits in InDef and polls HW::delta().
  //
  //   using EncHW = hapi::APIOf<oneInput::InputDef,
  //     oneInput::Encoder,
  //     oneInput::avr::AvrEncPins<1, chip::PortC, 0, 1>
  //   >;
  //   ISR(PCINT1_vect) { EncHW::dispatch(); }
  //
  //   InDef<oneMenu::EncIn<EncHW, 4>, PCKbd, SerialIn> in;
  /// @brief OneInput→menu encoder adapter; polls HW::delta() and emits CKE Up/Down at Steps ticks
  template<typename HW, uint8_t Steps = 4>
  struct EncIn {
    template<typename O>
    struct Part : O {
      static bool available() {
        int8_t d = HW::delta();
        return d >= int8_t(Steps) || d <= -int8_t(Steps) || O::available();
      }
      static CKE cmd() {
        int8_t d = HW::delta();
        if (d >=  int8_t(Steps)) { HW::consume( int8_t(Steps)); return {Cmd::Up};   }
        if (d <= -int8_t(Steps)) { HW::consume(-int8_t(Steps)); return {Cmd::Down}; }
        return O::cmd();
      }
    };
  };

} // oneMenu
