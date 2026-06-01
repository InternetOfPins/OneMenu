// includes --
  #include <oneMenu/oneMenu.h>
  #include <oneMenu/menu/IO/ansiOut.h>
  #include <oneMenu/menu/fmt/textFmt.h>
  #include <oneMenu/menu/fmt/ansiFmt.h>
  #include <oneMenu/menu/fmt/xmlFmt.h>
  #include <oneMenu/menu/IO/pcKbdIn.h>
  #include <oneMenu/menu/body/cArrayBody.h>
  #include <oneMenu/menu/body/stdBody.h>

  #ifdef __AVR__
    #include <menu/IO/arduino/serialOut.h>
    #include <menu/IO/arduino/serialIn.h>
    #include <streamFlow.h>
    using namespace StreamFlow;
    #define cout Serial
  #else
    #include <vector>
    #include <oneMenu/menu/IO/streamOut.h>
    #include <oneMenu/menu/IO/linuxKeyIn.h>
  #endif
  using namespace std;

bool running=true;

using namespace hapi;
using namespace oneMenu;
using namespace oneData;

InDef<
  #ifdef ARDUINO
    SerialIn,
  #else
    LinuxKeyIn,
  #endif
  PCKbd
> in;

IOutDef<
  ScrollPrinter,//menu parts to use
  // FullPrinter,
  ANSIFmt,//add some ANSI colors and format to the output
  // TextFmt,
  ClearFreeFmt,//this can take a lot of burden away from user format
  DataParser<>,//put all data into characters
  CtrlChars,
  UTF8,//bypass UTF8 surrogate codes
  TextWrap,//long texts continue next line
  Clip,//keep content inside area
  ColorTrack<int>,//track color setting for device resume...
  Cursor,//track cursor position for resume...
  Gate,
  ANSIOut,//inject ansi codes into the next output device
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<20,10>,
  StaticArea<30,8>
> out;

OutDef<
  DataParser<>,//put all data into characters
  CtrlChars,
  UTF8,//bypass UTF8 surrogate codes
  TextWrap,//long texts continue next line
  Clip,//keep content inside area
  ColorTrack<int>,//track color setting for device resume...
  Cursor,//track cursor position for resume...
  Gate,
  ANSIOut,//inject ansi codes into the next output device
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<decltype(out)::orgX(),decltype(out)::orgY()+decltype(out)::height()>,
  StaticArea<decltype(out)::width(),4>
> footer;

OutDef<
  TextFmt,
  DataParser<>,//put all data into characters
  CtrlChars,
  // UTF8,//bypass UTF8 surrogate codes
  TextWrap,//long texts continue next line
  Clip,//keep content inside area
  Buffer<>,
  ColorTrack<int>,
  Cursor,//track and report cursor movement
  Gate,//locks output for measuring and other operations
  ANSIOut,//inject ansi codes into the next output device
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<5,decltype(footer)::orgY()+decltype(footer)::height()+1>,
  StaticArea<80,10>
> syslog;

IOutDef<
  FullPrinter,
  XmlFmt,
  DataParser<>,//put all data into characters
  // CtrlChars,
  // UTF8,//bypass UTF8 surrogate codes
  // TextWrap,//long texts continue next line
  // Clip,//keep content inside area
  Buffer<>,
  ColorTrack<int>,
  Cursor,//track and report cursor movement
  Gate,//locks output for measuring and other operations
  ANSIOut,//inject ansi codes into the next output device
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<0,decltype(syslog)::orgY()+decltype(syslog)::height()+1>,
  StaticArea<100,20>
> web;

namespace text {
  static constexpr const CText main_menu{"Main menu"};
  static constexpr const CText back{"<Back"};
  static constexpr const CText quit{"Exit!"};
  static constexpr const CText op1{"Option 1"};
  static constexpr const CText op2{"Option 2"};
  static constexpr const CText op3{"Option 3"};
  static constexpr const CText fields_menu{"Fields..."};
  static constexpr const CText array_sub_menu{"Same type array>"};
  static constexpr const CText sub_ibody{"IItem* array"};
  static constexpr const CText sub_sbody{"std::container"};
  static constexpr const CText sub1{"10"};
  static constexpr const CText sub2{"40"};
  static constexpr const CText sub3{"60"};
  static constexpr const CText sub4{"80"};
  static constexpr const CText sub5{"100"};
  static constexpr const CText power{"Power"};
  static constexpr const CText percent{"%"};
  static constexpr const CText off{"Off"};
  static constexpr const CText on{"On"};
  static constexpr const CText yes{"Yes"};
  static constexpr const CText no{"No"};
  static constexpr const CText toggle_demo{"Toggle"};
  static constexpr const CText select_demo{"Select"};
  static constexpr const CText choose_demo{"Choose"};
  static constexpr const CText dateSep{"."};
};

