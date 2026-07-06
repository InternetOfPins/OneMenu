#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // OneInput analog-joystick InDef adapter.
  //
  // HW is the assembled hardware chain (oneInput::InputDef<Joystick<...>, AvrAdcAxes<...>,
  // hw::avr::AVRAdc<...>>). JoyIn<HW> is a HAPI component that sits in InDef and polls
  // HW::dx()/HW::dy() — signed "ticks past deadzone, repeat-gated" deltas, the analog-axis
  // analogue of Encoder::delta() (EncIn) produced by deadzone+repeat filtering instead of
  // quadrature decode. X maps to Left/Right, Y maps to Up/Down — same Cmd pairing doOutput's
  // wraparound nav already uses for a physical D-pad/encoder.
  //
  //   using JoyHW = oneInput::InputDef<
  //     oneInput::Joystick<512,96,200>,
  //     oneInput::avr::AvrAdcAxes<0,1>,
  //     hw::avr::AVRAdc<>
  //   >;
  //   InDef<oneMenu::JoyIn<JoyHW>, oneMenu::BtnIn<SwHW>, PCKbd> in;
  //
  // A joystick module's own center push-button (or any second digital button beside it)
  // composes unchanged via the existing oneMenu::BtnIn/oneInput::BtnCapture chain — only the
  // 2 analog axes needed new machinery (Joystick, AvrAdcAxes, hw::avr::AVRAdc).
  /// @brief OneInput→menu analog-joystick adapter; polls HW::dx()/dy() and emits CKE Left/Right/Up/Down
  template<typename HW>
  struct JoyIn {
    template<typename O>
    struct Part : O {
      static bool available() {
        return HW::dx() != 0 || HW::dy() != 0 || O::available();
      }
      static CKE cmd() {
        int8_t x = HW::dx();
        if (x > 0) { HW::consumeX( 1); return {Cmd::Right}; }
        if (x < 0) { HW::consumeX(-1); return {Cmd::Left};  }
        int8_t y = HW::dy();
        if (y > 0) { HW::consumeY( 1); return {Cmd::Down}; }
        if (y < 0) { HW::consumeY(-1); return {Cmd::Up};   }
        return O::cmd();
      }
    };
  };

} // oneMenu
