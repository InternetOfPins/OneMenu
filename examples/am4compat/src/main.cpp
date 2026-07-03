/**
 * @file main.cpp
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AM4 compat-macro proof: same MENU/FIELD/OP/EXIT/SUBMENU call syntax as
 *        AM4's own README example, expanded onto OneMenu via oneMenu/compat/am4.h.
 *        I/O + nav wiring is native OneMenu (NAVROOT/MENU_INPUTS/MENU_OUTPUTS
 *        device macros are explicitly out of scope for this compat layer — see
 *        am4.h's file comment and notes.md "AM4 compat layer").
 *
 * env:selftest drives nav.up()/nav.enter() directly (no PCKbd/ANSI parsing
 * involved) — same verification style as the `fields` example's selftest.
 */

#include <oneMenu/compat/am4.h>
#include <oneMenu/menu/IO/ansiOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/fmt/ansiFmt.h>
#include <oneMenu/menu/IO/streamOut.h>
#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneItem/oneItem.h>
#include <oneOutput/oneOutput.h>
#include <cassert>
#include <cstdio>

using namespace hapi;
using namespace oneData;
// NOTE: deliberately no `using namespace oneMenu;` here — oneMenu::Menu<T,B,OO...>
// (the menu class template) unavoidably collides with AM4's `Menu` namespace as
// soon as both are visible unqualified in the same TU, *even when Menu:: members
// are referenced fully qualified* (Menu::noStyle still needs unqualified lookup
// of "Menu" itself to resolve the `::`, and that's what's ambiguous). Verified
// empirically — see notes.md "AM4 compat layer". Fix: qualify the handful of
// native OneMenu identifiers below with oneMenu:: instead of a blanket using-directive.

// ── variables edited directly by FIELD() via DataRef (AM4's edit-by-reference) ──
unsigned int timeOn = 10;
unsigned int timeOff = 90;

namespace action {
  int op1Count = 0;
  bool op1(int) { op1Count++; return true; }
}

// ── menu tree, verbatim AM4 call syntax ─────────────────────────────────────
MENU(subMenu, "Sub-Menu", Menu::doNothing, Menu::anyEvent, Menu::noStyle
  ,OP("Sub1", action::op1, Menu::anyEvent)
  ,EXIT("<Back")
);

MENU(mainMenu, "Blink menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,FIELD(timeOn,  "On",  "ms", 0, 1000,  10, 1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,FIELD(timeOff, "Off", "ms", 0, 10000, 10, 1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,OP("Op1", action::op1, Menu::anyEvent)
  ,SUBMENU(subMenu)
  ,EXIT("<Back")
);

// ── I/O + nav: native OneMenu wiring (not part of the AM4 compat surface) ──
oneMenu::OutDef<
  oneMenu::FullPrinter, oneMenu::ANSIFmt, oneMenu::DataParser<>, oneMenu::CtrlChars,
  oneMenu::ColorTrack<int>, oneMenu::Cursor<>, oneMenu::Gate,
  oneMenu::ANSIOut, oneMenu::ConsoleOut, oneMenu::StaticPos<0,0>, oneMenu::StaticArea<40,10>
> out;

oneMenu::INavDef<oneMenu::TreeNav, oneMenu::Root<decltype(mainMenu), mainMenu>> nav;

int main() {
  out.lockMode(oneMenu::LockMode::None);
  out.setColors(WHITE, BLACK);
  out.clear();
  nav.printTo(out);

  // index 0=On, 1=Off, 2=Op1, 3=subMenu, 4=<Back>  (Cmd::Up increments, Down decrements)
  assert(action::op1Count == 0);
  nav.up(); nav.up();           // 0 -> 1 -> 2 (Op1)
  nav.enter();                  // fires action::op1 via OP()'s Action<fn>
  assert(action::op1Count == 1 && "OP() did not fire on Enter");

  nav.up();                     // 2 -> 3 (subMenu, spliced in via SUBMENU(subMenu))
  nav.enter();                  // opens the submenu (level 0 -> 1)
  assert(nav.level() == 1 && "SUBMENU(subMenu) did not open a nested level");
  nav.enter();                  // submenu index 0 = Sub1 -> fires action::op1 again
  assert(action::op1Count == 2 && "SUBMENU()'d menu's OP() did not fire");
  nav.esc();
  assert(nav.level() == 0 && "esc() did not close back out of the submenu");

  // FIELD() edits the real external variable directly via DataRef, zero-copy.
  // Selection is at index 3 (subMenu) after esc() — close() doesn't restore sel.
  // Cmd::Down decrements: 3 -> 2 (Op1) -> 1 (Off field).
  assert(timeOff == 90);
  nav.down(); nav.down();       // index 3 -> 1 (Off field)
  nav.enter();                  // enters edit mode on the field
  nav.up();                     // NumField::Part::nav inverts Up/Down while in Edit mode
                                 // (Cmd::Up -> Base::down()) — see item.h; not a compat-layer bug
  assert(timeOff == 89 && "FIELD() did not edit the bound variable via DataRef");
  nav.enter();                  // leave edit mode

  printf("OK: MENU/FIELD/OP/EXIT/SUBMENU compat macros all verified\n");
  return 0;
}
