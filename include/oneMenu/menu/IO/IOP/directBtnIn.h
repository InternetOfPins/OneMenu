#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // OneInput single-fixed-command button adapter — the "no click/hold
  // distinction" sibling of BtnIn<HW,ClickCmd,HoldCmd>. HW is a
  // PressCapture-terminated chain (oneInput::InputDef<PressCapture,
  // Debounce<Ms>, avr::AvrBtnPin<...>>), firing FixedCmd exactly once per
  // physical press. For a real dedicated-nav-button panel — one pin, one
  // fixed command each (Up/Down/Enter/Esc) — not a single multi-purpose
  // button distinguishing click vs. hold (that's BtnIn's own job).
  //
  //   using SelHW = oneInput::InputDef<
  //     oneInput::PressCapture,
  //     oneInput::Debounce<20>,
  //     oneInput::avr::AvrBtnPin<2, chip::PortD, 3>
  //   >;
  //   ISR(PCINT2_vect) { SelHW::dispatch(); UpHW::dispatch(); ... }
  //
  //   InDef<oneMenu::DirectBtnIn<SelHW,Cmd::Enter>,
  //         oneMenu::DirectBtnIn<UpHW,Cmd::Up>, ...> in;
  /// @brief OneInput->menu single-fixed-command button adapter (one pin, one Cmd, fires on press)
  template<typename HW, Cmd FixedCmd>
  struct DirectBtnIn {
    template<typename O>
    struct Part : O {
      static bool available() { return HW::available() || O::available(); }
      static CKE cmd() {
        if (HW::available() && HW::take()) return {FixedCmd};
        return O::cmd();
      }
    };
  };

} // oneMenu
