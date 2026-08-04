#pragma once

#include "oneMenu/menu/in.h"

#ifdef ARDUINO
  #include <Arduino.h>   // millis(), for the debounce gate below
#endif

namespace oneMenu {

  /// @brief an axis-aligned on-screen key rectangle, screen-pixel units.
  struct KeyRect { Sz x,y,w,h; };

  /// @brief real per-key touch hit-testing InDef Part — resolves a raw touch point
  /// (from any Raw layer exposing static touched()/rawPoint(), e.g. Xpt2046BitBang)
  /// against a caller-supplied, compile-time table of on-screen key rectangles, and
  /// emits Cmd::Go carrying that key's 1-based body index — the exact event
  /// IndexGo::Part::in() (nav.h) already turns into "select item N, then fire
  /// Cmd::Enter", so no core Nav/CKE change is needed. This generalizes AM4's own
  /// touch strategy (uniform ROW hit-test: touch.y/resY -> row index) to real
  /// independent per-key rectangles — "a rectangle is a rectangle".
  ///
  /// `rects`/`nRects` should point at the SAME array the sketch used to place each
  /// key's Liquid<x,y> render position, so the touchable area and the drawn key can
  /// never drift apart. `goBase` is the Cmd::Go key value for rects[0] — i.e. the
  /// key item's 1-based position in the nav's assembled body (Base::go(key-1));
  /// caller picks this to match wherever the keys start in that body.
  ///
  /// Calibration (rawXMin/Max, rawYMin/Max) is a first-cut compile-time linear
  /// min/max map from this specific panel's raw ADC range (captured during bring-up
  /// on real hardware) to screenW x screenH pixel space — NOT a real 2-point/
  /// 4-corner affine calibration. Good enough for one physical panel wired once;
  /// revisit if the mapping proves off-center/skewed on hardware.
  ///
  /// One-shot touch-down latch (same idiom as Xpt2046Vendor/UrTouchVendor): fires
  /// once per touch-down transition, not repeatedly while held.
  ///
  /// `debounceMs` (default 150, caller-controlled — resistive touch panels vary):
  /// ignores a new touch-down transition within `debounceMs` of the last one that
  /// actually fired, absorbing IRQ contact bounce during a single physical press.
  /// Neither existing debounce component in this codebase fits a plain polled
  /// `bool touched()` without an adapter — oneInput::Debounce<Ms> is an interrupt-driven
  /// onRise()/onFall() mixin (OneInput/include/oneInput/debounce.h), and
  /// oneMenu::DebouncedButton<ChangeSource,Ms> needs a full ChangeSource object
  /// (OneMenu/include/oneMenu/debouncedButton.h) — so this is a small, self-contained
  /// time-gated debounce, same "ignore an edge within Ms of the last one" idea
  /// oneInput::Debounce itself uses, just inlined for a polled source.
  template<typename Raw, const KeyRect* rects, Sz nRects, Sz goBase,
           int rawXMin,int rawXMax,int rawYMin,int rawYMax,
           Sz screenW,Sz screenH, Sz debounceMs=150>
  struct TouchKeypad {
    template<typename In>
    struct Part : In {
      inline static bool _wasTouching = false;
      #ifdef ARDUINO
        inline static unsigned long _lastMs = 0;
      #endif

      static bool available() {return Raw::touched() || In::available();}

      static bool debounceOk() {
        #ifdef ARDUINO
          unsigned long now = millis();
          if (now-_lastMs < (unsigned long)debounceMs) return false;
          _lastMs = now;
          return true;
        #else
          return true;   // no millis() off-Arduino; TouchKeypad is never instantiated there
        #endif
      }

      // long intermediate, not Sz: Sz is int (16-bit on AVR) — (v-inMin)*(outMax-outMin)
      // reaches ~2.4M for this panel's real raw ADC range, silently overflowing a
      // 16-bit int and producing garbage screen coordinates that never land inside
      // any key rectangle (found the hard way: real hardware saw NO taps register at
      // all). Same reason Arduino's own map() uses long internally.
      static Sz mapRange(Sz v,Sz inMin,Sz inMax,Sz outMin,Sz outMax) {
        long num = (long)(v-inMin)*(long)(outMax-outMin);
        return (Sz)(outMin + num/(inMax-inMin));
      }

      static CKE cmd() {
        bool touching = Raw::touched();
        if (touching && !_wasTouching) {
          // Latch on the real touch-down transition regardless of debounceOk()'s
          // verdict — otherwise a single SUSTAINED hold would spuriously re-fire the
          // moment debounceMs elapses while still touching, since _wasTouching would
          // never have been set. debounceOk() only gates whether THIS transition (which
          // might itself be a bounce artifact) actually produces a Cmd::Go.
          _wasTouching = true;
          if (debounceOk()) {
            Pos raw = Raw::rawPoint();
            Sz sx = mapRange(raw.x, rawXMin,rawXMax, 0,screenW);
            Sz sy = mapRange(raw.y, rawYMin,rawYMax, 0,screenH);
            for (Sz i=0;i<nRects;i++) {
              const KeyRect& r = rects[i];
              if (sx>=r.x && sx<r.x+r.w && sy>=r.y && sy<r.y+r.h)
                return {Cmd::Go, Key(goBase+i), false, false};
            }
          }
        } else _wasTouching = touching;
        return In::cmd();
      }
    };
  };

} // namespace oneMenu
