#pragma once

#include "oneMenu/menu/in.h"

namespace oneMenu {

  // Thin, direct wrapper over a real, already-constructed Keypad vendor
  // object (Mark Stanley/Alexander Brevig's Keypad library) — same "call
  // the mature vendor library directly instead of reimplementing it"
  // choice as oneIO::display::U8g2Vendor/AdaGfxVendor, and the exact same
  // chain slot/shape as UartIn (above): a raw char source sitting below
  // whatever key-code parser chain is composed above it (PCKbd/IdParser),
  // not a new parsing mechanism of its own.
  //
  // AM4's real `keypadIn` (menuIO/keypadIn.h) does no matrix-to-command
  // translation itself either — it's a bare passthrough over the same
  // Keypad::getKey(), delegating 100% of key-to-char mapping to the real
  // vendor library's own keys[ROWS][COLS] table (confirmed directly
  // against both the real AM4 source and the real Keypad library this
  // session) — arbitrary chars (digits, letters, symbols), not just nav
  // command codes. This wrapper reproduces that same "just produce chars"
  // role; any special-cased command chars (Enter/Esc/etc.) are handled the
  // same way as any other char-producing source: PCKbd/IdParser above it
  // in the chain, not here.
  //
  // Keypad::getKey() is a real, consuming call (returns NO_KEY='\0' if
  // nothing pressed) — calling it once for available() and again for
  // cmd() would silently drop every other keypress. Caches the pending
  // key locally between the two calls, same fix AM4's own keypadIn already
  // applies (`peek(){return key?key:(key=in.getKey())?key:-1;}`).
  //
  //   Keypad customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
  //   InDef<KeypadVendor<Keypad, customKeypad>, PCKbd> in;
  /// @brief direct thin wrapper over a real Keypad vendor object — no reimplementation, no AM4 dependency
  template<typename KeypadT, KeypadT& kp>
  struct KeypadVendor {
    template<typename In>
    struct Part : In {
      inline static char _key = 0;

      static bool available() {
        if (!_key) _key = kp.getKey();
        return _key != 0 || In::available();
      }
      static CKE cmd() {
        if (!_key) _key = kp.getKey();
        if (_key) {
          char k = _key;
          _key = 0;
          return In::parseKey(Key(k));
        }
        return In::cmd();
      }
    };
  };

} // namespace oneMenu