namespace desc {
  static constexpr const CText back{"return to previous menu."};
  static constexpr const CText quit{"terminate the program."};
  static constexpr const CText dot{"."};
  static constexpr const CText op1{"simple option with action function.\n`Enter` to activate function."};
  static constexpr const CText op2{"toggles Option 3\nenabled/disabled state."};
  static constexpr const CText op3{"simple option\ncan be enabled/disabled at runtime."};
  static constexpr const CText fields_menu{"some field examples..."};
  static constexpr const CText array_sub_menu{"pure C-Array as menu body, no virtual functions, all items the same"};
  static constexpr const CText sub_ibody{"pure C-Array with virtual derived items"};
  static constexpr const CText sub_sbody{"std::container with virtual items"};
};

enum ids {op3,power,container,date_fld};

namespace action {
  bool op1(Sz) {
    syslog.resume();
    syslog<<"option #1 action called."<<endl;
    out.resume();
    return true;
  }
  bool op2(Sz);
  bool op3(Sz) {
    syslog.resume();
    syslog<<"option #3 action called."<<endl;
    out.resume();
    return true;
  }
  bool quit(Sz) {
    syslog.setColors(RED,BLACK);
    syslog.erase();
    syslog<<"Bye!"<<endl;
    running=false;
    return true;
  }
  bool subIdx(Sz i) {
    syslog.resume();
    syslog<<"sub option #"<<i<<" selected."<<endl;
    out.resume();
    return false;
  }
}

template<typename... OO> using Desc=OnFocus<typename Put<OO...>::template ToOut<decltype(footer),footer,Clear::yes>>;

using Back=ItemDef<StaticText<text::back>,Desc<StaticText<desc::back>>>;
using Quit=ItemDef<Action<action::quit>,AsLabel<StaticText<text::quit>,Desc<StaticText<desc::quit>>>>;

using CItem=ItemDef<Text>;

CItem cBody[]{
  "..",
  ".pio/",
  "src/",
  "all of the same type.txt",
  "platformio.ini",
  "README.md"
};

bool stay(int i) {
  syslog.resume();
  syslog<<"stay function called. The menu remains, not closing as default."<<endl;
  out.resume();
  return true;
}

IItem* iBody[]{
  new IItemDef<StaticText<text::op1>>{},
  new IItemDef<StaticText<text::op2>>{},
  new IItemDef<StaticText<text::op3>>{},
  new IItemDef<Text>{"what else..."},
  new IItemDef<Text,Desc<Text>,Action<stay>>{"Stay","select and stay!"},
  new IItemDef<StaticText<text::back>>{}
  // new Back::As<IItemDef>{}
};

using ChooseDemo=ChooseFieldDef<
  ItemDef<
    StaticText<text::choose_demo>,
    AsEditMode<>,//edit mode indicator
    BodyAction<action::subIdx>
  >,
  StaticBody<//sub menu static body
    ItemDef<AsField<StaticText<text::sub1>,AsUnit<StaticText<text::percent>>>>,
    ItemDef<AsField<StaticText<text::sub2>,AsUnit<StaticText<text::percent>>>>,
    ItemDef<AsField<StaticText<text::sub3>,AsUnit<StaticText<text::percent>>>>,
    ItemDef<AsField<StaticText<text::sub4>,AsUnit<StaticText<text::percent>>>>,
    ItemDef<AsField<StaticText<text::sub5>,AsUnit<StaticText<text::percent>>>>
    //,Back <--//TODO: extend the enter nav to this level and respect the handling (same as esc)
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

