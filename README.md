# OneMenu

HAPI-based composable menu system for embedded targets. Zero dynamic allocation, zero-overhead output chain, streaming-first (no host framebuffer). Tested on AVR, ESP32, and Linux native.

**HAPI Compatibility:** Updated for new Check/Apply/ApplyPack API (2026-Q2)

## Features

- Composable output chain: format + printer + cursor + raw device assembled at compile time
- Multiple simultaneous outputs (OLED, LCD, serial, web)
- Partial updates: only redraws changed items
- Navigation: tree menus, wrap/no-wrap, enter/esc/up/down
- Data items: editable numeric and string fields
- Web output: XML + XSLT, live navigation via browser
- Runtime item lookup by compile-time `Id<>`: `find<>`/`find()` walk the menu tree (title, chain, and nested bodies) and return a real reference to the tagged item

## Finding items by `Id<>`

Tag any item with `Id<V>` and look it up later — from an action callback, for example — to toggle, enable, or otherwise mutate it at runtime:

```cpp
enum ids { op3_id };

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Main menu"},
  staticBody(
    ItemDef<Action<action::op2>, Text>{"Option 2"},
    ItemDef<Id<ids::op3_id>, Action<action::op3>, Watch<EnDis<false>>, Text>{"Option 3"}
    // ...
  )
);

// "Option 2" toggles "Option 3"'s enabled state at runtime:
bool action::op2(Sz) {
  auto& op3 = mainMenu.find<SameAs<Id<ids::op3_id>>>();  // or: mainMenu.find(byId<ids::op3_id>)
  op3.enable(!op3.enabled());
  return true;
}
```

`find<>` searches the whole tree — a submenu's own `Id<>`, its title, or anything nested arbitrarily deep in its body — and is a compile error if the tag isn't found anywhere.

## Output devices

| Header | Device |
|--------|--------|
| `IO/IOP/oledOut.h` | SSD1306 OLED via [OneIO](https://github.com/InternetOfPins/OneIO) |
| `IO/IOP/lcdOut.h` | HD44780 character LCD via [OneIO](https://github.com/InternetOfPins/OneIO) |
| `IO/arduino/serialOut.h` | Arduino `Serial` |
| `IO/arduino/webOut.h` | ESP32 `WebServer` — streams XML + XSLT |
| `IO/streamOut.h` | `std::ostream` (native/Linux) |

## Formats

| Header | Target | Output |
|--------|--------|--------|
| `fmt/textFmt.h` | All | Plain text, one line per item |
| `fmt/ansiFmt.h` | Linux/native | ANSI color, cursor positioning, partial repaint |
| `fmt/gfxFmt.h` | OLED/pixel display | Pixel-level layout, rounded selection, big title |
| `fmt/xmlFmt.h` | Web/serial | XML with CDATA-wrapped labels; paired with `menu.xsl` |
| `fmt/jsonFmt.h` | Web/serial/debug | JSON object per item, all attributes as properties |
| `fmt/htmlFmt.h` | Web | HTML fragment |

Format layers slot into `OutDef<Printer, **Fmt**, DataParser<>, Cursor<>, Device, Pos, Area>`.

## Quick start

```cpp
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/IOP/oledOut.h>
#include <oneIO/display/i2cOled.h>
using namespace oneMenu;
using namespace oneIO::display;

using MyOled = I2cOledWire<Wire, SDA, SCL>;

namespace action {
  bool op2(Sz) { /* ... */ return true; }
}

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Main menu"},
  staticBody(
    ItemDef<StaticText<&text::op1>>{},
    ItemDef<Action<action::op2>, StaticText<&text::op2>>{}
  )
);

NavDef<TreeNav, Root<decltype(mainMenu), mainMenu>> nav;
OledDisplay<MyOled, GfxFmt<2, 0, true>> oledDisplay;

void setup() {
  MyOled::begin();
  oledDisplay.clear();
  oledDisplay.lockMode(LockMode::None);
  nav.printTo(oledDisplay);
}

void loop() {
  nav.in(in);
  if(nav.changed(oledDisplay)) {
    nav.printTo(oledDisplay);
    nav.sync(oledDisplay);
  }
}
```

## LCD output

```cpp
#include <oneMenu/menu/IO/IOP/lcdOut.h>
#include <oneIO/display/i2cLcd.h>

using MyLcd = oneIO::display::I2cLcd<AvrTwiMaster<>, 0x27>;
LcdDisplay<MyLcd, 20, 4> lcdDisplay;
// setup: MyLcd::begin(); lcdDisplay.clear(); lcdDisplay.lockMode(LockMode::None);
```

## Web output (ESP32)

```cpp
#include <oneMenu/menu/IO/arduino/webOut.h>
WebDisplay webDisplay;
WebOut::begin(webServer);
webServer.on("/", []() {
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/xml", "");
  webServer.sendContent("<?xml version=\"1.0\"?>\n<?xml-stylesheet type=\"text/xsl\" href=\"/menu.xsl\"?>\n");
  webDisplay.lockMode(LockMode::None);
  nav.printTo(webDisplay);
});
webServer.on("/menu.xsl", []() { webServer.send(200, "text/xsl", WebOut::xsl()); });
```

## Dependencies

- [HAPI](https://github.com/InternetOfPins/HAPI)
- [OneData](https://github.com/InternetOfPins/OneData)
- [OneItem](https://github.com/InternetOfPins/OneItem)
- [OneOutput](https://github.com/InternetOfPins/OneOutput)
- [OneParse](https://github.com/InternetOfPins/OneParse)
