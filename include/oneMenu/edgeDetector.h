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
#include "oneMenu/in.h"

namespace oneMenu {

  /// Edge detector for any ChangeSource with edge mode filtering
  template<typename ChangeSource>
  struct EdgeDetector : IIn {
    static_assert(onePin::is_change_source<ChangeSource>::value,
      "ChangeSource must satisfy is_change_source — see onePin/onChange.h");

    inline static volatile uint8_t _last = 0;
    inline static volatile bool _edge_seen = false;

    static void begin() final {
      ChangeSource::template begin<onePin::OnChange>();
      _last = ChangeSource::template read<onePin::OnChange>();
      _edge_seen = false;
    }

    static Action poll() final {
      if (!ChangeSource::template changed<onePin::OnChange>())
        return Action::idle();

      uint8_t now = ChangeSource::template read<onePin::OnChange>();
      uint8_t diff = now ^ _last;
      _last = now;

      if (diff & 0x01) {  // LSB changed
        _edge_seen = true;
        return Action::enter();  // Edge detected
      }

      return Action::idle();
    }

    static void end() final {}

    // Query: was an edge seen since last poll?
    static bool edge_pending() {
      return _edge_seen;
    }

    static void clear_edge() {
      _edge_seen = false;
    }
  };

} // oneMenu
