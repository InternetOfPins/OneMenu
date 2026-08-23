/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief OneMenu fields example — text, numeric, toggle, select, choose, date, prompt, idle
 */

// ── Includes ──────────────────────────────────────────────────────────────────
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/textFmt.h>  // must precede ansiFmt.h (defines MenuChars)
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#include <oneMenu/menu/IO/idxParser.h>

#ifdef __AVR__
  #include <onePin/onePin.h>
  #include <chips/avr/avrDevice.h>
  #include <oneMenu/menu/IO/arduino/serialOut.h>
  #include <oneMenu/menu/IO/arduino/serialIn.h>
  #ifdef IOP_GFX
    // Optional alternative output target (build_flags -D IOP_GFX, [env:unoGfx]):
    // real Adafruit_ST7789 SPI TFT in place of plain Serial/ANSI — same
    // proven, hardware-verified stack as an earlier local prototype.
    // Mutually exclusive with the default ANSI/serial output, not a second parallel one —
    // every GFX-only block below is guarded the same way this file already
    // guards ARDUINO/RP2040/arm/native, just one more axis on top of __AVR__.
    #include <SPI.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_ST7789.h>
    #include <Fonts/FreeSansBold12pt7b.h>
    #include <oneIO/display/adaGfxVendor.h>
    #include <oneMenu/menu/IO/IOP/oledOut.h>
    #include <oneMenu/menu/fmt/gfxColorFmt.h>
    #include <oneMenu/menu/fmt/vendorFont.h>
  #endif
#elif defined(ARDUINO_ARCH_RP2040)
  #include <oneChip/clock.h>
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
  using namespace hw::avr;  // brings chip::/AVR:: into scope (hw::avr::chip, hw::avr::AVR)
  using SysTick = chip::SysTick0<>;
  using Led1    = AVR::OutPin<Pins<5>, chip::PortB>;
  using Board   = AVR::Board<Boot<SysTick>, Led1>;
  #ifdef IOP
  IOP_TIMER0_ISR(Board)
  #endif
#elif defined(ARDUINO_ARCH_RP2040)
  struct SysTick {
    template<uint32_t Ms>
    using Period = hw::Period<Ms>;
  };
#elif defined(__arm__)
  using namespace onePin;
  using namespace oneBit;
  using SysTick = chip::SysClk<>;
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

#if defined(__AVR__) && defined(IOP_GFX)
// ── GFX device (optional) ───────────────────────────────────────────────────
// Pins/panel/font match an earlier local prototype exactly (hardware-
// verified there) — hardware SPI, board default MISO/MOSI/SCK.
#define TFT_CS  10
#define TFT_RST  9
#define TFT_DC   8
Adafruit_ST7789 gfx(TFT_CS, TFT_DC, TFT_RST);

// 12x16 pixel cells (6x8 base font at setTextSize(2) = 12x16 glyph, exact).
using Oled = oneIO::display::AdaGfxVendor</*W*/240, /*H*/320, /*CharW*/12, /*CharH*/16,
                                            /*Fg*/0xFFFF, /*Bg*/0x0000>;
Sz gfxLineHeight() { return (Sz)Oled::lineHeight(); }

// ST77XX_* vendor color constants (Adafruit_ST7789.h), not local BLACK/WHITE/...
// redefinitions: this file already unconditionally includes ansiOut.h (hence
// ansiCodes.h) above for the default ANSI output, whose own BLACK/WHITE/etc.
// are unnamespaced ints (see feedback_ansi_codes_global_namespace note) — a
// same-named local RGB565 constexpr here would collide/redefine.
using MyColors = typename Color<uint16_t>::template Table<
  /*Title*/   typename Color<uint16_t>::template Colors<ST77XX_BLUE,ST77XX_WHITE>,
  /*Default*/ typename Color<uint16_t>::template Colors<ST77XX_WHITE,ST77XX_BLUE>,
  /*View*/    typename Color<uint16_t>::template Colors<ST77XX_WHITE,ST77XX_BLUE>,
  /*Nav*/ typename Color<uint16_t>::template Nav<
    /*Enabled*/ typename Color<uint16_t>::template Enabled<
      typename Color<uint16_t>::template Item<
        /*Body*/     typename Color<uint16_t>::template Colors<ST77XX_WHITE,ST77XX_BLUE>,
        /*Field*/    typename Color<uint16_t>::template Colors<ST77XX_GREEN,ST77XX_WHITE>,
        /*EditMode*/ typename Color<uint16_t>::template Colors<ST77XX_BLUE,ST77XX_WHITE>
      >,
      /*Selected*/ typename Color<uint16_t>::template Colors<ST77XX_WHITE,ST77XX_GREEN>
    >,
    /*Disabled*/ typename Color<uint16_t>::template Enabled<
      typename Color<uint16_t>::template Item<
        /*Body*/     typename Color<uint16_t>::template Colors<ST77XX_BLACK,ST77XX_BLUE>,
        /*Field*/    typename Color<uint16_t>::template Colors<ST77XX_BLACK,ST77XX_WHITE>,
        /*EditMode*/ typename Color<uint16_t>::template Colors<ST77XX_BLUE,ST77XX_WHITE>
      >,
      /*Selected*/ typename Color<uint16_t>::template Colors<ST77XX_BLACK,ST77XX_GREEN>
    >
  >
