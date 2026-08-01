#pragma once
#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  // FontSwitch<Font, Tag=Fmt::Title, Prev=nullptr>
  // Wrapping component (same category as DeviceCursor/ColorTrack, out.h — a
  // real fmtStart/fmtStop override, not a marker like ColorTable<>): swaps the
  // underlying device to Font on Tag's fmtStart, restores Prev on Tag's
  // fmtStop. Calls Base::setFont(...), a raw device primitive forwarded by
  // VendorGfxOut/OledOut (oledOut.h's HasSetFont-gated forward) down to the
  // real driver (AdaGfxVendor::setFont); devices without real font support
  // safely no-op.
  //
  // Font-pointer-type agnostic (auto NTTP, C++17) — this header never has to
  // name a vendor font type (e.g. Adafruit_GFX's GFXfont); the example
  // supplies &SomeFont and includes that font's own header itself.
  //
  // Chain position matters: place ABOVE the device (VendorGfxOut<Oled>) in
  // the OutDef pack, not in a VendorGfxDisplay<...>'s Extra... slot — Extra
  // sits BELOW/inside VendorGfxOut there, so Base::setFont from that position
  // would never reach VendorGfxOut's own forward. Order relative to GfxFmtT
  // itself doesn't matter (GfxFmtT doesn't touch setFont), so FontSwitch can
  // sit directly below FullPrinter, above everything else.
  template<auto Font, Fmt Tag=Fmt::Title, decltype(Font) Prev=nullptr>
  struct FontSwitch {
    template<typename O>
    struct Part:O {
      using Base=O;
      using O::fmtStart;
      using O::fmtStop;
      template<Fmt tag>
      std::enable_if_t<tag&Tag>
      fmtStart(const Ctx& ctx) { Base::setFont(Font); Base::template fmtStart<tag>(ctx); }
      template<Fmt tag>
      std::enable_if_t<tag&Tag>
      fmtStop(const Ctx& ctx) { Base::template fmtStop<tag>(ctx); Base::setFont(Prev); }
    };
  };

} // namespace oneMenu
