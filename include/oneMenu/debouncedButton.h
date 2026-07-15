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

namespace oneMenu {

  /// Debounced button for any ChangeSource
  template<typename ChangeSource, uint16_t DebounceMs = 20>
  struct DebouncedButton : IIn {
    static_assert(onePin::is_change_source<ChangeSource>::value,
      "ChangeSource must satisfy is_change_source — see onePin/onChange.h");

    volatile uint16_t _debounce_count = 0;
    volatile bool _current = false;
    volatile bool _pending = false;
    CKE _last{};

    DebouncedButton() {
      ChangeSource::begin();
      _current = !(ChangeSource::read() & 0x01);  // active low
    }

    bool available() override {
      if (ChangeSource::changed()) {
        bool now = !(ChangeSource::read() & 0x01);

        if (now != _current) {
          _debounce_count++;
          if (_debounce_count * 10 >= DebounceMs) {
            _current = now;
            _debounce_count = 0;

            if (_current) {
              _last.cmd = Cmd::Enter;
              _pending = true;
              return true;
            }
          }
        } else {
          _debounce_count = 0;
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
