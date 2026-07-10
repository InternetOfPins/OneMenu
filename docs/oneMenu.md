# OneMenu User Guide

Menu system for embedded C++. Zero overhead, composable, cross-platform (AVR, STM32, ESP32, native).

---

## Quick start

```cpp
#include <oneMenu/oneMenu.h>
using namespace oneMenu;

InDef<LinuxKeyIn, PCKbd> in;
IOutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars, Cursor<>, Gate, ANSIOut, ConsoleOut,
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

Items are composed by listing components inside `ItemDef<...>` — those components wrap
each other; order matters, components higher in the list wrap those below. The item itself
is closed once assembled: a menu body holds a flat list of already-closed items (see
[Body types](#body-types)), it doesn't fold/wrap them the way components inside a single
`ItemDef<...>` fold into each other.

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
auto& item = menu.find<SameAs<Id<N>>>();  // or: menu.find(byId<N>)
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
auto& m = mainMenu.find<SameAs<Id<container_id>>>();
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

Call `visit(fn)` on the field — the lambda receives the currently selected body `ItemDef&`. This is useful for reaching whatever real accessors that item exposes (`enabled()`, a `Watch<>`'s `get()`, etc.):

```cpp
field.visit([](auto& item) { return item.enabled(); });
```

`Id<V>` is a compile-time-only tag (used with `find<>` to locate an item — see
[Runtime queries](#runtime-queries)); it has no runtime accessor, so it can't be
used to recover which option was picked. For that, use the index `BodyAction<fn>`
already hands you and map it to your own enum:

```cpp
enum Edge { RISING, FALLING, CHANGE };

bool onChange(Sz index) {
  Edge e = static_cast<Edge>(index);  // index matches body item order
  return false;
}

using EdgeMode = ToggleFieldDef<
  ItemDef<StaticText<&text::edge>, AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::rise>>>,   // index 0 → RISING
    ItemDef<AsField<StaticText<&text::fall>>>,   // index 1 → FALLING
    ItemDef<AsField<StaticText<&text::both>>>    // index 2 → CHANGE
  >,
  BodyAction<onChange>
>;
```

---

## Layout

`Row`/`Rows` arrange several independently-printable items (typically `ItemDef<...>`) on
one line or across a header/body/footer stack. Both are compile-time-static (partition
sizes are template params, not runtime state) — see [Cross-platform setup](#cross-platform-setup)
for the general zero-overhead philosophy this follows.

### `Align<method,II...>` — grouping tag, no layout by itself

```cpp
AlignLeft<II...>    // = Align<AlignMethod::Left, II...>
AlignCenter<II...>  // = Align<AlignMethod::Center, II...>
AlignRight<II...>   // = Align<AlignMethod::Right, II...>
```

Groups `II...` the same way `Hidden<II...>`/`Decor<II...>` do — used on its own (nothing
routing it through `Row`) it just prints its content plain, unaligned. `Align` carries no
rendering logic; the method only takes effect inside a `Row`.

### `Row<Left,Center,Right>` — 3 items on one line

```cpp
using Test = ItemDef<Row<LeftItem,CenterItem,RightItem>>;
```

`Left`/`Center`/`Right` must each be a standalone printable+constructible type (typically
an `ItemDef<...>`). Measures all three with `LockMode::Measure` dry runs before printing any
for real, so `Center` correctly lands in the gap between `Left` and `Right` — something a
plain `Chain<Text,AlignCenter<Text>,AlignRight<Text>>` can't do, since each `Align` only
ever sees `out.free()` at the moment it runs, never what a later sibling will still consume.
Falls back to plain sequential printing on non-cursor (streaming) devices.

### `Rows<bottomLines,vMethod,Top,Body,Bottom>` — header/body/footer stack

```cpp
using Screen = ItemDef<Rows<1, AlignMethod::Center, HeaderItem, BodyItem, FooterItem>>;
// sugars: RowsTop<bottomLines,...>, RowsCenter<...>, RowsBottom<...>
```

Partitions the vertical space: `Top` prints, `Bottom` is pinned to the last `bottomLines`
rows (a footer), and `Body` is anchored within whatever's left via `vMethod` (`Left`=top,
`Center`, `Right`=bottom — reusing `AlignMethod`). `Left`/top needs no measurement; `Center`/
`Right` measure `Body`'s own line count first via a `LockMode::Measure` dry run (the
vertical twin of `Row`'s width measurement). `bottomLines` is a compile-time constant, not
runtime-adjustable — same static-partition rule as `Row`. Falls back to plain sequential
printing on non-cursor devices, where every item already spans the full row width anyway.

### `Liquid<x,y>` / `LiquidPos` — jump an item to a fixed screen position

```cpp
ItemDef<Liquid<10,3>, StaticText<&text::corner>>{}   // compile-time position

