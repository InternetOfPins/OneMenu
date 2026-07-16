# OneMenu API Reference

Menu framework for embedded UIs. Compile-time menu composition with runtime navigation.

## Quick Start

```cpp
#include <oneMenu/oneMenu.h>
#include <oneOutput/oneOutput.h>

// Define data
oneData::Data<int> counter(0);

// Define menu structure
using MainMenu = oneMenu::Menu<
  oneMenu::ItemDef<oneData::StaticText<"Counter">>,
  oneMenu::ItemDef<oneMenu::NumField<oneData::StaticNumRange<0, 100>, counter>>
>;

// Define navigation
using Nav = oneMenu::TreeNav<MainMenu>;

// Define output (ANSI to serial)
using Out = oneMenu::ANSIOut;

// Create instances
Nav nav;
Out out;

int main() {
  nav.doCmd(oneMenu::Cmd::Down);  // Navigate down
  nav.doOutput(out);              // Render menu
}
```

---

## Menu Structure

### Item Components

| Type | Purpose |
|------|---------|
| `Menu<...>` | Hierarchical submenu container |
| `ItemDef<Components...>` | Item definition (field, action, submenu) |
| `StaticText<"Label">` | Read-only text label |
| `StaticBody<...>` | Fixed list of items (compile-time) |
| `Action<Fn>` | Callable menu item |
| `NumField<Range, Data>` | Editable number input |
| `TextField<MaxLen, Mask, Data>` | Editable text input |
| `ToggleBehave` | Item toggles instead of entering submenu |
| `ParentDraw` | Draw parent context behind this item |
| `AsFmt<Fmt, Component>` | Apply format directive |

### Menu Navigation

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `bool` | `Nav::doCmd(Cmd c)` | command | Process input command (Up/Down/Enter/Esc) |
| `bool` | `Nav::doOutput(Out& out)` | output | Render current menu state |
| `bool` | `Nav::changed()` | – | Did selection change? |
| `bool` | `Nav::enabled()` | – | Is current item enabled? |
| `void` | `Nav::reset()` | – | Return to root menu |

### Commands

| Command | Purpose |
|---------|---------|
| `Cmd::Up` | Move selection up |
| `Cmd::Down` | Move selection down |
| `Cmd::Enter` | Select/edit current item |
| `Cmd::Esc` | Go back/cancel edit |
| `Cmd::Left` | Adjust value (left) |
| `Cmd::Right` | Adjust value (right) |

---

## Input Integration

### Input Adapters (OneMenu ↔ OneInput)

| Type | Purpose | Include |
|------|---------|---------|
| `EncIn<HW, Steps>` | Rotary encoder adapter | `<oneMenu/menu/IO/IOP/encIn.h>` |
| `BtnIn<HW, Click, Hold>` | Button adapter | `<oneMenu/menu/IO/IOP/btnIn.h>` |
| `JoyIn<HW>` | Joystick adapter | `<oneMenu/menu/IO/IOP/joyIn.h>` |
| `EncoderIn<CS, Steps>` | Platform-agnostic encoder | `<oneMenu/encoderIn.h>` |
| `DebouncedButton<CS, Ms>` | Platform-agnostic button | `<oneMenu/debouncedButton.h>` |
| `EdgeDetector<CS>` | Platform-agnostic edge detector | `<oneMenu/edgeDetector.h>` |

**Usage**:
```cpp
using Encoder = oneMenu::EncoderIn<chip::OnChange<A0, A1, A2>, 4>;
Encoder encoder;

if (encoder.available()) {
  CKE cmd = encoder.cmd();
  nav.doCmd(cmd.cmd);
}
```

---

## Output Integration

### Output Formats

| Type | Purpose | Include |
|------|---------|---------|
| `ANSIOut` | ANSI color escape sequences | `<oneOutput/ansiOut.h>` |
| `ANSIFmt` | ANSI format directives | `<oneOutput/ansiOut.h>` |
| `StreamOut<Stream>` | Serial/cout output adapter | `<oneOutput/streamOut.h>` |
| `LcdOut` | HD44780 character LCD | `<oneOutput/lcdOut.h>` |
| `U8g2Out` | U8g2 graphics LCD | `<oneOutput/u8g2Out.h>` |

### Printer Components

