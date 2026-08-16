#pragma once

/**
 * @file pcKbdIn.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief PC keyboard extended codes parser
 * @version 5
 * @date 2024-11-13
 *
 * Translates ANSI/VT100 escape sequences and raw chars to CKE values.
 * No nav reference is needed — callers receive the CKE and dispatch locally.
 * The rare ESC+char double-command is handled via a one-slot _pending queue;
 * available() exposes it so outer drivers drain before reading new input.
 */

#include "oneMenu/menu/in.h"
#include <oneChip/clock.h>

namespace oneMenu {

  struct PCKbd {
    template<typename O>
    struct Part : O {

      inline static CKE _pending{};  // Cmd::None = nothing queued

      // esc/escTimer stay function-local statics (not inline static members): a member
      // with hw::Timeout's non-trivial constructor (calls millis()) needs AVR's global-
      // constructor-table machinery to run before main() — pulling that in cost 236 bytes
      // of flash on a target that only had 144 spare (neurMenu/uno: 32112 -> 32348 of
      // 32256, overflow). A function-local static's guarded lazy-init is far cheaper and
      // wasn't otherwise needed anywhere in this program. Exposed via these two small
      // accessors so available() (below) can read the same shared state parseKey uses.
      static Key& escState() { static Key esc=0; return esc; }
      static hw::Timeout<30>& escTimerState() { static hw::Timeout<30> t; return t; }

      // escState()==1 && escTimerState() (not escState()==1 alone): the latter would
      // report "ready" continuously while the 30ms window is still open, and inBurst
      // (in.h)'s draining loop would busy-spin forever on it — in() returns false for
      // Cmd::None (the timer hasn't fired yet), so its call-count never advances, and
      // available() going false is the loop's only other exit condition.
      static bool available() { return _pending.cmd != Cmd::None || (escState()==1 && escTimerState()) || O::available(); }

      // Poll with no new key — used for ESC timeout and draining _pending.
      static CKE cmd() {
        if (_pending.cmd != Cmd::None) {
          CKE r = _pending;
          _pending.cmd = Cmd::None;
          return r;
        }
        return parseKey(Key(0));
      }

      static CKE parseKey(Key k) {
        static Key term = 0;
        Key& esc = escState();
        hw::Timeout<30>& escTimer = escTimerState();

        // ESC timeout: ESC was received but no bracket followed within 30ms.
        if (esc == 1 && escTimer && !k) {
          esc = 0;
          return {Cmd::Esc, 0, false, true};
        }

        if (k == term || !k) return {};  // suppressed terminator or no input

        if (esc == 1) {
          if (k == '[') {
            esc = 2;
            return {};  // absorb; wait for sequence body
          } else {
            // ESC followed immediately by a non-bracket key:
            // deliver Esc first, queue the Key char for next poll.
            esc = 0;
            escTimer.reset();
            _pending = {Cmd::Key, k, false, false};
            return {Cmd::Esc, 0, false, false};
          }

        } else if (esc == 2) {
          esc = 0;
          switch (k) {
            // NOT a straight ANSI-CSI-letter-to-Cmd-name mapping: Cmd::Up/Down
            // are INDEX direction (Up increments, Down decrements — see the
            // comment on oneMenu::Cmd, sys/enums.h), not visual arrow direction.
            // A real down-arrow (ESC[B, moves the cursor to a LATER/higher-
            // index item) therefore has to fire Cmd::Up (increment) to move
            // down the list — this mapping is deliberate, already correct.
            // Confirmed by tracing a real down-arrow press through doNav()
            // natively: swapping this to the visually-obvious A=Up/B=Down
            // made a down-arrow press on sel=0 DECREMENT and wrap straight to
            // the last item instead of advancing to item 1 — a self-inflicted
            // regression from assuming Cmd names meant visual directions.
            case 'A': return {Cmd::Down};
            case 'B': return {Cmd::Up};
            case 'C': return {Cmd::Left};
            case 'D': return {Cmd::Right};
            default: {
              // Unknown escape sequence (e.g. Home=H, End=F, Del=3~): let inner chain try,
              // else emit as extended key so text fields can distinguish from typed chars.
              // Digit-prefixed codes (Del/Ins/PgUp/PgDown) end in a `~` terminator;
              // arm `term` to swallow it. Letter-terminated codes (Home=H, End=F) have
              // no trailing byte, so term stays unset — arming it here would risk
              // swallowing an unrelated later `~` keystroke.
              if (k >= '0' && k <= '9') term = 0x7E;
              CKE r = O::parseKey(k);
              return r.cmd != Cmd::None ? r : CKE{Cmd::Key, k, true, true};
            }
          }

        } else {
          esc = 0;
          if (k == 0x1B) {  // ESC start
            esc = 1;
            escTimer.reset();
            return {};
          }
          switch (k) {
            case 0xA:
              term = 0xD;
              return {Cmd::Enter};
            case 0xD:
              term = 0xA;
              return {Cmd::Enter};
            default: {
              // Raw key: let inner chain try first, else emit as Key event.
              CKE r = O::parseKey(k);
              return r.cmd != Cmd::None ? r : CKE{Cmd::Key, k, false, true};
            }
          }
        }
      }
    };
  };

};//namespace oneMenu