>;
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
  IdxParser,
  PCKbd
> in;

#if defined(__AVR__) && defined(IOP_GFX)
// Same chain shape as an earlier local prototype's TftOut (hardware-
// verified there): ScrollPrinter + GFX-aware fmt/ctrl/wrap layers + Cursor
// (real partial-update capability, HasPartialUpdate — see nav.h) + Gate +
// VendorGfxOut<Oled> + per-role ColorTable, instead of ANSIFmt+ANSIOut+
// Serial/ConsoleOut. Mutually exclusive with the ANSI branch below, not a
// second parallel output.
OutDef<
  ScrollPrinter,
  FontSwitch<&FreeSansBold12pt7b>,
  GfxColorFmt<2,0,/*BigTitle*/false>,
  DataParser<>,
  GfxCtrlChars,
  GfxTextWrap,
  Cursor<Oled::charWidth(), Oled::lineSpacing(), nullptr, &gfxLineHeight>,
  Gate,
  VendorGfxOut<Oled>,
  ColorTable<MyColors>,
  StaticPos<0, 0>,
  StaticArea<Oled::kWidth, Oled::kHeight>,
  LineSpacing<1,5>
> out;

// prompt overlay — inset over out, same fmt/color chain, FullPrinter (no
// scroll needed for a single "OK" item)
OutDef<
  FullPrinter,
  FontSwitch<&FreeSansBold12pt7b>,
  GfxColorFmt<2,0,false>,
  DataParser<>,
  GfxCtrlChars,
  GfxTextWrap,
  Cursor<Oled::charWidth(), Oled::lineSpacing(), nullptr, &gfxLineHeight>,
  Gate,
  VendorGfxOut<Oled>,
  ColorTable<MyColors>,
  StaticPos<decltype(out)::orgX()+8, decltype(out)::orgY()+40>,
  StaticArea<decltype(out)::width()-16, 60>,
  LineSpacing<1,5>
> promptOut;
#else
OutDef<
  ScrollPrinter,
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
  #ifdef ARDUINO
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<decltype(out)::orgX()+4, decltype(out)::orgY()+2>,
  StaticArea<decltype(out)::width()-8, 4>
> promptOut;
#endif

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
// AsEditMode<> deliberately listed FIRST — it's an attribute-only Fmt tag
// (XmlFmt's own attr_tags) that must fire while this item's own XML tag is
// still open; a label/text component listed before it would open+close its
// own child tag first, force-closing the item's tag in the process, and
// AsEditMode's mode="..." would land as malformed loose text instead of a
// real attribute (found 2026-07-22 rendering this exact item through
// XmlFmt — same root cause NumFieldDef itself had, fixed in fields.h).
using ToggleDemo = ToggleFieldDef<
  ItemDef<AsEditMode<>, StaticText<&text::toggle_demo>>,
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
    AsEditMode<>,
    AsLabel<StaticText<&text::select_demo>>
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
    AsEditMode<>,
    StaticText<&text::choose_demo>
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
    ItemDef<AsEditMode<>, AsLabel<Text>>{lbl},
    staticBody(
      ItemDef<
        EditField,
        ParentDraw,
        NumField<StaticNumRange<StaticRange<1900,2150,true>>, AsField<Watch<Default<Int,2026>>>>
      >{2026},
      ItemDef<
        AsEditMode<>,
        StaticText<&text::dateSep>,
        EditField, ParentDraw,
        NumField<StaticNumRange<StaticRange<1,12,true>>, AsField<Watch<Int>>>
      >{1},
      ItemDef<
        AsEditMode<>,
        StaticText<&text::dateSep>,
        EditField, ParentDraw,
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
        // GFX: trimmed field set — Uno/Nano's 32KB flash can't fit the
        // Adafruit_GFX+Adafruit_ST7789 vendor stack AND every field type at
        // once (measured: full set overflowed by ~8.4KB). Power+ToggleDemo
        // still exercise real field editing (NumField live-edit, inline
        // cycling) through the GFX chain; TextField/SelectDemo/ChooseDemo/
        // dateField are ANSI/serial-only here, not dropped from the library.
        #ifdef IOP_GFX
          Power{55},
          ToggleDemo{},
          Back{}
        #else
          ItemDef<
            AsEditMode<>,
            AsLabel<Text>,
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
        #endif
      )
    ),
    Quit{}
  )
);

INavDef<
  IndexGo,
  TreeNav<>,
  Root<mainMenu>
> nav;

