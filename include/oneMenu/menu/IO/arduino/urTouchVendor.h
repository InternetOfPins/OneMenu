#pragma once

#include "oneMenu/menu/in.h"

namespace oneMenu {

  // Thin, direct wrapper over a real, already-constructed URTouch object
  // (rinkydinkelectronics.com URTouch, the touch-panel companion to UTFT) —
  // same compile-time-NTTP-reference binding style KeypadVendor uses
  // (URTouch is a real global object, not begin()-bound like the OUTPUT-
  // side vendors). Confirmed via AM4's own real driver (menuIO/
  // urtouchIn.h): polled via dataAvailable() -> read() -> getX()/getY().
  //
  // Deliberately scoped to "any tap = Cmd::Enter", discarding getX()/
  // getY() entirely: OneMenu's CKE/Cmd has no absolute-position concept at
  // all (Cmd is None/Enter/Esc/Up/Down/Left/Right/Key/Go — nothing lets
  // Nav resolve "the item rendered at pixel (x,y)"). Resolving a tap to a
  // specific on-screen item would need a genuine CKE/Nav core extension —
  // out of scope for a vendor wrapper, a documented limitation, not an
  // oversight.
  //
  // NOT built on oneInput::PressCapture: PressCapture's onFall() is an
  // edge callback meant to be invoked from a real interrupt/pin-change
  // source; URTouch exposes no interrupt hook at this level, only a polled
  // dataAvailable(), so there's no edge to attach PressCapture to. Instead
  // this hand-rolls the equivalent one-shot-per-touch latch directly: fires
  // Enter once when dataAvailable() transitions available after having been
  // unavailable, not on every poll while a finger is held down (if
  // dataAvailable() instead stays continuously true while held, this still
  // degrades safely to "one Enter, then silence until release" — never a
  // runaway repeat-fire — the exact real transition behavior is a real-
  // hardware question, not assumed here).
  //
  //   URTouch uTouch(6,5,4,3,2);
  //   InDef<UrTouchVendor<URTouch, uTouch>, SerialIn> in;
  //   // setup(): uTouch.InitTouch(); uTouch.setPrecision(PREC_MEDIUM);
  /// @brief direct thin wrapper over a real URTouch vendor object — any tap maps to Cmd::Enter, coordinates discarded (see file header comment for why)
  template<typename TouchT, TouchT& touch>
  struct UrTouchVendor {
    template<typename In>
    struct Part : In {
      inline static bool _wasTouching = false;

      static bool available() {
        return touch.dataAvailable() || In::available();
      }
      static CKE cmd() {
        bool touching = touch.dataAvailable();
        if (touching && !_wasTouching) {
          touch.read();
          (void)touch.getX(); (void)touch.getY();  // discarded — see file header comment
          _wasTouching = true;
          return {Cmd::Enter};
        }
        _wasTouching = touching;
        return In::cmd();
      }
    };
  };

} // namespace oneMenu
