/**
 * @file encoderIn.h
 * @brief Platform-agnostic rotary encoder with debounced button.
 *
 * Works identically on AVR/ESP32/STM32 via ChangeSource abstraction.
 * Uses quadrature state machine; button is simple debounce.
 *
 * ChangeSource contract: OnChange<EdgeMode>, begin(), read(), changed()
 * See onePin/onChange.h for full contract.
 *
 * Usage:
 *   using MyEncoder = EncoderIn<chip::OnChange<A0, A1, A2>, 4>;  // A0=CHA, A1=CHB, A2=Button, Steps=4
 *   MyEncoder::begin();
 *   oneMenu::Action a = MyEncoder::poll();  // returns Action::Up, Down, Enter, or None
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>
#include <oneInput/inputEvent.h>
#include <oneMenu/menu/in.h>

namespace oneMenu {

  /// Generic rotary encoder + button for any ChangeSource (AVR/ESP32/STM32)
  /// Instance-based IIn wrapper for virtual dispatch in menus
  template<typename ChangeSource, uint8_t Steps = 4>
  struct EncoderIn : IIn {
    static_assert(onePin::is_change_source<ChangeSource>::value,
      "ChangeSource must satisfy is_change_source — see onePin/onChange.h");

    // Quadrature decode table: (prev_AB << 2) | curr_AB → step count
    static constexpr int8_t _quad[16] = {
       0, -1,  1,  0,
       1,  0,  0, -1,
      -1,  0,  0,  1,
       0,  1, -1,  0
    };

    volatile int8_t _delta = 0;      // accumulated steps
    volatile bool   _pending = false;
    CKE _last{};
    uint8_t _prevAB = 0;             // quadrature state
    bool    _btnPrev = true;         // button previous (active low)

    EncoderIn() {
      ChangeSource::begin();
      _prevAB = (ChangeSource::read() & 0x03);
    }

    bool available() override {
      if (ChangeSource::changed()) {
        uint8_t pins = ChangeSource::read();
        uint8_t currAB = (pins & 0x03);
        _delta += _quad[(_prevAB << 2) | currAB];
        _prevAB = currAB;

        bool btnNow = !(pins & 0x04);
        if (btnNow != _btnPrev) {
          _btnPrev = btnNow;
          if (btnNow) {
            _last.cmd = oneInput::Cmd::Enter;
            _pending = true;
            return true;
          }
        }

        int8_t rotations = _delta / Steps;
        if (rotations != 0) {
          _delta %= Steps;
          _last.cmd = (rotations > 0) ? oneInput::Cmd::Up : oneInput::Cmd::Down;
          _pending = true;
          return true;
        }
      }
      return _pending;
    }

    CKE cmd() override {
      _pending = false;
      return _last;
    }
  };

} // oneMenu
