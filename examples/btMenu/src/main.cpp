/**
 * @file main.cpp
 * @brief Smoke test: OneMenu field mirrored to a BLE GATT characteristic (ESP32).
 *        Power field is tagged BTRec<...,0> — editing it over the serial/ANSI menu
 *        pushes the new value out via Ble::char_write(0,...) whenever Watch sees a change.
 */

#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/textFmt.h>  // must precede ansiFmt.h (defines MenuChars)
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#include <oneMenu/menu/IO/idParser.h>
#include <oneMenu/menu/IO/arduino/serialOut.h>
#include <oneMenu/menu/IO/arduino/serialIn.h>

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>

#include <oneBus/uuid.h>
#include <chips/esp32/esp32Ble.h>
#include <oneChip/clock.h>

using namespace hapi;
using namespace oneMenu;
using namespace oneData;

struct SysTick {
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

using Ble = hw::esp32::esp32::Ble<
  BleDevice,
  oneBus::Characteristic<btIds::power_bt_id, oneBus::Uuid16<0x0002, NusBase>>
>;

// ── I/O ───────────────────────────────────────────────────────────────────────
InDef<SerialIn, IdParser, PCKbd> in;

OutDef<
  ScrollPrinter,
  ANSIFmt,
  DataParser<>,
  CtrlChars,
  ColorTrack<int>,
  Cursor<>,
  Gate,
  ANSIOut,
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
  Serial.begin(115200);
  delay(500);
  Serial.println("[bt] begin...");
  Ble::begin();
  Serial.println("[bt] advertising started");
  out.lockMode(LockMode::None);
  out.setColors(WHITE, BLACK);
  out.clear();
  nav.printTo(out);
}

void loop() {
  static SysTick::Period<30> fps;
  if (fps) {
    fps.reset();
    nav.in(in);
    if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  }
}
