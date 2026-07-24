/**
 * @file test_idtext.cpp
 * @brief OneMenu render-level test for oneMenu::IdText (item.h) — confirms
 * the web-format (XmlFmt/JsonFmt) gate: TextFmt must resolve real text,
 * JsonFmt must emit a marked id ("#N") instead, both from the SAME item.
 *
 * Native-only (no PlatformIO unit-test runner used here — see OneData's own
 * test/test.cpp for the same direct-g++ convention this mirrors).
 */

#include <sstream>
#include <cstdio>

#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/streamOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/fmt/jsonFmt.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

static const char* const idtext_en[] = {"Zero","One"};
static const char* const idtext_pt[] = {"Zero","Um"};
static const char* const* const idtext_table[] = {idtext_en, idtext_pt};
using TestSrc = FlashLangSrc<idtext_table, 2>;

using TestItem = ItemDef<AsLabel<oneMenu::IdText<1,TestSrc>>>;

static auto testMenu = menuDef<>(
  ItemDef<Text>{"t"},
  staticBody(TestItem{})
);

static std::ostringstream textStream;
using TextOut = OutDef<
  FullPrinter, TextFmt, DataParser<>, Cursor<1,1>,
  StreamOut<std::ostringstream, textStream>,
  StaticPos<0,0>, StaticArea<80,25>
>;

static std::ostringstream jsonStream;
using JsonOut = OutDef<
  FullPrinter, JsonFmt, DataParser<>, Cursor<1,1>,
  StreamOut<std::ostringstream, jsonStream>,
  StaticPos<0,0>, StaticArea<80,25>
>;

int main() {
  MultiLangText::current = 0;   // english: idtext_en[1] = "One"

  TextOut textOut;
  textOut.lockMode(LockMode::None);
  NavDef<TreeNav, Root<decltype(testMenu),testMenu>> nav1;
  nav1.printTo(textOut);
  std::string t = textStream.str();
  bool textOk = t.find("One") != std::string::npos && t.find("\"1\"") == std::string::npos;
  printf("TextFmt resolves real text: %s (got: %s)\n", textOk?"PASS":"FAIL", t.c_str());

  MultiLangText::current = 1;   // portuguese: idtext_pt[1] = "Um" — same id, different language

  JsonOut jsonOut;
  jsonOut.lockMode(LockMode::None);
  NavDef<TreeNav, Root<decltype(testMenu),testMenu>> nav2;
  nav2.printTo(jsonOut);
  std::string j = jsonStream.str();
  bool jsonOk = j.find("\"lbl\":\"#1\"") != std::string::npos && j.find("Um") == std::string::npos;
  printf("JsonFmt emits marked id: %s (got: %s)\n", jsonOk?"PASS":"FAIL", j.c_str());

  return (textOk && jsonOk) ? 0 : 1;
}
