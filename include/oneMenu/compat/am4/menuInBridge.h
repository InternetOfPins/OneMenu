/**
 * @file menuInBridge.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AM4 input-driver reuse — adapts OneMenu's own input pipeline onto
 *        AM4's real `Menu::menuIn` base interface, instead of a fresh
 *        native OneMenu backend per input device.
 *
 * Same "keep AM4's own driver wrapper classes as-is, translate OneMenu
 * calls onto them" rationale as menuOutBridge.h (own file header, same
 * date) — this is the input-side counterpart. Not for serial/keyboard
 * input specifically (OneMenu already has real, native support for that —
 * `oneMenu::ArduinoSerialIn`/`IdParser`/`PCKbd`, `menu/IO/arduino/
 * serialIn.h` — reuse those directly, no AM4 bridging needed or wanted
 * there); this is for AM4's own *physical* input drivers OneMenu doesn't
 * have a native equivalent for — `keyIn<N>`, `encoderIn`, `clickEncoderIn`,
 * `analogAxisIn`, `PCF8574KeyIn`, `rotaryEventIn`, `I2CkeypadIn`, ...
 *
 * Checked directly against AM4's real source (menuIo.h, menuBase.h), not
 * assumed: every one of those drivers derives from `Menu::menuIn:public
 * Stream` — the same uniform `available()`/`read()`/`peek()` byte-stream
 * interface regardless of the physical hardware, exactly mirroring
 * `Menu::menuOut`'s own shared-base situation on the output side. The byte
 * values aren't arbitrary either: AM4's own `Menu::config::navCodes`
 * (`navCodesDef`, a 10-entry `{navCmds cmd; uint8_t ch;}` table, default
 * `Menu::defaultOptions`) is the *shared* convention every driver's bytes
 * are meant to be read against — AM4's own `navRoot::doInput()` reverse
 * -looks-up each byte against this same table. So one generic bridge,
 * doing that same lookup, works uniformly for every AM4 input driver.
 *
 * Design choice, not full fidelity: only the six commands OneMenu's own
 * `oneMenu::Cmd` already has a direct equivalent for (`escCmd`/`enterCmd`/
 * `upCmd`/`downCmd`/`leftCmd`/`rightCmd`) get translated to a real `Cmd`.
 * `idxCmd` (AM4's index-entry marker) and anything unmatched (a bare
 * digit, a letter, ...) falls through to the *rest of the chain*'s own
 * `parseKey()` unchanged — meaning `oneMenu::IdParser` (already-shipped,
 * `menu/IO/idParser.h`: digit '1'-'9' → `Cmd::Go`) keeps doing that job
 * normally when composed above this bridge, no special-casing needed here.
 * `selCmd`/`scrlUpCmd`/`scrlDownCmd` have no direct `oneMenu::Cmd`
 * equivalent yet — deliberately left unmapped (falls through to
 * `parseKey()`, same as any other unrecognized byte) rather than guessed
 * at, matching this compat layer's own established "document the real gap,
 * don't silently misdispatch" precedent (see am4.h's own EventDispatch
 * scope comment for the same discipline elsewhere in this codebase).
 *
 * Same "device reader at the top of the chain" role
 * `oneMenu::ArduinoSerialIn`/`UartIn` already establish (`menu/IO/arduino/
 * serialIn.h`) — this bridge reads one byte from the wrapped AM4 driver and
 * feeds it either straight to a real `Cmd` or down into the rest of the
 * chain's own `parseKey()`, same shape, same InDef composition slot:
 *   `InDef<Am4InBridge<>, IdParser> in;`
 *
 * Real AM4 dependency, not this repo's own am4.h shim — see
 * menuOutBridge.h's own file header for what that distinction means and
 * why this stays a separate, opt-in header, out of am4.h's own include
 * chain.
 */
#pragma once

#include <nav.h>  // Menu::menuIn, Menu::config/navCodesDef/navCmds, Menu::defaultOptions
#include <oneMenu/menu/in.h>

namespace am4compat {

  /// @brief satisfies oneMenu's InDef<KK...> device-reader contract
  ///        (available()/cmd(), same slot oneMenu::ArduinoSerialIn/UartIn
  ///        already occupy), backed by ANY real, already-constructed AM4
  ///        `Menu::menuIn`-derived driver instance, reached purely through
  ///        `menuIn`'s own Stream-derived virtual interface plus AM4's own
  ///        `navCodesDef` byte convention — see file header comment.
  ///        Options defaults to AM4's own `Menu::defaultOptions`; pass a
  ///        sketch-specific `Menu::config` for a custom key mapping.
  template<const Menu::config& Options = Menu::defaultOptions>
  struct MenuInBridge {
    static inline Menu::menuIn* driver = nullptr;

    /// @brief binds this bridge to a real, already-begin()'d AM4 input
    ///        driver instance (any Menu::menuIn subclass). Call once,
    ///        before the first nav.in()/poll() use.
    static void begin(Menu::menuIn& d) { driver = &d; }

    static oneMenu::Cmd translate(uint8_t ch) {
      for(auto& nc : Options.navCodes) {
        if(nc.ch != ch) continue;
        switch(nc.cmd) {
          case Menu::escCmd:   return oneMenu::Cmd::Esc;
          case Menu::enterCmd: return oneMenu::Cmd::Enter;
          case Menu::upCmd:    return oneMenu::Cmd::Up;
          case Menu::downCmd:  return oneMenu::Cmd::Down;
          case Menu::leftCmd:  return oneMenu::Cmd::Left;
          case Menu::rightCmd: return oneMenu::Cmd::Right;
          // idxCmd/selCmd/scrlUpCmd/scrlDownCmd/noCmd: no direct oneMenu::Cmd
          // equivalent (yet) — fall through to parseKey(), same as any
          // other unrecognized byte, not guessed at. See file header.
          default: return oneMenu::Cmd::None;
        }
      }
      return oneMenu::Cmd::None;
    }

    template<typename In>
    struct Part : In {
      static bool available() { return In::available() || (driver && driver->available()); }
      static oneMenu::CKE cmd() {
        if(In::available()) return In::cmd();
        if(!driver || !driver->available()) return In::cmd();
        uint8_t ch = (uint8_t)driver->read();
        oneMenu::Cmd c = translate(ch);
        if(c != oneMenu::Cmd::None) return {c, 0, false, false};
        return In::parseKey(oneMenu::Key(ch));
      }
    };
  };

} // namespace am4compat
