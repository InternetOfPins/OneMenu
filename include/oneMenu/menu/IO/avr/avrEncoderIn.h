#pragma once
#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#include "oneMenu/menu/in.h"

namespace oneMenu {

  // ⚠ DEPRECATED: Use oneMenu::EncoderIn<chip::OnChange<>> instead (2026-Q3)
  //
  // AVR PCINT-based rotary encoder + button, for Port C (A0..A5 on Nano/Uno).
  // This implementation is platform-specific and requires manual ISR wiring.
  //
  // NEW: oneMenu::EncoderIn<ChangeSource> is platform-agnostic:
  //   #include <oneMenu/encoderIn.h>
  //   using MyEncoder = EncoderIn<chip::OnChange<A0, A1, A2>>;  // works on AVR/ESP32/STM32
  //   MyEncoder::begin();
  //   Action a = MyEncoder::poll();
  //
  // Legacy usage (DEPRECATED):
  // PinA, PinB : encoder channel bit positions in PORTC (0=A0, 1=A1, 2=A2 ...)
  // PinBtn     : button bit position in PORTC (active low, internal pull-up)
  // Steps      : quadrature transitions per detent (2 or 4 for most clicky encoders)
  //              set to 1 for encoders without detents or smooth encoders
  //
  // User must define the ISR in their translation unit:
  //   ISR(PCINT1_vect) { AvrEncoderIn<>::isr(); }
  //
  // Stacks cleanly with SerialIn for debug — encoder fires Up/Down/Enter directly,
  // serial chars still flow through PCKbd above it:
  //   InDef<AvrEncoderIn<>, PCKbd, SerialIn> in;
  // Or encoder-only:
  //   InDef<AvrEncoderIn<>> in;

  template<uint8_t PinA = 0, uint8_t PinB = 1, uint8_t PinBtn = 2, uint8_t Steps = 4>
  struct AvrEncoderIn {
    static constexpr uint8_t maskA   = uint8_t(1 << PinA);
    static constexpr uint8_t maskB   = uint8_t(1 << PinB);
    static constexpr uint8_t maskAB  = uint8_t(maskA | maskB);
    static constexpr uint8_t maskBtn = uint8_t(1 << PinBtn);

    // Quadrature decode: (prev_AB<<2)|curr_AB → ±1 or 0
    static constexpr int8_t _enc[16] = {
       0, -1,  1,  0,
       1,  0,  0, -1,
      -1,  0,  0,  1,
       0,  1, -1,  0
    };

    inline static volatile int8_t _delta    = 0;
    inline static volatile bool   _btn      = false;
    inline static          uint8_t _prevAB  = 0;
    inline static          bool    _btnPrev = true;  // unpressed = high

    static void begin() {
      DDRC  &= ~uint8_t(maskAB | maskBtn);   // inputs
      PORTC |=  uint8_t(maskAB | maskBtn);   // pull-ups on
      PCICR  |= (1<<PCIE1);                  // enable PCINT1 group
      PCMSK1 |= uint8_t(maskAB | maskBtn);   // watch our pins
      _prevAB = (PINC & maskAB) >> PinA;
      sei();
    }

    // Call from ISR(PCINT1_vect) in user's main.cpp
    static void isr() {
      uint8_t pins   = PINC;
      uint8_t currAB = (pins & maskAB) >> PinA;
      _delta += _enc[(_prevAB << 2) | currAB];
      _prevAB = currAB;
      bool btnNow = !(pins & maskBtn);        // active low
      if (btnNow && !_btnPrev) _btn = true;   // falling edge only
      _btnPrev = btnNow;
    }

    template<typename In>
    struct Part : In {
      static bool available() {
        return _btn || (_delta >= Steps) || (_delta <= -int8_t(Steps)) || In::available();
      }

      static CKE cmd() {
        if (_btn)                       { _btn = false;    return {Cmd::Enter}; }
        if (_delta >=  int8_t(Steps))   { _delta -= Steps; return {Cmd::Up};   }
        if (_delta <= -int8_t(Steps))   { _delta += Steps; return {Cmd::Down}; }
        return In::cmd();
      }
    };
  };

} // oneMenu
#endif
