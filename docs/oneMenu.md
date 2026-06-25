# OneMenu User Guide

Menu system for embedded C++. Zero overhead, composable, cross-platform (AVR, STM32, ESP32, native).

---

## Quick start

```cpp
#include <oneMenu/oneMenu.h>
using namespace oneMenu;

InDef<LinuxKeyIn, PCKbd> in;
IOutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars, ANSIOut, ConsoleOut,
        StaticPos<20,10>, StaticArea<30,8>> out;

bool quit(Sz) { running=false; return true; }

auto menu = menuDef<WrapNav>(
  ItemDef<Text>{"My menu"},
  staticBody(
    ItemDef<Action<quit>, StaticText<&text::quit>>{},
  )
);

INavDef<TreeNav, Root<decltype(menu), menu>> nav;

void setup() {
  out.lockMode(LockMode::None);
  out.setColors(WHITE, BLACK);
  out.clear();
  nav.printTo(out);
}

bool run() {
  nav.in(in);
  if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  return running;
}
```

---

## Item building blocks

Items are composed by listing components inside `ItemDef<...>`. Order matters — components higher in the list wrap those below.

### Text labels

| Component | Usage | Notes |
|---|---|---|
| `CText` | `static constexpr CText name{"label"};` | Compile-time string, zero RAM on AVR |
| `StaticText<&name>` | `ItemDef<StaticText<&name>>` | Prints the `CText` constant |
| `Text` | `ItemDef<Text>{"label"}` | Runtime `const char*`, set at construction |

```cpp
static constexpr CText quit_text{"Exit!"};
using Quit = ItemDef<StaticText<&quit_text>>;  // zero RAM label
ItemDef<Text>{"runtime label"}                 // set at runtime
```

### Roles

| Component | What it does |
|---|---|
| `AsLabel<...>` | Marks enclosed text as the item label (shown on the left) |
| `AsField<...>` | Marks enclosed data as the editable/selectable value |
| `AsUnit<...>` | Appends a unit string after the field value |
| `AsEditMode<>` | Shows an edit-mode indicator when the item is being edited |

```cpp
ItemDef<AsLabel<StaticText<&text::power>>,     // "Power"
        AsField<Watch<Default<Int,55>>>,         //  55
        AsUnit<StaticText<&text::percent>>>{}    //  %
```

### Data

| Component | What it does |
|---|---|
| `Watch<T>` | Wraps `T`, signals `changed()` when value changes |
| `Default<T, V>` | Wraps `T`, sets initial value to `V` |
| `Int` | `Data<int>` — integer storage |
| `Bool` | `Data<bool>` — bool storage |
| `EnDis<B>` | Enable/disable flag, initial state `B` (`true`=enabled) |

```cpp
Watch<Default<Int, 55>>   // int, starts at 55, change-notifies
Watch<EnDis<false>>       // starts disabled
```

### Identity & enable/disable

```cpp
Id<N>                // assigns compile-time integer id N to this item
Watch<EnDis<false>>  // item starts disabled
```

Use `Id<N>` to find and manipulate items at runtime:

```cpp
auto& item = find<SameAs<Id<N>>>(menu);
item.enable(false);         // disable
item.enable(!item.enabled()); // toggle
```

---

## Menu structure

### `menuDef` — build a menu

```cpp
auto menu = menuDef<NavOptions...>(
  title_item,
  staticBody(item1, item2, ...)
);
```

`menuDef` returns a value — the menu lives in the variable. Menus compose freely; pass one as a body item to create a submenu:

```cpp
auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"Main"},
  staticBody(
    menuDef<WrapNav>(
      ItemDef<StaticText<&text::settings>>{},
      staticBody(/* ... */)
    ),
    Quit{}
  )
);
```

### Body types

| Type | Characteristics |
|---|---|
| `StaticBody<I1,I2,...>` / `staticBody(i1,i2,...)` | Compile-time, heterogeneous items, zero overhead |
| `CArrayBody<T, arr, N>` | C array of same-type items, no virtual functions |
| `CPtrArrayBody<I, arr, N>` | C array of `I*` pointers, virtual dispatch |
| `StdBody<Container>` | `std::vector<I*>` or similar, runtime size |

