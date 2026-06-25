/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief OneMenu fields example — text, numeric, toggle, select, choose, date, prompt, idle
 */

// ── Includes ──────────────────────────────────────────────────────────────────
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/pcKbdIn.h>

#ifdef __AVR__
  #include <onePin/onePin.h>
  #include <chips/avr/avrDevice.h>
  #include <oneMenu/menu/IO/arduino/serialOut.h>
  #include <oneMenu/menu/IO/arduino/serialIn.h>
#elif defined(__arm__)
  #include <onePin/onePin.h>
  #include <chips/stm32/stm32Device.h>
  using namespace hw::stm32;
#else
  #include <oneMenu/menu/IO/streamOut.h>
  #include <oneMenu/menu/IO/linuxKeyIn.h>
  #include <oneChip/clock.h>
#endif

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>

using namespace std;
using namespace hapi;
using namespace oneMenu;
using namespace oneData;
using oneMenu::Action;

// ── Board / SysTick ───────────────────────────────────────────────────────────
#ifdef __AVR__
  using namespace onePin;
  using namespace oneBit;
  using SysTick = chip::SysTick0<>;
  using Led1    = AVR::OutPin<Pins<5>, chip::PortB>;
  using Board   = AVR::Board<Boot<SysTick>, Led1>;
  #ifdef IOP
  IOP_TIMER0_ISR(Board)
  #endif
#elif defined(__arm__)
  using namespace onePin;
  using namespace oneBit;
  using SysTick = chip::SysTick<>;
  using Led1    = STM32::InvOutPin<Pins<13>, chip::PortC>;
  using Board   = STM32::Board<Boot<>, Led1>;
  #ifdef IOP
  IOP_SYSTICK_ISR(Board)
  #endif
#else
  struct SysTick {
    template<uint32_t Ms>
    using Period = hw::Period<Ms>;
  };
#endif
// ─────────────────────────────────────────────────────────────────────────────

bool running = true;
char promptMsg[48]{};

// ── I/O ───────────────────────────────────────────────────────────────────────
InDef<
  #ifdef ARDUINO
    SerialIn,
  #else
    LinuxKeyIn,
  #endif
  PCKbd
> in;

// main output — IOutDef auto-injects Gate/ColorTrack/Cursor
IOutDef<
  FullPrinter,
  ANSIFmt,
  DataParser<>,
  CtrlChars,
  ANSIOut,
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<20,10>,
  StaticArea<30,10>
> out;

// prompt overlay — OutDef with explicit tracking components, inset over out
OutDef<
  FullPrinter,
  ANSIFmt,
  DataParser<>,
  CtrlChars,
  ColorTrack<int>,
  Cursor<>,
  Gate,
  ANSIOut,
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<decltype(out)::orgX()+4, decltype(out)::orgY()+2>,
  StaticArea<decltype(out)::width()-8, 4>
> promptOut;

// ── Text strings ──────────────────────────────────────────────────────────────
namespace text {
  static constexpr CText back        {"<Back"};
  static constexpr CText quit        {"Exit!"};
  static constexpr CText op1         {"Option 1"};
  static constexpr CText op2         {"Option 2"};
  static constexpr CText op3         {"Option 3"};
  static constexpr CText settings    {"Settings..."};
  static constexpr CText power       {"Power"};
  static constexpr CText percent     {"%"};
  static constexpr CText toggle_demo {"Toggle"};
  static constexpr CText rise        {"Rise"};
  static constexpr CText fall        {"Fall"};
  static constexpr CText both        {"Both"};
  static constexpr CText select_demo {"Select"};
  static constexpr CText choose_demo {"Choose"};
  static constexpr CText s10         {"10"};
  static constexpr CText s40         {"40"};
  static constexpr CText s60         {"60"};
  static constexpr CText s80         {"80"};
  static constexpr CText s100        {"100"};
  static constexpr CText dateSep     {"."};
  static constexpr CText ok          {"OK"};
}

// ── Item IDs (for find<>) ─────────────────────────────────────────────────────
enum ids { op3_id, power_id };

// ── Actions ───────────────────────────────────────────────────────────────────
namespace action {
  bool op1(Sz);  // defined after showPrompt
  bool op2(Sz);  // defined after mainMenu
  bool ok(Sz);   // defined after showPrompt
  bool op3(Sz) { return true; }
  bool quit(Sz) {
    running = false;
    return true;
  }
  bool subIdx(Sz i) { return false; }
}

// ── Reusable item types ───────────────────────────────────────────────────────
using Back = ItemDef<StaticText<&text::back>>;
using Quit = ItemDef<Action<action::quit>, AsLabel<StaticText<&text::quit>>>;

// ── Field definitions ─────────────────────────────────────────────────────────

// Power: numeric 0–100%, default 55, watch-able by Id
using Power = NumFieldDef<
  Chain<
    Id<ids::power_id>,
    AsLabel<StaticText<&text::power>>
  >,
  NumField<
    StaticNumRange<StaticRange<0,100,false>>,
    AsField<Watch<Default<Int,55>>>
  >,
  AsUnit<StaticText<&text::percent>>
>;

// Toggle: cycles Rise/Fall/Both inline on each Enter (WrapNav baked in)
using ToggleDemo = ToggleFieldDef<
  ItemDef<StaticText<&text::toggle_demo>, AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::rise>>>,
    ItemDef<AsField<StaticText<&text::fall>>>,
    ItemDef<AsField<StaticText<&text::both>>>
  >,
  BodyAction<action::subIdx>
