#pragma once

#include "oneMenu/menu/sys/base.h"
#include <Arduino.h>

namespace oneMenu {

  /// @brief Polled, interrupt-free XPT2046 resistive-touch reader; bit-bangs its SPI protocol over plain GPIO.
  /// Not an InDef Part itself (no cmd()/available()) — use via TouchKeypad (touchKeypad.h).
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
