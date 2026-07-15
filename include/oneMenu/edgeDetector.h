/**
 * @file edgeDetector.h
 * @brief Platform-agnostic edge detector (rise/fall).
 *
 * Detects and reports GPIO state transitions.
 * Works with any ChangeSource via OnChange/OnRise/OnFall modes.
 *
 * Usage:
 *   using MyRise = EdgeDetector<chip::OnRise<GPIO_PIN>>;    // rising edge only
 *   using MyFall = EdgeDetector<chip::OnFall<GPIO_PIN>>;    // falling edge only
 *   MyRise::begin();
 *   if (MyRise::poll() == Action::enter()) { handle_rise(); }
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>
#include <oneMenu/menu/in.h>

namespace oneMenu {

  /// Edge detector for any ChangeSource with edge mode filtering
  template<typename ChangeSource>
  struct EdgeDetector : IIn {
    static_assert(onePin::is_change_source<ChangeSource>::value,
      "ChangeSource must satisfy is_change_source — see onePin/onChange.h");

    volatile uint8_t _last = 0;
    volatile bool _edge_seen = false;
    CKE _last_cke{};

    EdgeDetector() {
      ChangeSource::begin();
      _last = ChangeSource::read();
    }

    bool available() override {
      if (ChangeSource::changed()) {
        uint8_t now = ChangeSource::read();
        uint8_t diff = now ^ _last;
        _last = now;

        if (diff & 0x01) {
          _edge_seen = true;
          _last_cke.cmd = Cmd::Enter;
          return true;
        }
      }
      return _edge_seen;
    }

    CKE cmd() override {
      _edge_seen = false;
      return _last_cke;
    }

    bool edge_pending() {
      return _edge_seen;
    }

    void clear_edge() {
      _edge_seen = false;
    }
  };

} // oneMenu
