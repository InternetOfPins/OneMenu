#pragma once

#include "oneMenu/menu/in.h"

#ifdef ARDUINO
  #include <Arduino.h>   // millis(), for the debounce gate below
#endif

namespace oneMenu {

  /// @brief an axis-aligned on-screen key rectangle, screen-pixel units.
  struct KeyRect { Sz x,y,w,h; };

  /// @brief Per-key touch hit-testing InDef Part: resolves a raw touch point from Raw (e.g. Xpt2046BitBang)
  /// against a compile-time table of key rectangles, emitting Cmd::Go for the hit key. One-shot latch:
  /// fires once per touch-down transition, debounced by debounceMs.
  /// @tparam rects/nRects key-rectangle table, screen-pixel units; must match the rendered key positions
  /// @tparam goBase Cmd::Go key value for rects[0] (1-based body index)
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