>;

// Select: pick one from a list; selected value shown inline
using SelectDemo = SelectFieldDef<
  ItemDef<
    AsLabel<StaticText<&text::select_demo>>,
    AsEditMode<>,
    BodyAction<action::subIdx>
  >,
  StaticBody<
    ItemDef<AsField<StaticText<&text::s10>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s40>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s60>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s80>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s100>>, AsUnit<StaticText<&text::percent>>>
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

// Choose: navigate into sub-body to pick; chosen value shown on item row
using ChooseDemo = ChooseFieldDef<
  ItemDef<
    StaticText<&text::choose_demo>,
    AsEditMode<>,
    BodyAction<action::subIdx>
  >,
  StaticBody<
    ItemDef<AsField<StaticText<&text::s10>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s40>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s60>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s80>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s100>>, AsUnit<StaticText<&text::percent>>>
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

// Date: composite pad — three EditField/ParentDraw columns (year.month.day)
auto dateField(const char* lbl) {
  return padDef(
    ItemDef<AsLabel<Text>, AsEditMode<>>{lbl},
    staticBody(
      ItemDef<
        EditField,
        ParentDraw,
        NumField<StaticNumRange<StaticRange<1900,2150,true>>, AsField<Watch<Default<Int,2026>>>>
      >{2026},
      ItemDef<
        StaticText<&text::dateSep>,
        EditField, ParentDraw, AsEditMode<>,
        NumField<StaticNumRange<StaticRange<1,12,true>>, AsField<Watch<Int>>>
      >{1},
      ItemDef<
        StaticText<&text::dateSep>,
        EditField, ParentDraw, AsEditMode<>,
        NumField<StaticNumRange<StaticRange<1,31,true>>, AsField<Watch<Int>>>
      >{1}
    )
  );
}

// ── Menu tree ─────────────────────────────────────────────────────────────────
auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Main menu"},
  staticBody(
    ItemDef<Action<action::op1>, StaticText<&text::op1>>{},
    ItemDef<Action<action::op2>, StaticText<&text::op2>>{},
    // op3 starts disabled; op2 toggles it at runtime via find<>
    ItemDef<Id<ids::op3_id>, Action<action::op3>, Watch<EnDis<false>>, StaticText<&text::op3>>{},
    menuDef<WrapNav>(
      ItemDef<StaticText<&text::settings>>{},
      staticBody(
        ItemDef<
          AsLabel<Text>,
          AsEditMode<>,
          EditField,
          ParentDraw,
          AsField<TextField<15>>
        >{"Name"},
        Power{55},
        ToggleDemo{},
        SelectDemo{},
        ChooseDemo{},
        dateField("date"),
        Back{}
      )
    ),
    Quit{}
  )
);

INavDef<
  TreeNav,
  Root<decltype(mainMenu), mainMenu>
> nav;

bool action::op2(Sz) {
  auto& op3 = find<SameAs<Id<ids::op3_id>>>(mainMenu);
  op3.enable(!op3.enabled());
  return true;
}

// ── Prompt ────────────────────────────────────────────────────────────────────
auto promptMenu = menuDef<>(
  ItemDef<Text>{promptMsg},
  staticBody(
    ItemDef<Action<action::ok>, StaticText<&text::ok>>{}
  )
);

INavDef<
  TreeNav,
  Root<decltype(promptMenu), promptMenu>
> promptNav;

// ── Handler machinery ─────────────────────────────────────────────────────────
using RunFn = bool(*)();
RunFn activeRun;

static SysTick::Period<30000> idleTimer;

void showIdle();  // forward

bool mainRun() {
  bool input = nav.in(in);
  if (input) idleTimer.reset();
  if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  if (idleTimer) { idleTimer.reset(); showIdle(); }
  return running;
}

bool idleRun() {
  if (nav.in(in)) {
    idleTimer.reset();
    activeRun = mainRun;
    out.lockMode(LockMode::None);
    nav.printTo(out);
  }
  return running;
}

bool promptRun() {
  promptNav.in(in);
  if (promptNav.changed(promptOut)) {
    promptNav.printTo(promptOut);
    promptNav.sync(promptOut);
  }
  return running;
}

void showIdle() {
  activeRun = idleRun;
  out.clear();
}

void showPrompt(const char* msg) {
  strncpy(promptMsg, msg, sizeof(promptMsg)-1);
  activeRun = promptRun;
  promptOut.lockMode(LockMode::None);
  promptOut.setColors(BLACK, WHITE);
  promptOut.clear();
  promptNav.printTo(promptOut);
}

bool action::op1(Sz) { showPrompt("Option 1 activated!"); return true; }
bool action::ok(Sz)  {
  promptOut.clear();
  activeRun = mainRun;
  out.lockMode(LockMode::None);
  nav.printTo(out);
  return true;
}

// ── Loop ──────────────────────────────────────────────────────────────────────
bool run() {
  static SysTick::Period<30> fps;
  if (fps) {
    fps.reset();
    activeRun();
  }
  if (!fps) hw::delay_ms(fps.when() - hw::millis());
  return running;
}

void setup() {
  out.lockMode(LockMode::None);
  out.setColors(WHITE, BLACK);
  out.clear();
  activeRun = mainRun;
  nav.printTo(out);
}

int main() {
  setup();
  while (run());
  return 0;
}
