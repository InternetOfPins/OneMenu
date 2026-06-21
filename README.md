# OneMenu

HAPI-based composable menu system for embedded targets. Zero dynamic allocation, zero-overhead output chain, streaming-first (no host framebuffer). Tested on AVR, ESP32, and Linux native.

## Features

- Composable output chain: format + printer + cursor + raw device assembled at compile time
- Multiple simultaneous outputs (OLED, LCD, serial, web)
- Partial updates: only redraws changed items
- Navigation: tree menus, wrap/no-wrap, enter/esc/up/down
- Data items: editable numeric and string fields
- Web output: XML + XSLT, live navigation via browser

## Output devices

| Header | Device |
|--------|--------|
| `IO/IOP/oledOut.h` | SSD1306 OLED via [OneIO](https://github.com/InternetOfPins/OneIO) |
| `IO/IOP/lcdOut.h` | HD44780 character LCD via [OneIO](https://github.com/InternetOfPins/OneIO) |
| `IO/arduino/serialOut.h` | Arduino `Serial` |
| `IO/arduino/webOut.h` | ESP32 `WebServer` — streams XML + XSLT |
| `IO/streamOut.h` | `std::ostream` (native/Linux) |

## Quick start

```cpp
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/IOP/oledOut.h>
#include <oneIO/display/i2cOled.h>
using namespace oneMenu;
using namespace oneIO::display;

using MyOled = I2cOledWire<Wire, SDA, SCL>;

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Main menu"},
  staticBody(
    ItemDef<StaticText<&text::op1>>{},
    ItemDef<StaticText<&text::op2>>{}
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
