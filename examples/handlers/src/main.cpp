/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AM4 handlers.ino compat-macro port — the "events" showcase. See the
 *        original at github.com/neu-rah/ArduinoMenu examples/handlers/handlers/
 *        handlers.ino: a single showEvent() handler observing every raised
 *        event across a submenu, a plain op, and a field.
 *
 * Deliberately trimmed vs. the original — real AM4 features it also
 * demonstrates that this compat layer doesn't support yet, dropped rather
 * than faked:
 *  - altOP's custom prompt subclass (virtual printTo() override) — a
 *    different paradigm than OneMenu's compile-time components.
 *  - EDIT()'s masked/pattern text editing (name/hex fields) — not ported.
 *  - the idle system (nav.idleOn/idleTask) — not built yet (see notes.md
 *    "idle can be a Nav component...").
 *  - altFIELD's custom decimal-places field type, and the LED TOGGLE — both
 *    dropped for the same reason as the next point (nothing new to show
 *    once FIELD's/SUBMENU's event wiring is already demonstrated).
 *
 * Also: am4.h's OP()/MENU()/TOGGLE()/SELECT()/CHOOSE() macros don't yet wire
 * their fn/mask args into a real event handler — only FIELD() does, via
 * EventCall (see am4.h's own FIELD() doc comment). Real AM4 semantics have
 * every item's fn *always* be an event handler; this compat layer's OP()
 * instead maps fn to the older, unrelated Action<fn> (bool(int)) — a v1
 * simplification that predates the event system (see am4.h's OP() doc
 * comment: "mask is accepted but ignored"). Rather than silently redesign
 * that already-shipped contract as a side effect of this port, items below
 * that need the real showEvent handler splice a native EventAction<mask,fn>
 * component in alongside the macro-built ones — "components first" (notes.md
 * "AM4 compat layer" design principle, 2026-07-03).
 *
 * The selftest drives nav.up()/down()/enter()/esc() directly (same style as
 * examples/am4compat's own selftest; the real input-driven poll()/in() path
 * is already covered by that example's own regression checks, not repeated
 * here).
 */

#include <oneMenu/compat/am4.h>
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/streamOut.h>
#include <oneMenu/menu/in.h>
#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>
#include <cassert>
#include <cstdio>

using namespace hapi;
using namespace oneData;
// NOTE: deliberately no `using namespace oneMenu;` — see examples/am4compat's
// own comment for why (oneMenu::Menu<> collides with AM4's Menu namespace).

namespace action {
  int enterCount = 0, exitCount = 0, focusCount = 0, blurCount = 0;

  const char* eventName(oneMenu::EventMask e) {
    switch(e) {
      case oneMenu::EventMask::Enter: return "enterEvent";
      case oneMenu::EventMask::Exit:  return "exitEvent";
      case oneMenu::EventMask::Focus: return "focusEvent";
      case oneMenu::EventMask::Blur:  return "blurEvent";
      default: return "?";
    }
  }
  // AM4's real showEvent(eventMask,navNode&,prompt&) — trimmed to the
  // bool(EventMask) shape EventAction supports (no nav/path context yet;
  // see notes.md "AM4 compat layer" open question on a nav-context variant).
  bool showEvent(oneMenu::EventMask e) {
    if(e & oneMenu::EventMask::Enter) enterCount++;
    if(e & oneMenu::EventMask::Exit)  exitCount++;
    if(e & oneMenu::EventMask::Focus) focusCount++;
    if(e & oneMenu::EventMask::Blur)  blurCount++;
    printf("event: %s\n", eventName(e));
    return true;
  }
  bool op1(int) { printf("Op1 executed\n"); return true; }

  // FIELD()'s fn slot wires EventCall (void(), opaque to *which* event fired
  // — see am4.h's FIELD() doc comment), a different, narrower handler shape
  // than showEvent's. Masked to enterEvent only (not anyEvent, unlike the
  // AM4 original) so Focus/Blur passing over this field don't also count as
  // a "field updated" — a deliberate deviation, not a limitation of the fix.
  int fieldEvents = 0;
  void onFieldEvent() { fieldEvents++; printf("event: field updated\n"); }
}

float test = 55;

// Native menuDef (not the MENU() macro) so EventAction can be attached to
// the submenu's own item slot — Menu<T,B,MM...>'s title is a plain data
// member, not part of the inheritance chain, so an event component has to
// be one of MM... (see notes.md: "attaching EventAction to a submenu's
// title" — MENU()'s macro doesn't expose that slot today).
auto subMenu = oneMenu::menuDef<oneMenu::EventAction<oneMenu::EventMask::Any, action::showEvent>>(
  oneMenu::ItemDef<Text>{"Sub-Menu"},
  oneMenu::staticBody(
    // OP() ignores fn/mask (v1) — spliced natively so Enter is observable.
    oneMenu::ItemDef<oneMenu::Action<action::op1>,
                      oneMenu::EventAction<oneMenu::EventMask::Any, action::showEvent>,
                      Text>{"Sub1"},
    EXIT("<Back")
  )
);

MENU(mainMenu, "Main menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,FIELD(test, "Test", "%", 0, 100, 1, 0, action::onFieldEvent, Menu::enterEvent, Menu::noStyle)
  ,SUBMENU(subMenu)
  ,EXIT("<Back")
);

ANSI_OUT(devOut, 40, 10);

// deterministic zero-op input source — the selftest drives nav directly.
struct NoOpIn {
  template<typename In> struct Part : In {
    static bool available() { return false; }
    static oneMenu::CKE cmd() { return {}; }
  };
};
oneMenu::InDef<NoOpIn> devIn;

MENU_INPUTS(in, &devIn);
MENU_OUTPUTS(out, /*maxDepth*/2, &devOut);
NAVROOT(nav, mainMenu, /*maxDepth*/2, in, out);

int main() {
  devOut.lockMode(oneMenu::LockMode::None);
  devOut.setColors(WHITE, BLACK);
  devOut.clear();
  nav.printTo(devOut);

  // index 0=Test(field), 1=Sub-Menu, 2=<Back>  (Cmd::Up increments, Down decrements)
  assert(action::fieldEvents == 0);
  nav.enter();  // enters edit mode on the Test field -> EventCall fires (Enter)
  assert(action::fieldEvents == 1 && "FIELD()'s EventCall did not fire entering edit mode");
  nav.enter();  // leaves edit mode -> EventCall fires again (documented FIELD() deviation)
  assert(action::fieldEvents == 2 && "FIELD()'s EventCall did not fire leaving edit mode");

  assert(action::focusCount == 0 && action::blurCount == 0);
  nav.up();     // 0 -> 1 (Test -> Sub-Menu): Blur has no handler on Test, Focus does on Sub-Menu
  assert(action::focusCount == 1 && action::blurCount == 0 &&
         "Sub-Menu's own EventAction did not observe Focus");

  assert(action::enterCount == 0);
  nav.enter();  // opens Sub-Menu (level 0 -> 1) -> Enter fires on the Sub-Menu item itself
  assert(nav.level() == 1 && action::enterCount == 1 &&
         "Sub-Menu's own EventAction did not observe Enter on open");
  nav.enter();  // Sub1 (index 0 inside Sub-Menu) -> its own EventAction + Action<op1> both fire
  assert(action::enterCount == 2 && "Sub1's EventAction did not observe Enter");

  assert(action::exitCount == 0);
  nav.esc();    // closes Sub-Menu (level 1 -> 0) -> Exit fires on the Sub-Menu item
  assert(nav.level() == 0 && action::exitCount == 1 &&
         "Sub-Menu's own EventAction did not observe Exit on close");

  printf("OK: handlers.ino compat-macro port (showEvent/FIELD-EventCall) verified\n");
  return 0;
}
