#pragma once

#include "oneMenu/menu/sys/base.h"
#include <Arduino.h>

namespace oneMenu {

  /// @brief polled, interrupt-free XPT2046 resistive-touch reader — bit-bangs the
  /// chip's 8-bit-command/16-bit-response SPI protocol directly over plain GPIO
  /// (no hardware SPI, no vendor library), so it doesn't contend with a real
  /// hardware-SPI display bus (e.g. Adafruit_ST7789) sharing the same board.
  /// T_IRQ is only ever polled via digitalRead() here, never wired to a real
  /// interrupt.
  ///
  /// Protocol (standard XPT2046 control byte: S=1,A2..A0=channel,MODE=0,
  /// SER/DFR=0,PD1..0=1 — 0xD0 selects the X channel, 0x90 the Y channel):
  /// clock out the 8-bit command MSB-first, then clock in 16 response bits;
  /// the chip returns a leading dummy bit + 12-bit ADC result + 3 trailing
  /// zero bits, hence the final `>>3`. Verbatim port of the bring-up sketch
  /// at OneIO/.RnD/touchXpt2046Test/src/main.cpp, which confirmed clean,
  /// tracking raw X/Y on real hardware.
  ///
  /// Not an InDef Part itself (no cmd()/available()) — this is the raw
  /// protocol layer; TouchKeypad (touchKeypad.h) is the InDef Part that
  /// consumes touched()/rawPoint() and turns them into a CKE.
  ///
  ///   Xpt2046BitBang<3,4,5,6,7>::begin();  // setup()
  ///   if (Xpt2046BitBang<3,4,5,6,7>::touched()) auto p = Xpt2046BitBang<3,4,5,6,7>::rawPoint();
  template<uint8_t IRQ,uint8_t DO,uint8_t DIN,uint8_t CS,uint8_t CLK>
  struct Xpt2046BitBang {
    static void begin() {
      pinMode(IRQ,INPUT);
      pinMode(DO,INPUT);
      pinMode(DIN,OUTPUT);
      pinMode(CS,OUTPUT);
      pinMode(CLK,OUTPUT);
      digitalWrite(CS,HIGH);
      digitalWrite(CLK,LOW);
    }

    static bool touched() {return digitalRead(IRQ)==LOW;}

    static Pos rawPoint() {return {Sz(readTouch(0xD0)),Sz(readTouch(0x90))};}

  private:
    static uint16_t readTouch(uint8_t cmd) {
      digitalWrite(CS,LOW);
      for (int8_t i=7;i>=0;i--) {
        digitalWrite(CLK,LOW);
        digitalWrite(DIN,(cmd>>i)&0x01);
        delayMicroseconds(2);
        digitalWrite(CLK,HIGH);
        delayMicroseconds(2);
      }
      digitalWrite(CLK,LOW);
      delayMicroseconds(2);

      uint16_t value=0;
      for (int8_t i=0;i<16;i++) {
        digitalWrite(CLK,HIGH);
        delayMicroseconds(2);
        digitalWrite(CLK,LOW);
        value<<=1;
        if (digitalRead(DO)) value|=1;
        delayMicroseconds(2);
      }
      digitalWrite(CS,HIGH);
      return value>>3;
    }
  };

} // namespace oneMenu