using SelectDemo=SelectFieldDef<
  ItemDef<
    AsLabel<StaticText<text::select_demo>>,
    AsEditMode<>,//edit mode indicator
    BodyAction<action::subIdx>
  >,
  StaticBody<//sub menu static body
    ItemDef<AsField<StaticText<text::sub1>>,AsUnit<StaticText<text::percent>>>,
    ItemDef<AsField<StaticText<text::sub2>>,AsUnit<StaticText<text::percent>>>,
    ItemDef<AsField<StaticText<text::sub3>>,AsUnit<StaticText<text::percent>>>,
    ItemDef<AsField<StaticText<text::sub4>>,AsUnit<StaticText<text::percent>>>,
    ItemDef<AsField<StaticText<text::sub5>>,AsUnit<StaticText<text::percent>>>
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

//a Map operation, this operation will fit Map<> and will insert OO... components into the chain
template<typename... OO> struct Ins {template<typename M>  using Map=typename M::template Ins<OO...>;};
template<typename... OO> struct App {template<typename M>  using Map=typename M::template App<OO...>;};

using ToggleDemo=ToggleFieldDef<
  ItemDef<Text,AsEditMode<>>,
  StaticBody<
    ItemDef<AsField<StaticText<text::no>>>,
    ItemDef<AsField<StaticText<text::yes>>>,
    ItemDef<AsField<Text>>
  >//,BodyAction<action::subIdx>
>;

using Power=NumFieldDef<
  Chain<
    Id<ids::power>,
    AsLabel<StaticText<text::power>>//field label
  >,
  NumField<//use range to change the data
    StaticNumRange<int,0,100,false>,//valid range
    ItemNav,
    Watch<AsField<Default<int,55>,Int>>//use `int` and `change watch` as field (data)
  >,
  AsUnit<StaticText<text::percent>>//field unit
>;

  // date field generating function
auto dateField(const char*lbl)
#ifdef __AVR__
  ->decltype(
  padDef(
    ItemDef<AsLabel<Text>,AsEditMode<>>{lbl},
    staticBody(
      ItemDef<//lets define a numeric field:
        EditField,//use nav keys up/down to change numeric value within range
        ParentDraw,//draw inplace
        // EnDis<false>,
        // AsEditMode<>,//edit mode indicator (format)
        ItemNav,//open nav level for this item on Cmd::Enter
        NumField<StaticNumRange<int,1900,2150,true>,//static numeric range
        Watch<AsField<Default<int,2026>,Int>>>//watch for changes, format an Int (Data<int>) as field with default value 2026
      >{2026},
      ItemDef<
        StaticText<text::dateSep>,
        EditField,
        ParentDraw,
        AsEditMode<>,
        ItemNav,
        NumField<StaticNumRange<int,1,12,true>,
        Watch<AsField<Int>>>
      >{1},
      ItemDef<
        StaticText<text::dateSep>,
        EditField,
        ParentDraw,
        AsEditMode<>,
        ItemNav,
        NumField<StaticNumRange<int,1,31,true>,
        Watch<AsField<Int>>>
      >{1}
    )
  )
)
#endif
{
  return padDef(
    ItemDef<AsLabel<Text>,AsEditMode<>>{lbl},
    staticBody(
      ItemDef<//lets define a numeric field:
        EditField,//use nav keys up/down to change numeric value within range
        ParentDraw,//draw inplace
        // EnDis<false>,
        // AsEditMode<>,//edit mode indicator (format)
        ItemNav,//open nav level for this item on Cmd::Enter
        NumField<
          StaticNumRange<int,1900,2150,true>,//static numeric range
          Watch<
            AsField<
              Default<int,2026>,
              Int
            >
          >
        >
      >{2026},
      ItemDef<
        StaticText<text::dateSep>,
        EditField,
        ParentDraw,
        AsEditMode<>,
        ItemNav,
        NumField<StaticNumRange<int,1,12,true>,
        Watch<AsField<Int>>>
      >{1},
      ItemDef<
        StaticText<text::dateSep>,
        EditField,
        ParentDraw,
        AsEditMode<>,
        ItemNav,
        NumField<StaticNumRange<int,1,31,true>,
        Watch<AsField<Int>>>
      >{1}
    )
  );
}

