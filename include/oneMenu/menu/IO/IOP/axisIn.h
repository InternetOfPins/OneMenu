#pragma once
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // OneInput single-axis InDef adapter — the 1-axis sibling of JoyIn<HW>
  // (joyIn.h). HW is the assembled hardware chain (oneInput::InputDef<
  // AnalogAxis<...>, avr::AvrAdcAxis<Ch>, hw::avr::AVRAdc<...>>, optionally
  // InvDelta<...>-wrapped for mount/wiring correction). AxisIn<HW> polls
  // HW::delta() — a signed "ticks past deadzone, repeat-gated" delta, same
  // shape as JoyIn's own HW::dx()/dy() — and maps it to PosCmd/NegCmd.
  //
  // Reproduces AM4's real analogAxis::pos()/neg():
  //   inline navCmds pos() const {return inv^field_mode?negCmd:posCmd;}
  //   inline navCmds neg() const {return inv^field_mode?posCmd:negCmd;}
  // `inv` is folded in below HW (oneInput::InvDelta), so only the
  // field_mode half of the XOR is done here: HW::fieldMode() flips which
  // Cmd fires for a given tick direction. KNOWN GAP (see AnalogAxis's own
  // doc comment, analogAxis.h, for the full explanation): OneMenu's InDef
  // has no automatic "entering/leaving field edit" broadcast to input
  // devices — a sketch that wants the flip wires HW::setFieldMode(...)
  // manually from a FIELD()'s own enter/exit event.
  //
  //   using AxisHW = oneInput::InputDef<
  //     oneInput::AnalogAxis<512,6,10,1>,
  //     oneInput::avr::AvrAdcAxis<0>,
  //     hw::avr::AVRAdc<>
  //   >;
  //   InDef<oneMenu::AxisIn<AxisHW>, oneMenu::BtnIn<SwHW>, PCKbd> in;
  /// @brief OneInput->menu single-axis adapter; polls HW::delta(), maps to PosCmd/NegCmd (XORed with HW::fieldMode())
  template<typename HW, Cmd PosCmd = Cmd::Up, Cmd NegCmd = Cmd::Down>
  struct AxisIn {
    template<typename O>
    struct Part : O {
      static bool available() {
        return HW::delta() != 0 || O::available();
      }
      static CKE cmd() {
        int8_t d = HW::delta();
        bool flip = HW::fieldMode();
        if (d > 0) { HW::consume(1);  return {flip ? NegCmd : PosCmd}; }
        if (d < 0) { HW::consume(-1); return {flip ? PosCmd : NegCmd}; }
        return O::cmd();
      }
    };
  };

} // oneMenu