```cpp
// same-type C array — no virtual functions
CItem files[] = {"file1.txt", "file2.txt", "file3.txt"};
MenuDef<ItemDef<...>, CArrayBody<CItem, files, 3>>{};

// runtime-populated std container
MenuDef<ItemDef<...>, StdBody<vector<IItem*>>, Id<container_id>>{};
// then in setup:
auto& m = find<SameAs<Id<container_id>>>(mainMenu);
m.body.push_back(new IItemDef<Text>{"dynamic item"});
```

---

## Fields

Fields are items that hold and display an editable value.

### `NumFieldDef` — numeric field

```cpp
using Power = NumFieldDef<
  Chain<Id<power_id>, AsLabel<StaticText<&text::power>>>,  // label
  NumField<
    StaticNumRange<StaticRange<0,100,false>>,               // range min,max,wrap
    AsField<Watch<Default<Int,55>>>                         // data
  >,
  AsUnit<StaticText<&text::percent>>                        // unit
>;
Power{55}  // construct with initial value
```

Up/Down keys change the value within range. `false` = clamp, `true` = wrap.

### `TextFieldDef` / inline text

Inline text editing with cursor:

```cpp
ItemDef<
  AsLabel<Text>,
  AsEditMode<>,
  EditField,
  ParentDraw,
  AsField<TextField<15>>   // max 15 chars
>{"Name"}
```

Or with the alias:
```cpp
using NameField = TextFieldDef<AsLabel<StaticText<&text::name>>, 15>;
```

`EditField` — activates editing on Enter. `ParentDraw` — renders inline (no sub-level). `TextField<N>` — manages the character buffer and cursor.

### `ToggleFieldDef` — cycle through options on Enter

Displays the current option inline. Each Enter advances to the next (wraps automatically).

```cpp
using EdgeMode = ToggleFieldDef<
  ItemDef<StaticText<&text::edge>, AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::rise>>>,
    ItemDef<AsField<StaticText<&text::fall>>>,
    ItemDef<AsField<StaticText<&text::both>>>
  >,
  BodyAction<action::onChange>
>;
EdgeMode{}
```

### `SelectFieldDef` — pick from inline list

Opens the option list inline (no sub-level navigation). Selection shown on the item row.

```cpp
using BrightnessSelect = SelectFieldDef<
  ItemDef<AsLabel<StaticText<&text::brightness>>, AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::s25>>, AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s50>>, AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s75>>, AsUnit<StaticText<&text::percent>>>
  >,
  WrapNav
>;
```

### `ChooseFieldDef` — navigate into sub-body

Opens as a full submenu level. Chosen option shown on the item row after returning.

```cpp
using ProfileSelect = ChooseFieldDef<
  ItemDef<StaticText<&text::profile>, AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::profile_a>>>,
    ItemDef<AsField<StaticText<&text::profile_b>>>,
    ItemDef<AsField<StaticText<&text::profile_c>>>
  >,
  WrapNav
>;
```

### Composite field — `padDef`

Groups multiple fields into a single row (inline pad):

```cpp
auto dateField(const char* lbl) {
  return padDef(
    ItemDef<AsLabel<Text>, AsEditMode<>>{lbl},
    staticBody(
      ItemDef<EditField, ParentDraw,
              NumField<StaticNumRange<StaticRange<1900,2150,true>>,
                       AsField<Watch<Default<Int,2026>>>>>{2026},
      ItemDef<StaticText<&text::dot>, EditField, ParentDraw, AsEditMode<>,
              NumField<StaticNumRange<StaticRange<1,12,true>>, AsField<Watch<Int>>>>{1},
      ItemDef<StaticText<&text::dot>, EditField, ParentDraw, AsEditMode<>,
              NumField<StaticNumRange<StaticRange<1,31,true>>, AsField<Watch<Int>>>>{1}
    )
  );
}
// usage: dateField("date")
```

### Getting the value from enumerated fields (Toggle/Select/Choose)

Call `visit(fn)` on the field — the lambda receives the currently selected body `ItemDef&`:

```cpp
field.visit([](auto& item) { return item.getId(); });
field.visit([](auto& item) { return item.key(); });   // if item has Key<V>
```

Put typed values on body items using `Id<V>` or a custom component:

```cpp
ItemDef<AsField<StaticText<&text::rise>>, Id<RISING>>
// then:
field.visit([](auto& item) { return item.getId(); }); // returns RISING
```

---

## Navigation

### `INavDef` — the navigator

```cpp
INavDef<TreeNav, Root<decltype(myMenu), myMenu>> nav;
```