ItemDef<LiquidPos, StaticText<&text::badge>> badge{};
badge.liquidPos({10,3});                              // runtime-settable position
```

Both save the cursor position, jump, print, then restore — so the next sequential item
isn't displaced. Meaningless on non-cursor (streaming) devices, so both fall back to a
plain in-sequence print there. **Incompatible with `ScrollPrinter`/`NoTitleScrollPrinter`**
(scroll measurement assumes items advance the cursor sequentially — enforced via
`static_assert`, not just documented); use `FullPrinter`/`NoTitlePrinter` instead.

### `FullScreen` — one item, the whole page

Wraps an item so it claims all remaining vertical space after printing its own content
(pads down via `Cursor::clearFree()`/a native `fillRect` on GFX outputs). Combined with a
scroll-search printer, this turns a list into a single-item-per-page carousel — each item
gets the full display to itself as selection moves.

```cpp
using VCenterDemo = ItemDef<FullScreen, CenterRow<AsField<StaticText<&text::alpha>>>>;

auto menu = menuDef<WrapNav>(
  ItemDef<Text>{"FullScreen demo"},
  staticBody(
    ItemDef<FullScreen, CenterRow<AsField<StaticText<&text::alpha>>>>{},
    ItemDef<FullScreen, CenterRow<AsField<StaticText<&text::beta>>>>{}
  )
);
```

Requires a printer built on `aScrollBody` (`ScrollPrinter`/`NoTitleScrollPrinter`/
`SelectPrinter`/`NoTitleSelectPrinter` — see [Printers](#printers)) — `FullPrinter`/
`NoTitlePrinter` would just pad trailing blank lines with no paging effect
(`static_assert`-enforced). For a strict "always show exactly the selected item, nothing
else" carousel, prefer `SelectPrinter`/`NoTitleSelectPrinter` over `ScrollPrinter` —
`ScrollPrinter`'s general multi-item window search isn't built for a body where every
single item already consumes the whole page.

### Cascading color/font tables — `Color<Cor>`/`ColorTable<...>`, `Font<Fnt>`/`FontTable<...>`

`ANSIFmt`/`GfxFmt` resolve colors (and `GfxFmt` also fonts) from a compile-time table that
cascades: set one value at the root and it applies everywhere, override only the branch
that needs to differ.

```cpp
using MyPalette = Color<int>::Table<
  /*Title*/   Color<int>::Colors<BLUE,WHITE>,
  /*Default*/ Color<int>::Colors<WHITE,BLACK>   // Default cascades to View, Nav, everything
>;

IOutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars,
        ColorTable<MyPalette>,                  // placed below ANSIFmt/GfxFmt
        Cursor<>, Gate, ANSIOut, ConsoleOut,
        StaticPos<0,0>, StaticArea<40,10>> out;
