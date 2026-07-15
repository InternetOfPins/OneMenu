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
#include <oneMenu/menu/in.h>
#include <oneItem/oneItem.h>

namespace oneMenu {

  /// Generic rotary encoder + button for any ChangeSource (AVR/ESP32/STM32)
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

    inline static volatile int8_t _delta = 0;      // accumulated steps
    inline static volatile bool   _btn = false;    // button pressed
    inline static          uint8_t _prevAB = 0;    // quadrature state
    inline static          bool    _btnPrev = true; // button previous (active low)

    void begin() override {
      ChangeSource::begin();
      _prevAB = (ChangeSource::read() & 0x03);  // pins 0,1 = A,B
    }

    Action poll() override {
      if (!ChangeSource::changed())
        return Action::idle();

      uint8_t pins = ChangeSource::read();
      uint8_t currAB = (pins & 0x03);         // A0, A1 → A, B channels
      _delta += _quad[(_prevAB << 2) | currAB];
      _prevAB = currAB;

      bool btnNow = !(pins & 0x04);           // A2 is button (active low)
      if (btnNow != _btnPrev) {
        _btnPrev = btnNow;
        if (btnNow) return Action::enter();  // debounced press
      }

      // Check if we've rotated a full detent
      int8_t rotations = _delta / Steps;
      if (rotations != 0) {
        _delta %= Steps;
        return (rotations > 0) ? Action::up() : Action::down();
      }

      return Action::idle();
    }

    void end() override {}
  };

} // oneMenu