`Root<T, ref>` binds the nav to an existing menu variable. `StaticRoot<MenuType>` owns the menu inside the nav instead.

### Nav options (pass to `menuDef<>` or `INavDef<>`)

| Component | What it does |
|---|---|
| `TreeNav` | Hierarchical navigator — tracks path, level, selection |
| `WrapNav` | Navigation wraps from last item to first and vice versa |

`WrapNav` is per-menu — add it to each `menuDef<>` where you want wrapping:

```cpp
menuDef<WrapNav>(title, body)      // wraps
menuDef<>(title, body)             // clamps at ends
```

`ToggleFieldDef` always wraps (baked in).

### Running the navigator

```cpp
nav.in(in);                             // process input
if (nav.changed(out)) {
  nav.printTo(out);                     // render changed items
  nav.sync(out);                        // mark as up-to-date
}
```

### Force full redraw

```cpp
out.lockMode(LockMode::None);
nav.printTo(out);
```

---

## Output

### `IOutDef` vs `OutDef`

`IOutDef` — auto-injects `Gate`/`ColorTrack`/`Cursor` at the head. Use for your primary output device.

`OutDef` — no auto-injection. Use for secondary outputs (overlays, footers). Add tracking components explicitly if using ANSI:

```cpp
// primary — auto-injected
IOutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars, ANSIOut, ConsoleOut,
        StaticPos<20,10>, StaticArea<30,8>> out;

// secondary overlay — explicit
OutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars,
       ColorTrack<int>, Cursor<>, Gate, ANSIOut, ConsoleOut,
       StaticPos<24,12>, StaticArea<22,4>> promptOut;
```

### Printers

| Component | What it does |
|---|---|
| `FullPrinter` | Redraws all visible items every frame |
| `ScrollPrinter` | Redraws only changed items; scrolls when body exceeds area |

### Format components

| Component | What it does |
|---|---|
| `ANSIFmt` | ANSI colors, cursor highlight, edit-mode indicators |
| `TextFmt` | Plain text cursors and decorations, no escape codes |
| `DataParser<>` | Converts data values to characters |
| `CtrlChars` | Translates control characters (newline, clear, etc.) |
| `TextWrap` | Long text continues on next line |
| `Clip` | Keeps content inside the defined area |

### ANSI output device chain

```
ANSIFmt → DataParser<> → CtrlChars → [ColorTrack/Cursor/Gate] → ANSIOut → ConsoleOut/SerialOut
```

`ANSIOut` injects ANSI escape codes into the stream. `ConsoleOut` writes to stdout.

### Position and area

```cpp
StaticPos<col, row>      // top-left corner in character units
StaticArea<width, height> // display area in characters
```

Relative positioning (use after defining a prior output):

```cpp
StaticPos<decltype(out)::orgX()+4, decltype(out)::orgY()+2>
StaticArea<decltype(out)::width()-8, 4>
```

### Colors and lock mode

```cpp
out.lockMode(LockMode::None);   // force full redraw next printTo
out.setColors(WHITE, BLACK);    // foreground, background
out.clear();                    // erase the output area
out.resume();                   // re-anchor device to tracked position/colors
```

---

## Actions and events

### `Action<fn>` — item action

Called when the item is activated (Enter on a non-edit item). Return `true` to keep the menu open, `false` to close this level.

```cpp
bool myAction(Sz index) {
  // index = position of this item in its body
  return true;
}
ItemDef<Action<myAction>, StaticText<&text::label>>{}
```

### `BodyAction<fn>` — field selection callback

Called when a value is confirmed in a field body (Toggle/Select/Choose). `index` is the selected body position.

```cpp
bool onChange(Sz index) { /* react to new selection */ return false; }
ToggleFieldDef<..., StaticBody<...>, BodyAction<onChange>>
```

### `OnFocus<...>` — render on focus

Renders enclosed content to a secondary output when this item is focused. Used for description panels or footers.

```cpp
template<typename... OO>
using Desc = OnFocus<typename Put<OO...>::template ToOut<decltype(footer), footer, Clear::yes>>;

ItemDef<StaticText<&text::op1>, Desc<StaticText<&text::desc_op1>>>{}
```

---

## Runtime queries

### `find<Q>` — locate an item in the menu tree

```cpp
auto& item = find<SameAs<Id<N>>>(menu);        // by id
auto& item = find<TagIs<MyTag>>(menu);          // by tag
```