```

Table shape (same for `Color<Cor>` and `Font<Fnt>`, just `Colors<f,b>` vs `Value<v>` leaves):

| Level | Meaning | Defaults from |
|---|---|---|
| `Table<Tit,Def,Vw,Nv>` | whole table | `Def`/`Vw`/`Nv` default to `Tit` |
| `Nav<En,Dis>` | enabled vs disabled | `Dis` defaults to `En` |
| `Enabled<It,Sel>` | item vs selected/focused | `Sel` defaults to `It::Body` |
| `Item<Bd,Fld,Ed>` | body/field/edit-mode role | `Fld`/`Ed` default to `Bd` |

Omit `ColorTable<...>`/`FontTable<...>` entirely to use the format's own built-in default
(`ANSIFmt`'s reproduces the original hardcoded ANSI palette; `GfxFmt`'s uses `bool` for
big/normal font selection, title big by default).

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

### `AsyncNav` — stateless path-based navigation (web/HTTP)

A second nav component (add to `NavDef<...>`, not `menuDef<...>`) that adds `async(path)`:
jump straight to an absolute index path (`"/1/3/"`, 0-based, trailing `/`) from a cold
start, no prior nav state needed — built for HTTP, where each request arrives independent
of the last and the nav can't just remember "where it was".

```cpp
NavDef<AsyncNav, TreeNav, Root<decltype(mainMenu), mainMenu>> webNav;  // separate instance
NavDef<TreeNav,  Root<decltype(mainMenu), mainMenu>>          nav;     // hardware nav, unaffected

webNav.async("/1/3/");             // reset to root, descend into submenu 1, select item 3
webNav.enter();                    // then click — triggers the action or opens edit mode
if (webNav.async(path)) webNav.set(val);  // or: navigate then set the now-focused field by string
```

Give the web nav its own `NavDef<...>` instance sharing the same `Root<menu>` as your
hardware nav — they read/write the same underlying `OneData` values (shared automatically,
since those live inside the menu struct) but keep independent cursor/level state, so a web
request never disturbs where the hardware nav currently is. See
[Web output](#web-output--xmlfmtjsonfmt-weboutwebdisplay) for the HTTP side of this.

### Force full redraw

```cpp
out.lockMode(LockMode::None);
nav.printTo(out);
```

---

## Input

### `InDef<Sources...>` — the input chain

Same shape as `IOutDef`/`OutDef`: a HAPI chain of sources, first with a pending command
wins. `available()`/`cmd()` are queried in list order.

```cpp
InDef<LinuxKeyIn, PCKbd> in;   // native/host testing: raw key codes + PC-keyboard mapping
nav.in(in);                    // feed into the navigator each loop
```

### OneInput hardware adapters

Bridge a real `oneInput::InputDef<...>` hardware chain (debounce/click/hold/encoder/
joystick filters — see [OneInput's own README](../../OneInput/README.md) for the filter
stack itself) into `CKE` navigation commands:

| Component | Wraps | Emits |
|---|---|---|
| `BtnIn<HW, ClickCmd=Enter, HoldCmd=Esc>` | `oneInput::BtnCapture` (+`Hold`/`Click`/`Debounce`) | `ClickCmd` on click, `HoldCmd` on hold |
| `EncIn<HW, Steps>` | `oneInput::Encoder` | `Up`/`Down` per `Steps` detents |
| `JoyIn<HW>` | `oneInput::Joystick` (+ADC axes) | `Left`/`Right`/`Up`/`Down` from deadzone-gated analog deltas |

```cpp
using BtnHW = oneInput::InputDef<
  oneInput::BtnCapture, oneInput::Hold<800>, oneInput::Click<300>, oneInput::Debounce<20>,
  oneInput::avr::AvrBtnPin<1, chip::PortC, 2>
>;
using EncHW = oneInput::InputDef<oneInput::Encoder, oneInput::avr::AvrEncPins<1, chip::PortC, 0, 1>>;
ISR(PCINT1_vect) { BtnHW::dispatch(); EncHW::dispatch(); }

