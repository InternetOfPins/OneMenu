/**
 * @file main.cpp
 * @brief Cross-platform encoder example using OnChange abstraction.
 *
 * Same code compiles and works identically on AVR, ESP32, STM32.
 * The ChangeSource (interrupt mechanism) is resolved at compile-time via chip:: alias.
 *
 * Platform-specific pins:
 *   AVR:   A0=encoder A, A1=encoder B, A2=button (PORTC)
 *   ESP32: GPIO34=encoder A, GPIO35=encoder B, GPIO36=button
 *   STM32: PA0=encoder A, PA1=encoder B, PA2=button
 */

// Platform-specific includes
#ifdef __AVR__
  #include <chips/avr/avrDevice.h>
#elif defined(ESP32)
  #include <chips/esp32/esp32Device.h>
#elif defined(STM32)
  #include <chips/stm32/stm32Device.h>
#else
  // Native/test build
  #include <stdint.h>
#endif

#include <oneMenu/encoderIn.h>

// Platform-specific pin mapping via chip:: alias
#ifdef __AVR__
  using EncoderSource = chip::OnChange<0, 1, 2>;  // A0, A1, A2
#elif defined(ESP32)
  using EncoderSource = chip::OnChange<GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36>;
#elif defined(STM32)
  using EncoderSource = chip::OnChange<0, 1, 2>;  // PA0, PA1, PA2 (PIN_ID encoding)
#else
  // Native/test build — null source
  using EncoderSource = onePin::ChangeSourceTerm;
#endif

// Generic encoder instance — IDENTICAL on all platforms
using EncoderType = oneMenu::EncoderIn<EncoderSource, 4>;
static EncoderType encoder;

// Simple action handler
void handle_action(oneMenu::CKE cke) {
  switch (cke.cmd) {
    case oneMenu::Cmd::Up:
      // Encoder rotated forward
      break;
    case oneMenu::Cmd::Down:
      // Encoder rotated backward
      break;
    case oneMenu::Cmd::Enter:
      // Button pressed
      break;
    default:
      break;
  }
}

#ifdef ARDUINO
  void setup() {
    // Constructor already calls begin()
  }

  void loop() {
    if (encoder.available()) {
      handle_action(encoder.cmd());
    }
  }
#else
  // Bare-metal or FreeRTOS
  int main() {
    while (true) {
      if (encoder.available()) {
        handle_action(encoder.cmd());
      }
    }
  }
#endif
