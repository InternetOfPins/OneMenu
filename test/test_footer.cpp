/**
 * @file test_footer.cpp
 * @brief OneMenu render-level test for oneMenu::Footer<id,Src> (item.h) — a
 * per-item mouseover/description text component. Confirms the per-format
 * split: XmlFmt/JsonFmt emit it as a real child tag for EVERY item that has
 * one (unconditional, "printed together" in the same pass, not focus-gated
 * — a mouse can hover any visible item); TextFmt emits it as an indented
 * follow-up line ONLY for the currently focused item (neurMenu's own
 * OnFocus-split precedent, adapted); any other format is a no-op.
 *
 * Native-only (no PlatformIO unit-test runner used here — see OneData's own
 * test/test.cpp for the same direct-g++ convention this mirrors).
 */

#include <sstream>
#include <cstdio>
#include <cstring>

#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/streamOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/fmt/jsonFmt.h>
#include <oneMenu/menu/fmt/xmlFmt.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

namespace text {
  enum Id { txtA, txtB, txtADesc, txtBDesc, txtCount };
  static const char* const table_en[] = {"A","B","desc of A","desc of B"};
  static const char* const* const table[] = {table_en};
  using Src = FlashLangSrc<table,1>;
}

namespace action { bool noop(int){return true;} }

// Item A: AsLabel-wrapped IdText (Power/LangSel's own composition style).
// Item B: bare IdText, no AsLabel (Op1/Op2/Op3/Settings' own composition
// style, webMenu/src/main.cpp) — jsonFmt.h auto-opens an unclosed "lbl"
// property for this shape (autoLbl), which Fmt::Footer's own fmtStart must
// explicitly closeAutoLbl() before opening "footer", or the two properties
// run together into invalid JSON (caught exactly this way, real bug).
auto testMenu = menuDef<WrapNav>(
  ItemDef<Text>{"t"},
  staticBody(
    ItemDef<Action<action::noop>, AsLabel<oneMenu::IdText<text::txtA,text::Src>>, oneMenu::Footer<text::txtADesc,text::Src>>{},
    ItemDef<Action<action::noop>, oneMenu::IdText<text::txtB,text::Src>, oneMenu::Footer<text::txtBDesc,text::Src>>{}
  )
);
using TestMenu = decltype(testMenu);

template<typename Fmt>
std::string render() {
  static std::ostringstream ss;
  ss.str("");
  using Out = OutDef<FullPrinter, Fmt, DataParser<>, Cursor<1,1>, StreamOut<std::ostringstream,ss>, StaticPos<0,0>, StaticArea<80,25>>;
  Out out;
  out.lockMode(LockMode::None);
  NavDef<TreeNav, Root<testMenu>> nav;
  nav.printTo(out);
  return ss.str();
}

int fails=0;
void check(bool cond, const char* msg) {
  if(!cond) { printf("FAIL: %s\n", msg); fails++; }
  else printf("ok: %s\n", msg);
}

int main() {
  // TextFmt: item A is focused by default (idx 0) -> should show its own
  // description as a follow-up indented line; item B (not focused) should NOT.
  std::string t = render<TextFmt>();
  check(t.find("desc of A") != std::string::npos, "TextFmt shows focused item's description");
  check(t.find("desc of B") == std::string::npos, "TextFmt hides unfocused item's description");

  // JsonFmt: BOTH items' footer should appear, unconditionally (not focus-gated).
  std::string j = render<JsonFmt>();
  check(j.find("\"footer\":\"desc of A\"") != std::string::npos, "JsonFmt emits item A's footer (AsLabel-wrapped)");
  check(j.find("\"footer\":\"desc of B\"") != std::string::npos, "JsonFmt emits item B's footer (bare IdText, no AsLabel)");
  check(j.find("\"lbl\":\"#1\",\"footer\":\"desc of B\"") != std::string::npos, "JsonFmt: item B's auto-opened lbl closes cleanly before footer opens");

  std::string x = render<XmlFmt>();
  check(x.find("<footer><![CDATA[desc of A]]></footer>") != std::string::npos, "XmlFmt emits item A's <footer>");
  check(x.find("<footer><![CDATA[desc of B]]></footer>") != std::string::npos, "XmlFmt emits item B's <footer> (even though unfocused)");

  printf(fails==0 ? "\nALL OK\n" : "\n%d FAILURES\n", fails);
  return fails;
}