using Btn = oneMenu::BtnIn<BtnHW>;
using Enc = oneMenu::EncIn<EncHW, 4>;
InDef<Enc, Btn, PCKbd> in;
```

The ISR (or a polled equivalent) drives the underlying `HW::dispatch()`; the `*In<HW>`
wrapper only translates already-captured events into `CKE` on each `nav.in(in)` poll.

### Runtime input sources — `IInDef`/`IIn`, `InList`, `InGroup`

Mirrors the output side's `IOutDef`/`IOut`/`OutList` split:

| Type | Shape |
|---|---|
| `IInDef<KK...>` | `InDef<KK...>` that also implements the virtual `IIn` interface |
| `InList<N>` | Runtime list of `IIn*` sources, polled in sequence, first with a pending event wins — a drop-in `in` object anywhere `InDef<...>` is used today |
| `InGroup<I, Ins...>` | Compile-time OR-combination of sources with no type erasure (unlike `InList`, not runtime-reconfigurable) |

```cpp
IInDef<LinuxKeyIn> kbdSrc;
IInDef<oneMenu::BtnIn<BtnHW>> btnSrc;
InList<2> in(&kbdSrc, &btnSrc);   // or default-construct + in.add(src) at runtime
nav.in(in);
```

Use `InList` when the active device set isn't fixed at compile time (e.g. a hot-pluggable
secondary input); use plain `InDef<...>` (or `InGroup` for a type-erasure-free static OR)
otherwise — it costs nothing extra.

---

## Output

### `IOutDef` vs `OutDef`

Neither auto-injects anything — both need the same explicit component list (printer,
format, parsers, `Cursor<>`/`Gate`/`ColorTrack<>` if the format needs them, device,
geometry). The only difference is `IOutDef` also implements the virtual `IOut` interface,
for code that needs to hold/pass an output by a common runtime-polymorphic type without
knowing its concrete component chain; `OutDef` has no virtual dispatch. Use `OutDef` unless
something specifically needs that runtime interface (e.g. a secondary output reached through
type-erased code).

```cpp
IOutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars, Cursor<>, Gate, ANSIOut, ConsoleOut,
        StaticPos<20,10>, StaticArea<30,8>> out;

// secondary overlay, same shape, ColorTrack<> added for independent ANSI color state
OutDef<FullPrinter, ANSIFmt, DataParser<>, CtrlChars,
       ColorTrack<int>, Cursor<>, Gate, ANSIOut, ConsoleOut,
       StaticPos<24,12>, StaticArea<22,4>> promptOut;
