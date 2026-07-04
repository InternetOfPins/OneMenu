/**
 * @file main.cpp
 * @brief Smoke test: OneMenu field mirrored to a BLE GATT characteristic (ESP32 or nRF52).
 *        Power field is tagged BTRec<...,0> — editing it over the serial/ANSI menu
 *        pushes the new value out via Ble::char_write(0,...) whenever Watch sees a change.
 */

#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#include <oneMenu/menu/IO/idParser.h>
#include <oneMenu/menu/IO/arduino/serialOut.h>
#include <oneMenu/menu/IO/arduino/serialIn.h>

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>

#include <oneBus/uuid.h>
#include <oneChip/clock.h>
#if defined(ARDUINO_ARCH_NRF52)
  #include <chips/nrf52/nrf52Ble.h>
#else
  #include <chips/esp32/esp32Ble.h>
#endif

using namespace hapi;
using namespace oneMenu;
using namespace oneData;

// named AppTick, not SysTick — CMSIS (core_cm4.h, pulled in on ARM/nRF52) #defines
// SysTick as a register-struct macro, which would clash with a same-named type here.
struct AppTick {
  template<uint32_t Ms>
  using Period = hw::Period<Ms>;
};

// ── BLE setup ─────────────────────────────────────────────────────────────────
struct BleDevice {
  static constexpr const char* name        = "MicroRTC";
  static constexpr const char* serviceUuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
};

enum btIds { power_bt_id };

// Nordic UART Service base — characteristic UUIDs are 6e4000XX-..., matching the service.
using NusBase = oneBus::Uuid128<0x6e,0x40,0x00,0x00, 0xb5,0xa3, 0xf3,0x93,
                                 0xe0,0xa9, 0xe5,0x0e,0x24,0xdc,0xca,0x9e>;

#if defined(ARDUINO_ARCH_NRF52)
using Ble = hw::nrf52::nrf52::Ble<
#else
using Ble = hw::esp32::esp32::Ble<
#endif
  BleDevice,
  oneBus::Characteristic<btIds::power_bt_id, oneBus::Uuid16<0x0002, NusBase>>
>;

// ── I/O ───────────────────────────────────────────────────────────────────────
InDef<SerialIn, IdParser, PCKbd> in;

OutDef<
  ScrollPrinter,
  TextFmt,
  DataParser<>,
  CtrlChars,
  Cursor<>,
  Gate,
  SerialOut,
  StaticPos<0,0>,
  StaticArea<40,10>
> out;

// ── Text strings ──────────────────────────────────────────────────────────────
namespace text {
  static constexpr CText power {"Power"};
  static constexpr CText percent {"%"};
  static constexpr CText quit {"Exit!"};
}

namespace action {
  bool quit(Sz) { return true; }
}

using Quit = ItemDef<Action<action::quit>, AsLabel<StaticText<&text::quit>>>;

// Power: numeric 0-100%, default 55, mirrored to BLE characteristic 0 on change.
using Power = NumFieldDef<
  Chain<AsLabel<StaticText<&text::power>>>,
  NumField<
    StaticNumRange<StaticRange<0,100,false>>,
    AsField<BTRec<Watch<Default<Int,55>>, btIds::power_bt_id>>
  >,
  AsUnit<StaticText<&text::percent>>
>;

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"BT Menu"},
  staticBody(
    Power{55},
    Quit{}
  )
);

INavDef<
  IndexGo,
  TreeNav,
  Root<decltype(mainMenu), mainMenu>
> nav;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // on as soon as setup() is reached at all

  Serial.begin(115200);
  Serial.println("[bt] 1 serial up");
  Serial.flush();

  delay(500);
  Serial.println("[bt] 2 after delay");
  Serial.flush();

#ifndef BT_DISABLE_BLE_FOR_TEST
  Ble::begin();
#endif
  Serial.println("[bt] 3 ble done");
  Serial.flush();

  out.lockMode(LockMode::None);
  out.clear();
  Serial.println("[bt] 4 out cleared");
  Serial.flush();

  nav.printTo(out);
  Serial.println("[bt] 5 first printTo done");
  Serial.flush();

  digitalWrite(LED_BUILTIN, LOW);  // off once setup() fully completes
}

void loop() {
  static AppTick::Period<500> heartbeat;
  if (heartbeat) { heartbeat.reset(); digitalToggle(LED_BUILTIN); }

  static AppTick::Period<30> fps;
  if (fps) {
    fps.reset();
    nav.in(in);
    if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  }
}