bool action::op2(Sz) {
  auto& op3 = mainMenu.find<SameAs<Id<ids::op3_id>>>();  // member find<> searches body
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
  TreeNav<>,
  Root<promptMenu>
> promptNav;

// ── Handler machinery ─────────────────────────────────────────────────────────
// Was hand-rolled here (a plain RunFn activeRun, swapped by showIdle()/
// showPrompt()/action::ok()) — now oneMenu::RunLoop<mainFn>, the same
// pattern formalized into the library (see nav.h's own doc comment for why:
// this exact code, duplicated near-identically across several examples,
// was the real motivating case).
bool mainRun();  // forward — RunLoop binds this as its compile-time default
using Run = RunLoop<mainRun>;

static SysTick::Period<30000> idleTimer;

void showIdle();  // forward

bool mainRun() {
  bool input = nav.in(in);
  if (input) idleTimer.reset();
  #if defined(__AVR__) && defined(IOP_GFX)
    // doOutput(), not the manual changed()/printTo()/sync() below: it forces a
    // full clear+redraw on nav.levelChanged() (entering/leaving Settings) and
    // restores lockMode to Update afterward — without it, a GFX partial-update
    // chain leaves stale glyphs from the old item set showing through the new
    // one (same fix as an earlier local prototype's own loop()).
    nav.doOutput(out);
  #else
    if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  #endif
  if (idleTimer) { idleTimer.reset(); showIdle(); }
  return running;
}

bool idleRun() {
  if (nav.in(in)) {
    idleTimer.reset();
    Run::idleOff();  // back to mainRun
    out.lockMode(LockMode::None);
    nav.printTo(out);
  }
  return running;
}

bool promptRun() {
  promptNav.in(in);
  #if defined(__AVR__) && defined(IOP_GFX)
    promptNav.doOutput(promptOut);
  #else
    if (promptNav.changed(promptOut)) {
      promptNav.printTo(promptOut);
      promptNav.sync(promptOut);
    }
  #endif
  return running;
}

void showIdle() {
  Run::idleOn(idleRun);
  out.clear();
  out.resume();
  out << "z z z" << endl;
}

void showPrompt(const char* msg) {
  strncpy(promptMsg, msg, sizeof(promptMsg)-1);
  Run::idleOn(promptRun);
  promptOut.lockMode(LockMode::None);
  // GFX: per-role coloring comes from ColorTable<MyColors> (composed into the
  // chain itself), not a runtime setColors() override — ANSI-only call.
  #if !(defined(__AVR__) && defined(IOP_GFX))
    promptOut.setColors(BLACK, WHITE);
  #endif
  promptOut.clear();
  promptNav.printTo(promptOut);
}

bool action::op1(Sz) { showPrompt("Option 1 activated!"); return true; }
bool action::ok(Sz)  {
  promptOut.clear();
  Run::idleOff();  // back to mainRun
  out.lockMode(LockMode::None);
  nav.printTo(out);
  return true;
}

// ── Loop ──────────────────────────────────────────────────────────────────────
bool run() {
  static SysTick::Period<30> fps;
  if (fps) {
    fps.reset();
    Run::run();
  }
  if (!fps) hw::delay_ms(fps.when() - hw::millis());
  return running;
}

void setup() {
  #ifdef ARDUINO_ARCH_RP2040
    Serial.begin(115200);
    while (!Serial) delay(10);
  #endif
  #if defined(__AVR__) && defined(IOP_GFX)
    // Same init sequence as an earlier local prototype's setup() —
    // hardware-verified there (MADCTL fix + invertDisplay(false) needed for
    // correct colors on this cheap ST7789 clone).
    gfx.setSPISpeed(40000000);
    gfx.init(240, 320);
    gfx.invertDisplay(false);
    gfx.setRotation(0);
    { uint8_t madctl = ST77XX_MADCTL_MX | 0x08;
      gfx.sendCommand(ST77XX_MADCTL, &madctl, 1); }
    delay(50);
    gfx.setTextSize(2);
    Oled::begin(gfx);
    Oled::primeBaseFontHeight();          // built-in font active by default
    Oled::setFont(&FreeSansBold12pt7b);
    Oled::primeCustomFontHeight();
    Oled::setFont(nullptr);               // restore built-in before real rendering
  #endif
  out.lockMode(LockMode::None);
  // GFX: per-role coloring comes from ColorTable<MyColors> — ANSI-only call.
  #if !(defined(__AVR__) && defined(IOP_GFX))
    out.setColors(WHITE, BLACK);
  #endif
  out.clear();
  // Run::alternative already starts at mainRun (RunLoop's own static init) —
  // no explicit "activeRun=mainRun" needed here anymore.
  nav.printTo(out);
  #if defined(__AVR__) && defined(IOP_GFX)
    // sync(Out&) restores lockMode to whatever it was at entry (still None) —
    // without this, mainRun()'s first doOutput() call starts at None too, so
    // only THAT call's own end-of-doOutput reset would engage Update; explicit
    // here for the very first frame, same reasoning as the earlier prototype.
    nav.sync(out);
    out.lockMode(LockMode::Update);
  #endif
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
