/**
 * @file main.cpp
 * @brief OneMenu field mirrored to a BLE GATT characteristic (ESP32 or nRF52).
 *        Power field is tagged BTRec<...,0> — editing it pushes the new value out via
 *        Ble::char_write(0,...) whenever Watch sees a change.
 *
 * On nRF52 (the LUXO breadboard — see project_luxo memory) Power also drives a real
 * PWM LED channel directly, and the board's own SEL/UP/DN buttons drive nav — real
 * hardware in both directions, not just a serial/BLE demo. Pins from the real LUXO
 * firmware's Boards.h, ARDUINO_NRF52840_FEATHER block (bare Feather on a breadboard,
 * not the dedicated NRF52_LUXO PCB).
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

// ── LUXO breadboard hardware (nRF52 only) ──────────────────────────────────────
// Pins match HS_PWM_nChannels/lib/Boards.h's ARDUINO_NRF52840_FEATHER block.
#if defined(ARDUINO_ARCH_NRF52)
  // As actually wired on this board (not Boards.h's digital-pin assignment —
  // confirmed by Rui: this 2-channel board uses the A0/A2 analog-labeled headers).
  #define LUXO_PWM_CH1 A0  // Ch1 Out
  #define LUXO_PWM_CH2 A2  // Ch2 Out
  #define LUXO_SEL     11  // Select button
  #define LUXO_UP      12  // Value Up
  #define LUXO_DN      13  // Value Down
#endif

// Power's value lives here (DataRef, not owned) so loop() can drive the PWM pin
// from the exact same storage the menu field edits — no find<>/duplication needed.
int currentPower = 55;

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

// FullPrinter, not ScrollPrinter: ScrollPrinter needs real position feedback from
// the device to compute its scroll window (that's why the earlier ANSIFmt+ANSIOut
// version worked — real ANSI cursor addressing). Plain TextFmt+SerialOut has a
// no-op setPos(), so ScrollPrinter's body never rendered — same tradeoff already
// documented in the liquid example (FullPrinter required for Liquid/LiquidPos).
OutDef<
  FullPrinter,
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

// Power: numeric 0-100%, mirrored to BLE characteristic 0 on change, backed by
// currentPower directly (DataRef) so loop() reads the live value with no lookup.
using Power = NumFieldDef<
  Chain<AsLabel<StaticText<&text::power>>>,
  NumField<
    StaticNumRange<StaticRange<0,100,false>>,
    AsField<BTRec<Watch<DataRef<&currentPower>>, btIds::power_bt_id>>
  >,
  AsUnit<StaticText<&text::percent>>
>;

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"BT Menu"},
  staticBody(
    Power{},
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

  Serial.begin(115200);
  delay(500);

  Ble::begin();

#if defined(ARDUINO_ARCH_NRF52)
  Bluefruit.autoConnLed(false);  // no longer strictly needed on A0/A2, kept off regardless

  pinMode(LUXO_PWM_CH1, OUTPUT);
  pinMode(LUXO_PWM_CH2, OUTPUT);
  pinMode(LUXO_SEL, INPUT_PULLUP);
  pinMode(LUXO_UP,  INPUT_PULLUP);
  pinMode(LUXO_DN,  INPUT_PULLUP);
#endif

  out.lockMode(LockMode::None);
  out.clear();
  nav.printTo(out);
}

#if defined(ARDUINO_ARCH_NRF52)
// SEL/UP/DN are active-low (INPUT_PULLUP); this breadboard has been in a drawer
// for years, so debounce generously rather than trust clean edges.
void pollLuxoButtons() {
  static bool lastSel=false, lastUp=false, lastDn=false;
  static AppTick::Period<40> debounce;
  if (!debounce) return;
  debounce.reset();

  bool sel = !digitalRead(LUXO_SEL);
  bool up  = !digitalRead(LUXO_UP);
  bool dn  = !digitalRead(LUXO_DN);

  if (sel && !lastSel) {
    Serial.println("[luxo] SEL edge -> nav.enter() ..."); Serial.flush();
    nav.enter();
    Serial.println("[luxo] nav.enter() returned"); Serial.flush();
  }
  if (up  && !lastUp) {
    Serial.println("[luxo] UP edge -> nav.up() ..."); Serial.flush();
    nav.up();
    Serial.println("[luxo] nav.up() returned"); Serial.flush();
  }
  if (dn  && !lastDn) {
    Serial.println("[luxo] DN edge -> nav.down() ..."); Serial.flush();
    nav.down();
    Serial.println("[luxo] nav.down() returned"); Serial.flush();
  }

  lastSel=sel; lastUp=up; lastDn=dn;
}

#endif

void loop() {
  static AppTick::Period<500> heartbeat;
  if (heartbeat) { heartbeat.reset(); digitalToggle(LED_BUILTIN); }

#if defined(ARDUINO_ARCH_NRF52)
  pollLuxoButtons();
  // Confirmed working on real hardware (A0/A2) — Power now drives real brightness.
  analogWrite(LUXO_PWM_CH1, map(currentPower, 0, 100, 0, 255));
#endif

  static AppTick::Period<30> fps;
  if (fps) {
    fps.reset();
    nav.in(in);
    // doOutput() (not hand-rolled changed()+printTo()+sync()) — it also restores
    // lockMode to Update after a full redraw and force-clears on a level change,
    // both of which the hand-rolled version was silently skipping.
    nav.doOutput(out);
  }
}
