// OneMenu mono_block tests
//
// Verifies OneMenu components under the mono_block HAPI chain:
//   1. OutDef basic collapse — put() flows through DataParser→Cursor→Gate→device
//   2. query<IsFormat>       — TextFmt detected via OutDef::Types
//   3. query<IsCursor>       — Cursor detected via OutDef::Types
//   4. find<IsCursor>        — runtime ref to Cursor component, pos() starts at {0,0}
//   5. query<IsArea>         — StaticArea tag visible through chain
//   6. ItemDef + printItem   — item prints through TextFmt chain, output captured

#include <cassert>
#include <sstream>
#include <iostream>
using namespace std;

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneOutput/oneOutput.h>
#include <oneItem/oneItem.h>
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/streamOut.h>
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/sys/printers.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

// ─── Shared chain types ───────────────────────────────────────────────────────

// Minimal headless output chain — no ANSI, no Buffer, no ScrollPrinter.
// StaticArea<80,1> satisfies Cursor's IsArea requirement.
using MinOut = OutDef<
  DataParser<>,
  CtrlChars,
  Cursor,
  Gate,
  ConsoleOut,
  StaticPos<0,0>,
  StaticArea<80,1>
>;

// Same chain with a format layer added — satisfies rules for both TextFmt and ScrollPrinter.
using FmtOut = OutDef<
  ScrollPrinter,
  TextFmt,
  DataParser<>,
  CtrlChars,
  Cursor,
  Gate,
  ConsoleOut,
  StaticPos<0,0>,
  StaticArea<80,4>
>;

// ─── Helper: capture cout to a string ────────────────────────────────────────

template<typename F>
static string capture(F fn) {
  ostringstream buf;
  streambuf* old = cout.rdbuf(buf.rdbuf());
  fn();
  cout.rdbuf(old);
  return buf.str();
}

// ─── Test 1: OutDef basic collapse ───────────────────────────────────────────

void test_outdef_collapse() {
  string out = capture([]{
    MinOut o;
    o.lockMode(LockMode::None);
    o.put('A'); o.put('B'); o.put('C');
  });
  assert(out == "ABC");
  cout << "PASS test_outdef_collapse  out=" << out << "\n";
}

// ─── Test 2: query<IsFormat> ─────────────────────────────────────────────────

void test_query_format() {
  static_assert( query<IsFormat, FmtOut>, "IsFormat should be found in FmtOut (has TextFmt)");
  static_assert(!query<IsFormat, MinOut>, "IsFormat should not be found in MinOut (no format layer)");
  cout << "PASS test_query_format\n";
}

// ─── Test 3: query<IsCursor> ─────────────────────────────────────────────────

void test_query_cursor() {
  static_assert(query<IsCursor, MinOut>, "IsCursor should be found in MinOut (has Cursor)");
  cout << "PASS test_query_cursor\n";
}

// ─── Test 4: find<IsCursor> — runtime component access ───────────────────────

void test_find_cursor() {
  MinOut o;
  auto& cur = hapi::find<IsCursor>(o);
  Pos p = cur.pos();
  assert(p.x == 0 && p.y == 0);
  o.lockMode(LockMode::None);
  o.put('x'); o.put('y');  // advance cursor
  p = cur.pos();
  assert(p.x == 2 && p.y == 0);
  cout << "PASS test_find_cursor  pos after 2 puts: {" << p.x << "," << p.y << "}\n";
}

// ─── Test 5: query<IsArea> — StaticArea tag visible through chain ─────────────

void test_query_area() {
  static_assert(query<IsArea, MinOut>, "IsArea (StaticArea) should be found in MinOut");
  static_assert(query<IsArea, FmtOut>, "IsArea (StaticArea) should be found in FmtOut");
  cout << "PASS test_query_area\n";
}

// ─── Test 6: ItemDef printItem through TextFmt chain ─────────────────────────

static const CText txt_hello{"hello"};

void test_item_print() {
  string out = capture([]{
    FmtOut o;
    o.lockMode(LockMode::None);
    oneMenu::ItemDef<oneData::StaticText<&txt_hello> > item;
    Ctx ctx{{0}};
    item.printItem(o, ctx);
  });
  assert(out.find("hello") != string::npos);
  cout << "PASS test_item_print  captured: [" << out << "]\n";
}

// ─────────────────────────────────────────────────────────────────────────────

#ifdef ARDUINO
  void setup() {
    Serial.begin(115200);
    while (!Serial);
    test_outdef_collapse();
    test_query_format();
    test_query_cursor();
    test_find_cursor();
    test_query_area();
    test_item_print();
  }
  void loop() {}
#else
  int main() {
    test_outdef_collapse();
    test_query_format();
    test_query_cursor();
    test_find_cursor();
    test_query_area();
    test_item_print();
    cout << "\nAll tests passed.\n";
    return 0;
  }
#endif
