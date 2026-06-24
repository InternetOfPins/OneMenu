#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // OneInput button InDef adapter.
  //
  // HW is the assembled hardware chain (hapi::APIOf<InputDef, BtnCapture, Hold<800>, ...>).
  // BtnIn<HW, ClickCmd, HoldCmd> is a HAPI component that sits in InDef and drains HW events.
  // BtnCapture (in OneInput) captures onClick(1)/onHold(2) flags; BtnIn maps them to CKE.
  //
  //   using BtnHW = hapi::APIOf<oneInput::InputDef,
  //     oneInput::BtnCapture,
  //     oneInput::Hold<800>,
  //     oneInput::Click<300>,
  //     oneInput::Debounce<20>,
  //     oneInput::avr::AvrBtnPin<1, chip::PortC, 2>
  //   >;
  //   ISR(PCINT1_vect) { BtnHW::dispatch(); }
  //
  //   InDef<oneMenu::BtnIn<BtnHW>, PCKbd, SerialIn> in;
  /// @brief OneInput→menu button adapter; maps BtnCapture flags to CKE (click→Enter, hold→Esc)
  template<typename HW, Cmd ClickCmd = Cmd::Enter, Cmd HoldCmd = Cmd::Esc>
  struct BtnIn {
    template<typename O>
    struct Part : O {
      static bool available() {
        HW::available();              // drives Hold::check() time poll
        return HW::pending() != 0 || O::available();
      }
      static CKE cmd() {
        HW::available();              // allow Hold to fire if threshold just crossed
        uint8_t ev = HW::take();      // consume pending event
        if (ev == 1) return {ClickCmd};
        if (ev == 2) return {HoldCmd};
        return O::cmd();
      }
    };
  };

} // oneMenu