```

### Printers

| Component | What it does |
|---|---|
| `FullPrinter` | Title + body + footer; body shows every item in order, no scrolling/windowing |
| `ScrollPrinter` | Title + body + footer; body scrolls (window search) when it exceeds the area |
| `NoTitlePrinter` | Body + footer only, no title row — default shape for small displays |
| `NoTitleScrollPrinter` | `NoTitlePrinter` + scrolling body |
| `SelectPrinter` | Title + body + footer; body always shows exactly the selected item, nothing else |
| `NoTitleSelectPrinter` | `SelectPrinter` without the title row |

"Full" vs "Scroll" is about whether the body windows/searches for a scroll position when it
overflows the area — not about redraw frequency. Every printer here still goes through the
normal per-item `lockMode()`/`changed()` gating (see [Colors and lock mode](#colors-and-lock-mode)),
so partial updates (only redrawing items that actually changed) work the same under
`FullPrinter` as under `ScrollPrinter` on any device that supports it.

All six are `Chain<ViewPrinter, MenuPrinter<..., ItemsPrinter>>` compositions over the same
building blocks (`TitlePrinter`, `BodyPrinter`/`ScrollBodyPrinter`/`SelectBodyPrinter`) —
pick by title/no-title × body-fit strategy rather than hand-assembling the chain yourself.
`SelectPrinter`/`NoTitleSelectPrinter` are the ones [`FullScreen`](#fullscreen--one-item-the-whole-page)
items are meant to run under.

### Format components

| Component | What it does |
|---|---|
| `ANSIFmt` | ANSI colors, cursor highlight, edit-mode indicators |
| `TextFmt` | Plain text cursors and decorations, no escape codes |
| `GfxFmt<Radius,Spacing,BigTitle>` | Pixel-display format (inverted-video selection, optional big title font) — see [Pixel displays](#pixel-displays--gfxfmt) |
| `BtFmt` | Values-only compact format for BT/BLE payloads — see [BT/BLE output](#btble-output) |
| `XmlFmt` / `JsonFmt` | Whole-tree dump (paths, nav state, labels/fields) for a remote-viewer UI — see [Web output](#web-output--xmlfmtjsonfmt-weboutwebdisplay) |
| `HtmlFmt` | Complete HTML page per render, selection via CSS class, nav links embedded — see [Web output](#web-output--xmlfmtjsonfmt-weboutwebdisplay) |
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

### Pixel displays — `GfxFmt`

Format for GFX-capable devices (anything exposing `fillRect`, e.g. SSD1306/PCD8544 via
`OledOut`). Works in device-native coordinates — pixels horizontally, pages vertically for
SSD1306. Selection is shown as inverted video (`fillRect` + XOR'd font bytes); the plain
text `NavCursor` indicator is fully suppressed since inversion already marks the row.
Colors/fonts still go through the same `Color<Cor>`/`Font<Fnt>` cascading-table mechanism
as `ANSIFmt` (`Cor=bool` = inverted?, `Fnt=bool` = big/normal font).

```cpp
using MyOled = oneIO::display::I2cOledWire<Wire, 5, 4>;
OledDisplay<MyOled> display;                 // ready-made: FullPrinter+GfxFmt<>+OledOut
OledDisplay<MyOled, GfxFmt<2,0,true>> big;    // Radius, Spacing, BigTitle
```

`OledDisplay<Oled, GfxFmtT=GfxFmt<>, Extra...>` and `Nokia5110Display<Lcd, GfxFmtT, Extra...>`
are ready-made `OutDef`s deriving `Cursor<>` advances and the default area from the driver
(`kWidth`/`kHeight`/`charWidth`/`lineSpacing`); `Extra...` overrides position/area (earlier
entries in the HAPI chain win). For a hand-built chain (e.g. pairing `GfxFmt` with
`SelectPrinter` for a [`FullScreen`](#fullscreen--one-item-the-whole-page) carousel, which
neither ready-made alias offers), compose `OledOut<Oled>` directly — see
`menu/IO/IOP/oledOut.h`.

### Character LCD output

`LcdDisplay<LCD>` is the raw device adapter for HD44780-compatible character LCDs (same
slot as `ConsoleOut`/`SerialOut` — swap freely inside a hand-built `OutDef`). `LCD` must
expose `print(char)`/`print(const char*)`/`setCursor(col,row)`/`clear()` plus static
`cols`/`rows` (used for `IsArea`).

```cpp
using MyLcd = oneIO::display::I2cLcd<Twi, 0x27, 20, 4>;  // TwiMaster, Addr, Cols, Rows
LcdOut<MyLcd> lcdDisplay;                          // ready-made OutDef, TextFmt+NoTitleScrollPrinter
LcdOut<MyLcd, NoTitlePrinter> smallLcdDisplay;      // override the printer for a tiny device
```

`LcdOut<Lcd, Printer=NoTitleScrollPrinter>` is the ready-made `OutDef` (`Printer`+`TextFmt`+
`DataParser<>`+`Cursor<1,1>`+`LcdDisplay<Lcd>`) — cols/rows come from the LCD driver type
itself (e.g. `I2cLcd<...,Cols,Rows>` exposes them as `Cols`/`Rows` static members), not from
`LcdOut`'s own template params.

### BT/BLE output

Two independent paths for mirroring field values out over BLE GATT characteristics — same
distinction as the coarse-vs-fine choice elsewhere (per-field vs per-tree):

**Per-field** — tag the data component with `oneData::BTRec<W, Id>` (same composition shape
as `Watch`/`Default`, wraps freely):

```cpp
using Ch1 = NumFieldDef<
  Chain<AsLabel<StaticText<&text::ch1>>>,
  NumField<StaticNumRange<StaticRange<0,100,false>>,
           AsField<BTRec<Watch<DataRef<&currentCh1>>, btIds::ch1_bt_id>>>,
  AsUnit<StaticText<&text::percent>>
