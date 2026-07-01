/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief OneMenu Liquid example — fixed-point items that don't disturb sequential layout
 *
 * Demonstrates:
 *  - Liquid<x,y>:  compile-time fixed position. Item saves the cursor, jumps to (x,y),
 *                  prints, then restores — items around it lay out exactly as if it
 *                  wasn't there at all.
 *  - LiquidPos:    same behavior, runtime-settable position via .liquidPos({x,y}).
 *                  Combined here with find<Id<>> to relocate it from an action callback.
 *  - Non-cursor fallback: on an output with no Cursor<> (e.g. plain serial), both
 *                  components silently degrade to a normal in-sequence print.
 *  - ScrollBodyPrinter is INCOMPATIBLE with Liquid/LiquidPos and rejected at compile
 *                  time (scroll measurement assumes items advance the cursor
 *                  sequentially — a fixed-point jump-and-restore breaks that math).
 *                  This example therefore uses FullPrinter, not ScrollPrinter.
 *                  Swapping FullPrinter for ScrollPrinter below fails to build with:
 *                    "Liquid: incompatible with ScrollBodyPrinter ..."
 */

// includes --
  #include <oneMenu/oneMenu.h>
  #include <oneMenu/menu/IO/pcKbdIn.h>
  #include <oneMenu/menu/IO/ansiOut.h>    // must precede ansiFmt.h (defines color constants)
  #include <oneMenu/menu/fmt/textFmt.h>   // must precede ansiFmt.h (defines MenuChars)
  #include <oneMenu/menu/fmt/ansiFmt.h>
  #include <oneMenu/menu/IO/idParser.h>

  #ifdef __AVR__
    #include <onePin/onePin.h>
    #include <chips/avr/avrDevice.h>
    #include <oneMenu/menu/IO/arduino/serialOut.h>
    #include <oneMenu/menu/IO/arduino/serialIn.h>
  #elif defined(ARDUINO_ARCH_RP2040)
    #include <oneChip/clock.h>
    #include <oneMenu/menu/IO/arduino/serialOut.h>
    #include <oneMenu/menu/IO/arduino/serialIn.h>
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

// SysTick — hw::Period<N> wrapper, platform-agnostic ----------------------------
  #ifdef __AVR__
    using namespace onePin;
    using SysTick = chip::SysTick0<>;
  #elif defined(ARDUINO_ARCH_RP2040)
    struct SysTick { template<uint32_t Ms> using Period = hw::Period<Ms>; };
  #else
    struct SysTick { template<uint32_t Ms> using Period = hw::Period<Ms>; };
  #endif

bool running = true;

// I/O -----------------------------------------------------------------------
InDef<
  #ifdef ARDUINO
    SerialIn,
  #else
    LinuxKeyIn,
  #endif
  IdParser,
  PCKbd
> in;

// FullPrinter, not ScrollPrinter: required by Liquid/LiquidPos (see file header).
OutDef<
  FullPrinter,
  ANSIFmt,
  DataParser<>,
  CtrlChars,
  ColorTrack<int>,
  Cursor<>,
  Gate,
  ANSIOut,
  #ifdef ARDUINO
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<0,0>,
  StaticArea<40,10>
> out;

// text -------------------------------------------------------------------------
namespace text {
  static constexpr CText title  {"Liquid demo"};
  static constexpr CText op1    {"Option 1"};
  static constexpr CText toggle {"Move indicator"};
  static constexpr CText quit   {"Exit!"};
  static constexpr CText rec    {"[REC]"};   // fixed indicator, top-right corner
  static constexpr CText at_a   {"@A"};      // runtime-movable indicator
}

// ids ----------------------------------------------------------------------------
enum ids { mover_id };

// actions --------------------------------------------------------------------
namespace action {
  bool op1(Sz) { return true; }
  bool quit(Sz) { running=false; return true; }
  bool toggle(Sz);  // defined below mainMenu — needs find<> on it
}

using Quit = ItemDef<Action<action::quit>, StaticText<&text::quit>>;

// menu ---------------------------------------------------------------------------
// - "[REC]" is Liquid<35,0>: always rendered at the fixed corner (35,0), regardless
//   of where it sits in the body or how the rest of the menu redraws.
// - "@A" carries LiquidPos + Id<mover_id>: starts at (35,1); action::toggle() below
//   relocates it between two fixed points at runtime via find<>.
auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Liquid demo"},
  staticBody(
    ItemDef<Action<action::op1>, StaticText<&text::op1>>{},
    ItemDef<Action<action::toggle>, StaticText<&text::toggle>>{},
    Quit{},
    ItemDef<Liquid<35,0>, StaticText<&text::rec>>{},
    ItemDef<Id<ids::mover_id>, LiquidPos, StaticText<&text::at_a>>{}
  )
);

INavDef<
  IndexGo,
  TreeNav,
  Root<decltype(mainMenu), mainMenu>
> nav;

bool action::toggle(Sz) {
  static bool atA=true;
  auto& mover = mainMenu.find<SameAs<Id<ids::mover_id>>>();
  mover.liquidPos(atA ? Pos{35,2} : Pos{35,1});
  atA=!atA;
  return true;
}

// loop ----------------------------------------------------------------------------
bool run() {
  static SysTick::Period<30> fps;
  if (fps) {
    fps.reset();
    bool input=nav.in(in);
    (void)input;
    if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  }
  if (!fps) hw::delay_ms(fps.when() - hw::millis());
  return running;
}

void setup() {
  #ifdef ARDUINO_ARCH_RP2040
    Serial.begin(115200);
    while (!Serial) delay(10);
  #endif
  out.lockMode(LockMode::None);
  out.setColors(WHITE, BLACK);
  out.clear();
  nav.printTo(out);
}

#ifdef ARDUINO
void loop() { run(); }
#else
int main() {
  setup();
  while (run());
  return 0;
}
#endif
