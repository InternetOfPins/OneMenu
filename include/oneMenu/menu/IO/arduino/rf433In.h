#pragma once

/**
 * @file rf433In.h
 * @brief ISR-driven 433 MHz radio receiver input component (ported from AM4/5).
 *
 * Protocol: a 12 ms+ pulse starts a packet; 24 bits follow as pairs of half-bit
 * durations. Pairing: if a pin is present below in the chain (e.g. InPin<N>)
 * and it reads high, the received ID is saved as the paired remote ID.
 *
 * Radio code is forwarded to O::parseKey() — the component below in the chain
 * interprets the 8-bit command value (e.g. FourKeyIn maps +/-/*//):
 *
 *   InDef<Rf433In<0>, FourKeyIn> in;          // interrupt 0, +-*/ remote
 *
 * @tparam RFRcvInt  Arduino interrupt number (digitalPinToInterrupt() result)
 * @tparam radioBits total protocol bits (default 24)
 * @tparam idBits    remote-ID bits (radioBits - idBits = command bits, default 8)
 */

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include "oneMenu/menu/in.h"

namespace oneMenu {

  /// @brief ISR-driven 433 MHz receiver; decodes AM4/5 protocol, forwards 8-bit command to parseKey chain
  template<int RFRcvInt, int radioBits = 24, int idBits = 16>
  struct Rf433In {
    template<typename O>
    struct Part : O {
      inline static volatile int      rfCmd  = 0;
      inline static          uint32_t pairId = 0;

      static void begin() {
        attachInterrupt(RFRcvInt, handler, CHANGE);
        O::begin();
      }

      static void handler() {
        static const constexpr int div        = 10;
        static const constexpr int radioStart = 12000 >> div;
        static uint32_t last  = 0;
        static uint32_t data  = 0;
        static bool     start = false;
        static int      at    = 0;
        uint32_t now = micros();
        uint16_t d   = uint16_t((now - last) >> div);
        last = now;
        if (start) {
          if (d > 1 || (at >> 1) >= radioBits) {
            if (d <= 1) rcvCmd(~data);
            at = 0; start = false; data = 0;
          } else {
            if (at & 1) { data <<= 1; data |= d; }
            at++;
          }
        } else start = (d >= radioStart);
      }

      static void rcvCmd(uint32_t raw) {
        const constexpr uint32_t mask   = ~(((uint32_t)(~0u)) << (radioBits - idBits));
        const constexpr uint32_t idMask = ((uint32_t)(~0u)) >> ((sizeof(raw) << 3) - radioBits);
        uint32_t id = (raw & idMask) >> (radioBits - idBits);
        if (id == pairId || !pairId) rfCmd = int(raw & mask);
        // For pairing support, subclass and override rcvCmd; call pairId = id when
        // a dedicated pairing pin is asserted.
      }

      static bool available() { return rfCmd != 0 || O::available(); }

      static CKE cmd() {
        if (rfCmd) {
          auto tmp = rfCmd; rfCmd = 0;
          return O::parseKey(Key(tmp));
        }
        return O::cmd();
      }
    };
  };

} // oneMenu
