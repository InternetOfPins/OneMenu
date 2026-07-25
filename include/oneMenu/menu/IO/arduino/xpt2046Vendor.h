#pragma once

#include "oneMenu/menu/in.h"

namespace oneMenu {

  // Thin, direct wrapper over a real, already-constructed XPT2046_Touchscreen
  // object (Paul Stoffregen's XPT2046_Touchscreen — the resistive touch
  // controller paired with most ILI9341/ILI9325 SPI TFT modules, a
  // different chip/library than UrTouchVendor's own URTouch/UTFT pairing).
  // Same compile-time-NTTP-reference binding style UrTouchVendor/KeypadVendor
  // use (the touch object is real, already-constructed by the caller — this
  // wrapper never owns or begin()s it).
  //
  // Deliberately scoped to "any tap = Cmd::Enter", discarding getPoint()'s
  // x/y/z entirely — identical reasoning to UrTouchVendor (OneMenu's CKE/Cmd
  // has no absolute-position concept; resolving a tap to a specific
  // on-screen item would need a genuine CKE/Nav core extension, out of scope
  // for a vendor wrapper).
  //
  // touched() is a real, idempotent query (unlike KeypadVendor's consuming
  // getKey()), so no local caching is needed the way KeypadVendor's _key is —
  // this instead hand-rolls the same one-shot-per-touch latch UrTouchVendor
  // uses: fires Enter once when touched() transitions true after having been
  // false, not on every poll while a finger is held down.
  //
  //   XPT2046_Touchscreen touch(T_CS_PIN);  // T_IRQ omitted — polled, not interrupt-driven
  //   InDef<Xpt2046Vendor<XPT2046_Touchscreen, touch>, SerialIn> in;
  //   // setup(): touch.begin();
  /// @brief direct thin wrapper over a real XPT2046_Touchscreen vendor object — any tap maps to Cmd::Enter, coordinates discarded (see file header comment for why)
  template<typename TouchT, TouchT& touch>
  struct Xpt2046Vendor {
    template<typename In>
    struct Part : In {
      inline static bool _wasTouching = false;

      static bool available() {
        return touch.touched() || In::available();
      }
      static CKE cmd() {
        bool touching = touch.touched();
        if (touching && !_wasTouching) {
          (void)touch.getPoint();  // x/y/z discarded — see file header comment
          _wasTouching = true;
          return {Cmd::Enter};
        }
        _wasTouching = touching;
        return In::cmd();
      }
    };
  };

} // namespace oneMenu