>;
```

Every `print()`/`printItem()` calls `out.btWrite<Id>(get())` — a no-op unless the `Out`
chain composes a matching `oneOutput::BtOut<Ble, Id>`, so an untagged/unwired field costs
nothing. `Ble` is duck-typed (`char_write`/`char_read`/`char_written`, see `oneBus::BleAPI`)
— ESP32 and nRF52 backends both verified on real hardware. Inbound (peer write → `set()`)
isn't wired yet; read `Ble::char_written(Id)`/`char_read(Id,...)` directly in your loop.

**Per-tree** — pair `BtFmt` (strips paths/labels/nav-cursor chrome, emits only Field/Data
values comma-separated) with `oneMenu::BtOut<Ble,Id,BufSz>` (accumulates the render into a
buffer, flushes as one characteristic write). `BtDisplay<Ble,Id,BufSz>` is the ready-made
`OutDef` for this:

```cpp
BtDisplay<Ble, my_menu_bt_id> btOut;
nav.printTo(btOut);   // whole visible subtree -> one characteristic write on flush()
```

Use per-field for individually-addressable values pushed on change; per-tree for a compact
whole-menu snapshot record.

### Web output — `XmlFmt`/`JsonFmt`, `WebOut`/`WebDisplay`

Unlike `BtFmt`, `XmlFmt`/`JsonFmt` dump the **whole visible tree** — paths, nav-cursor/
edit-mode state, labels and field values — sized for an HTTP client to render a full UI, not
a small GATT characteristic.

- `XmlFmt` — element-per-item (`<menu>`/`<title>`/`<body>`/`<item>`/`<lbl>`/`<fld>`...),
  nav state as attributes (`ncur="@"`, `mode="edit"`...), item body text auto-wrapped in
  `CDATA`. Each element carries a `path="/1/3/"` attribute so a client-side XSLT can
  translate/route without walking ancestors.
- `JsonFmt` — same information as one JSON object per item, nav state as properties.
- `HtmlFmt` — skips XML/XSLT entirely and streams a complete HTML page directly per render:
  selection shown via a CSS class (`NavCursor` suppressed, the class handles highlight) and
  the up/down/enter/esc nav links embedded in the page itself. Use this instead of
  `XmlFmt`+XSLT when you'd rather own the markup outright than transform on the client.

`WebOut` (Arduino/ESP32 only, `#include <oneMenu/menu/IO/arduino/webOut.h>`) streams
directly to a `WebServer` via `sendContent()` — no host-side buffer, each `put()` sends
immediately. `WebDisplay` is the ready-made `OutDef` pairing `FullPrinter`+`XmlFmt`+`WebOut`:

```cpp
WebServer webServer(80);
WebDisplay webDisplay;

webServer.on("/", []() {
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/xml", "");
  webServer.sendContent("<?xml-stylesheet type=\"text/xsl\" href=\"/menu.xsl\"?>\n");
  webNav.printTo(webDisplay);
});
webServer.on("/menu.xsl", []() {
  webServer.send(200, "text/xsl", WebOut::xsl());   // built-in stylesheet, or serve your own
});
```

`WebOut::xsl()` returns a minimal built-in XSLT stylesheet (dark monospace theme, up/down/
enter/esc links) that renders the `XmlFmt` output as HTML in the browser and auto-refreshes;
`docs/menu.xsl` in this repo is the same stylesheet kept as a standalone, more readable file
for editing — serve your own by pointing `/menu.xsl` at a customized copy instead of
`WebOut::xsl()`.

Combine with [`AsyncNav`](#asyncnav--stateless-path-based-navigation-webhttp) for a
stateless per-request nav that doesn't fight your hardware nav over the same menu.

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

### `find<Q>()` / `find(Q)` — locate an item in the menu tree

Member calls on the menu (not free functions). Search the menu's title, its own
chain, and every nested submenu's body, to any depth — so `Id<N>` works whether
it's attached to a plain body item or to a submenu itself.

```cpp
auto& item = menu.find<SameAs<Id<N>>>();   // by id, explicit predicate type
auto& item = menu.find(byId<N>);           // same thing, no `<>` needed
auto& item = menu.find<TagIs<MyTag>>();    // by tag (anything deriving from MyTag)
```

A miss is a **compile error**, not a null/empty result — if `Id<N>` isn't anywhere
in the tree, `find<>` simply won't compile for that `N`.

### Enable / disable at runtime

```cpp
auto& item = mainMenu.find<SameAs<Id<op3_id>>>();
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

### Toggle option 3 at runtime

```cpp
enum ids { op3_id };
// in menu: ItemDef<Id<op3_id>, Watch<EnDis<false>>, ...>

auto& op3 = mainMenu.find<SameAs<Id<op3_id>>>();
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