| Type | Purpose |
|------|---------|
| `MenuPrinter<TitlePrinter, BodyPrinter, ItemPrinter>` | Menu layout |
| `ScrollBodyPrinter` | Scrolling list view |
| `FullScreenPrinter` | Full-screen mode (one item per screen) |
| `ItemPrinter<IndexPrinter, CursorPrinter, BodyPrinter>` | Item formatting |
| `IndexPrinter` | Item index/number display |
| `NavCursorPrinter` | Selection indicator (arrow/highlight) |

---

## Data Binding

### Data Types

| Type | Purpose |
|------|---------|
| `Data<T>` | Runtime mutable variable |
| `StaticText<"...">` | Compile-time constant string |
| `StaticNumRange<Min, Max>` | Numeric range constraint |
| `Watch<T>` | Observable variable (tracks changes) |
| `Default<Default, Data>` | Data with initial value |

**Usage**:
```cpp
oneData::Data<int> power(50);
oneData::Watch<oneData::Default<oneData::Data<int>, 55>> watched(55);

using PowerField = oneMenu::NumField<
  oneData::StaticNumRange<0, 100>,
  oneData::Watch<oneData::Default<oneData::Data<int>, 55>>
>;
```

---

## Event Handling

### Event System

| Enum | Purpose |
|------|---------|
| `EventMask::Enter` | Item entered/focused |
| `EventMask::Exit` | Item left/unfocused |
| `EventMask::Focus` | Selection moved onto item |
| `EventMask::Blur` | Selection moved away |

**Usage**:
```cpp
struct MyAction {
  void on(EventMask mask) {
    if (mask == EventMask::Enter) {
      handle_enter();
    }
  }
};
```

---

## Text Formatting

### Format Directives

| Enum | Purpose |
|------|---------|
| `Fmt::View` | Read-only display |
| `Fmt::Title` | Menu title bar |
| `Fmt::Menu` | Menu container |
| `Fmt::Body` | Menu body/list |
| `Fmt::Item` | Individual item |
| `Fmt::Label` | Field label |
| `Fmt::EditMode` | Editing indicator |
| `Fmt::EditCursor` | Cursor during edit |
| `Fmt::Field` | Field value display |
| `Fmt::Unit` | Measurement unit |

**Usage**:
```cpp
using LabeledField = oneMenu::ItemDef<
  oneMenu::AsFmt<oneMenu::Fmt::Label, StaticText<"Power">>,
  oneMenu::NumField<...>
>;
```

---

## Common Patterns

### Simple Number Field
```cpp
oneData::Data<int> value(50);

using Field = oneMenu::NumField<
  oneData::StaticNumRange<0, 100>,
  oneData::Watch<oneData::Default<oneData::Data<int>, 50>>
>;
```

### Encoder-Driven Menu
```cpp
using Enc = oneMenu::EncoderIn<chip::OnChange<A0, A1, A2>, 4>;
Enc encoder;

// Main loop
while(true) {
  if (encoder.available()) {
    CKE cmd = encoder.cmd();
    nav.doCmd(cmd.cmd);
  }
  nav.doOutput(out);
}
```

### Multi-Page with Scrolling
```cpp
using MainMenu = oneMenu::Menu<
  oneMenu::ItemDef<...>,
  // ... 20+ items ...
>;

using Printer = oneMenu::MenuPrinter<
  oneMenu::TitlePrinter,
  oneMenu::ScrollBodyPrinter,  // Auto-scroll if > 8 items
  oneMenu::ItemPrinter<...>
>;

using Nav = oneMenu::TreeNav<MainMenu>;
```

### Full-Screen Editing
```cpp
using FullScreenPrinter = oneMenu::FullScreenPrinter;

using Printer = oneMenu::MenuPrinter<
  oneMenu::TitlePrinter,
  oneMenu::FullScreenPrinter,  // One item per screen
  oneMenu::ItemPrinter<...>
>;
```

---

## Lock Modes (Output Control)

| Mode | Effect |
|------|--------|
| `LockMode::None` | Full redraw every poll |
| `LockMode::Update` | Draw only changed regions |
| `LockMode::Sync` | Sync data only, no draw |
| `LockMode::Measure` | Calculate without rendering |
| `LockMode::Changed` | Check if anything changed |

---

## See Also

- [OneChip](../../../OneChip/docs/REFERENCE.md) — Hardware platforms
- [OneInput](../../../OneInput/docs/REFERENCE.md) — Input drivers
- [OneOutput](../../../OneOutput/docs/REFERENCE.md) — Output formatters
- [OneData](../../../OneData/docs/REFERENCE.md) — Data binding & observation