### Enable / disable at runtime

```cpp
auto& item = find<SameAs<Id<op3_id>>>(mainMenu);
item.enable(false);
item.enable(!item.enabled());
```

### Change detection

```cpp
nav.changed(out)   // true if any visible item changed since last sync
item.changed()     // true if this item's value changed
nav.sync(out)      // mark all as synced
item.sync()        // mark this item as synced
```

---

## Patterns and tricks

### Dialog / prompt (handler pointer swap)

```cpp
char promptMsg[48]{};

using RunFn = bool(*)();
RunFn activeRun;

bool mainRun()  { /* nav loop */ return running; }
bool promptRun() { /* promptNav loop */ return running; }

void showPrompt(const char* msg) {
  strncpy(promptMsg, msg, sizeof(promptMsg)-1);
  activeRun = promptRun;
  promptOut.lockMode(LockMode::None);
  promptOut.setColors(BLACK, WHITE);
  promptOut.clear();
  promptNav.printTo(promptOut);
}

bool action::ok(Sz) {
  promptOut.clear();
  activeRun = mainRun;
  out.lockMode(LockMode::None);
  nav.printTo(out);
  return true;
}
```

The prompt menu title is `ItemDef<Text>{promptMsg}` — `Text` stores a pointer, so `promptMsg` changes are picked up at render time.

Any number of modes (prompt, idle, sub-app) follow the same pattern.

### Idle / inactivity

```cpp
static SysTick::Period<30000> idleTimer;  // 30 seconds — file scope

bool mainRun() {
  bool input = nav.in(in);
  if (input) idleTimer.reset();
  if (nav.changed(out)) { nav.printTo(out); nav.sync(out); }
  if (idleTimer) { idleTimer.reset(); showIdle(); }
  return running;
}

bool idleRun() {
  if (nav.in(in)) {            // any key wakes up
    idleTimer.reset();
    activeRun = mainRun;
    out.lockMode(LockMode::None);
    nav.printTo(out);
  }
  return running;
}
```

`idleTimer` must be file-scope (not local to `mainRun`) so `idleRun` can reset it on wake.

### Typed value on enumerated field body items

Put `Id<V>` (or any component with a value accessor) on body items:

```cpp
StaticBody<
  ItemDef<AsField<StaticText<&text::rise>>, Id<RISING>>,
  ItemDef<AsField<StaticText<&text::fall>>, Id<FALLING>>,
  ItemDef<AsField<StaticText<&text::both>>, Id<CHANGE>>
>
```

Retrieve via `visit`:

```cpp
int mode = field.visit([](auto& item) { return item.getId(); });
```

### Toggle option 3 at runtime

```cpp
enum ids { op3_id };
// in menu: ItemDef<Id<op3_id>, Watch<EnDis<false>>, ...>

auto& op3 = find<SameAs<Id<op3_id>>>(mainMenu);
op3.enable(!op3.enabled());
nav.printTo(out);  // or let the next changed() catch it
```

### `menuDef` vs `MenuDef`

`menuDef<Opts...>(title, body)` — factory function, returns a value, menu stored in a variable.

`MenuDef<Title, Body, Opts...>{}` — direct type alias, construct inline as a body item (no external variable needed).

```cpp
// as a body item:
MenuDef<ItemDef<StaticText<&text::sub>>, StaticBody<...>, WrapNav>{}
```

### `padDef` for multi-column inline fields

`padDef(label_item, body)` creates a single row displaying all body items inline. Each body item uses `EditField` + `ParentDraw` so it edits without opening a sub-level. Use `StaticText` separators between columns.

---

## Cross-platform setup

```cpp
#ifdef __AVR__
  using SysTick = chip::SysTick0<>;
  IOP_TIMER0_ISR(Board)
  // InDef: SerialIn / OutDef: SerialOut
#elif defined(__arm__)
  using SysTick = chip::SysTick<>;
  IOP_SYSTICK_ISR(Board)
#else
  struct SysTick {
    template<uint32_t Ms> using Period = hw::Period<Ms>;
  };
  // InDef: LinuxKeyIn / OutDef: ConsoleOut
#endif
```

The 30fps loop pattern:

```cpp
bool run() {
  static SysTick::Period<30> fps;
  if (fps) { fps.reset(); activeRun(); }
  if (!fps) hw::delay_ms(fps.when() - hw::millis());
  return running;
}
```