auto mainMenu=menuDef<WrapNav>(
  ItemDef<Text>{"Main menu"},
  staticBody(
    ItemDef<Action<action::op1>,StaticText<text::op1>,Desc<StaticText<desc::op1>>>{},
    ItemDef<Action<action::op2>,StaticText<text::op2>,Desc<StaticText<desc::op2>>>{},
    ItemDef<Id<ids::op3>,Action<action::op3>,Watch<EnDis<false>>,StaticText<text::op3>,Desc<StaticText<desc::op3>>>{},
    menuDef<WrapNav>(
      ItemDef<StaticText<text::fields_menu>,Desc<StaticText<desc::fields_menu>>>{},
      staticBody(
        ItemDef<
          AsLabel<Text>,
          AsEditMode<>,
          EditField,
          ParentDraw,
          // ItemNav,
          AsField<TextField<15>>
        >{"Name"},
        Power{55},
        // ToggleDemo{"Toggle","Maybe"},
        SelectDemo{},
        ChooseDemo{},
        dateField("date"),
        Back{}
      )
    ),
    MenuDef<//sub menu with C array body (all items of the same type)
      ItemDef<
        BodyAction<action::subIdx>,
        StaticText<text::array_sub_menu>,
        Desc<StaticText<desc::array_sub_menu>>
      >,
      CArrayBody<CItem,cBody,sizeof cBody/sizeof *cBody>
    >{},
    #ifndef __AVR__
      MenuDef<//sub menu with C array body of virtual `IItem` (not all of the same type)
        ItemDef<BodyAction<action::subIdx>,StaticText<text::sub_ibody>,Desc<StaticText<desc::sub_ibody>>>,
        CPtrArrayBody<IItem,iBody,sizeof(iBody)/sizeof(iBody[0])>,
        WrapNav
      >{},
      MenuDef<
        ItemDef<BodyAction<action::subIdx>,StaticText<text::sub_sbody>,Desc<StaticText<desc::sub_sbody>>>,
        StdBody<vector<IItem*>>,
        WrapNav,Id<ids::container>
      >{},
    #endif
    Quit{}
  )
);

INavDef<
  TreeNav,
  Root<decltype(mainMenu),mainMenu>
> nav;

bool action::op2(Sz) {
  syslog<<"option #2 action called.\ntoggle option #3 enable/disable state"<<endl;
  // mainMenu.template withId<Id<ids::op3>>().enable(!mainMenu.template withId<Id<ids::op3>>().enabled());
  return true;
}

//================================================================================================--
bool run() {
  static TinyTimeUtils::FPS<60> fps;
  if(fps) {
    fps.reset();
    nav.in(in);
    if(nav.changed(out)) {
      web.resume();
      // web.lockMode(LockMode::None);
      // web.setColors(RED,WHITE);
      web.clear();
      nav.printTo(web);
      out.resume();
      nav.printTo(out);
      nav.sync(out);
    }
  }
  return running;
}

void setup() {
  cout<<"AM5 R&D"<<endl;

  #ifndef __AVR__
    //populate std container menu
    // mainMenu.template withId<Id<ids::container>>().body().push_back(new IItemDef<Text>{"runtime"});
    // mainMenu.template withId<Id<ids::container>>().body().push_back(new IItemDef<Text>{"populated"});
    // mainMenu.template withId<Id<ids::container>>().body().push_back(new IItemDef<Text>{"items"});
  #endif

  web.lockMode(LockMode::None);
  web.setColors(RED,WHITE);
  web.clear();
  nav.printTo(web);

  syslog.lockMode(LockMode::None);
  syslog.setColors(GREEN,BLACK);
  syslog.clear();
  syslog.put(".·•<::(log)::>•·.");

  footer.lockMode(LockMode::None);
  footer.setColors(BLUE,BLACK);
  footer.clear();
  footer.put("footer");

  out.lockMode(LockMode::None);
  out.setColors(WHITE,BLACK);
  out.clear();
  nav.printTo(out);
}

int main(){
  // mainMenu.template withId<Id<ids::op3>>().print(out,{{0}});
  setup();
  while(run());
  // dout<<xy<0,50><<"end."<<endl;
  return 0;
}