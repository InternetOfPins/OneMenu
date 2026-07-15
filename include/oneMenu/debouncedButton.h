/**
 * @file debouncedButton.h
 * @brief Platform-agnostic debounced button input.
 *
 * Works on any ChangeSource (OnChange/OnRise/OnFall).
 * Simple debounce: requires stable state for debounce_ms before reporting.
 *
 * Usage:
 *   using MyButton = DebouncedButton<chip::OnChange<A2>, 20>;  // 20ms debounce
 *   MyButton::begin();
 *   Action a = MyButton::poll();  // returns Action::enter() when pressed
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>
#include <oneMenu/menu/in.h>
#include <oneItem/oneItem.h>

namespace oneMenu {

  /// Debounced button for any ChangeSource
  template<typename ChangeSource, uint16_t DebounceMs = 20>
  struct DebouncedButton : IIn {
    static_assert(onePin::is_change_source<ChangeSource>::value,
      "ChangeSource must satisfy is_change_source — see onePin/onChange.h");

    enum State : uint8_t { Released = 0, Pressed = 1, Debouncing = 2 };

    inline static volatile State _state = Released;
    inline static volatile uint16_t _debounce_count = 0;
    inline static volatile bool _current = false;  // false=released, true=pressed

    void begin() override {
      ChangeSource::begin();
      _state = Released;
      _debounce_count = 0;
      _current = !(ChangeSource::read() & 0x01);  // active low
    }

    Action poll() override {
      if (!ChangeSource::changed())
        return Action::idle();

      bool now = !(ChangeSource::read() & 0x01);

      if (now == _current) {
        // No change; reset debounce
        _debounce_count = 0;
        return Action::idle();
      }

      // State changed; debounce
      _debounce_count++;
      if (_debounce_count * 10 >= DebounceMs) {  // assuming 10ms poll interval
        _current = now;
        _debounce_count = 0;

        if (_current) {
          return Action::enter();  // Button pressed (debounced)
        }
      }

      return Action::idle();
    }

    void end() override {}
  };

} // oneMenu
