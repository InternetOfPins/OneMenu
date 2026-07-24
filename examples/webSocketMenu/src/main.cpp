// Web menu example, hardware-verified on ESP32 (lolin32) and ESP8266
// (d1_mini). Two ways to reach the same live menu tree:
//   - /menu: an HTTP+xml-stylesheet-PI route (WebOut/WebDisplay, XmlFmt)
//     rendered client-side by data/menu.xslt — stateless, one full render
//     per request, works with no JS at all.
//   - a WebSocket channel (WebSocketOut/WsJsonDisplay, JsonFmt) that
//     data/menu.js overlays on top: field edits and live updates push to
//     every connected browser instantly, while navigation re-runs
//     menu.xslt's own transform client-side instead of a full page reload.
// Also: a plain-text Serial "checkup" route (SerialDisplay/serialNav,
// TextFmt) with PC-keyboard-over-serial input (SerialIn/PCKbd), on its own
// independent Nav instance, and an FTP server (xreef/SimpleFTPServer) onto
// the same SPIFFS data/ folder so the front-end files can be edited over
// the LAN without a full firmware reflash.
// Upload data/ via "pio run -e <env> -t uploadfs" before flashing/running.
// Fill in your own WiFi (and FTP) credentials below.
#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <FS.h>
  // ESP8266's own SDK headers (pulled in transitively by ESP8266WiFi.h)
  // #define a global nl() macro that collides with OneMenu's own nl()
  // method used throughout its output chain; undef before any OneMenu
  // header is included.
  #undef nl
  using WebServerT = ESP8266WebServer;
#else
  #include <WiFi.h>
  #include <WebServer.h>
  #include <SPIFFS.h>
  using WebServerT = WebServer;
#endif
#include <WebSocketsServer.h>
// FTP access to the same SPIFFS data/ folder WebOut serves (menu.xslt,
// style.css) — lets those be edited/uploaded directly over the LAN without
// re-flashing the whole firmware for every front-end tweak. xreef/
// SimpleFTPServer is MIT-licensed and supports both ESP32 and ESP8266
// under one API. Storage backend forced to SPIFFS via platformio.ini's own
// build_flags (its own FtpServerKey.h otherwise defaults to LittleFS on
// ESP8266, SD on ESP32 — neither matches what this project already mounts).
#include <SimpleFTPServer.h>
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/arduino/webSocketOut.h>
// webOut.h (WebOut/WebDisplay, the HTTP+xml-stylesheet-PI route menu.xslt is
// designed for) platform-switches its own WebServer type internally
// (ESP8266WebServer vs WebServer).
#include <oneMenu/menu/IO/arduino/webOut.h>
// JsonFmt: the WS push path's own wire format (native JSON.parse()
// client-side, no XSLT/DOMParser needed to read a pushed update; see
// WsJsonDisplay below).
#include <oneMenu/menu/fmt/jsonFmt.h>
// Plain-text Serial checkup route (SerialDisplay/serialNav below) — a
// bare-bones, non-web output on the SAME mainMenu tree, independent nav, no
// XML/JSON/XSLT/WS anywhere in its path.
#include <oneMenu/menu/fmt/textFmt.h>
#include <oneMenu/menu/IO/arduino/serialOut.h>
// PC-keyboard-over-serial input for the checkup route: SerialIn supplies
// raw bytes off Serial, PCKbd parses VT100 arrow keys (Up/Down/Left/Right)
// + Enter, and passes anything else through as Cmd::Key for field editing.
#include <oneMenu/menu/IO/arduino/serialIn.h>
#include <oneMenu/menu/IO/pcKbdIn.h>
#include <oneMenu/menu/cmdTable.h>
#include <oneMenu/menu/asyncNav.h>
#include <hapi/hapi.h>
#include <oneData/oneData.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

static const char* wifi_ssid = "YOUR_WIFI_SSID";
static const char* wifi_pass = "YOUR_WIFI_PASSWORD";

// Plaintext FTP (no FTPS/TLS support in this library) — fine for a private
// LAN behind NAT, not for a network exposed beyond that. Change these.
static const char* ftp_user = "YOUR_FTP_USER";
static const char* ftp_pass = "YOUR_FTP_PASSWORD";

WebServerT server(80);
WebSocketsServer webSocket(81);
FtpServer ftpSrv;

// Menu tree: plain items, a disabled-by-default item, a numeric field, and
// a submenu with TextField/NumField/Toggle/Select/Choose/date-pad.
namespace text {
  static constexpr CText op1         {"Option 1"};
  static constexpr CText op2         {"Option 2"};
  static constexpr CText op3         {"Option 3"};
  static constexpr CText settings    {"Data fields..."};
  static constexpr CText power       {"Power"};
  static constexpr CText percent     {"%"};
  static constexpr CText toggle_demo {"Toggle"};
  static constexpr CText rise        {"Rise"};
  static constexpr CText fall        {"Fall"};
  static constexpr CText both        {"Both"};
  static constexpr CText select_demo {"Select"};
  static constexpr CText choose_demo {"Choose"};
  static constexpr CText s10         {"10"};
  static constexpr CText s40         {"40"};
  static constexpr CText s60         {"60"};
  static constexpr CText s80         {"80"};
  static constexpr CText s100        {"100"};
  static constexpr CText dateSep     {"."};
}

enum ids { op3_id };

// Output capture: action fns write result text here; if non-empty when
// /menu renders, it's wrapped as <output><![CDATA[...]]></output> ahead of
// <view> inside a new <result> root (see /menu handler below), adapted to
// WebOut's buffer-then-flush model instead of a live per-character stream.
// Single global buffer is safe: WebServerT handles one request at a time.
namespace webResult {
  static String buf;
  static void print(const char* s) { buf += s; }
  static void clear() { buf = ""; }
}

namespace action {
  bool op1(Sz) { webResult::print("Option 1 executed."); return true; }
  bool op2(Sz);  // toggles op3's enabled state, defined after mainMenu (needs find<>)
  bool op3(Sz) { return true; }
  bool subIdx(Sz) { return false; }
}

using Power = NumFieldDef<
  AsLabel<StaticText<&text::power>>,
  NumField<StaticNumRange<StaticRange<0,100,false>>, AsField<Watch<Default<Int,55>>>>,
  AsUnit<StaticText<&text::percent>>
>;

// AsEditMode<> listed FIRST throughout below — attribute-only Fmt tags must
// fire while the item's own XML tag is still open (see fields.h/xmlFmt.h).
using ToggleDemo = ToggleFieldDef<
  ItemDef<AsEditMode<>, StaticText<&text::toggle_demo>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::rise>>>,
    ItemDef<AsField<StaticText<&text::fall>>>,
    ItemDef<AsField<StaticText<&text::both>>>
  >,
  BodyAction<action::subIdx>
>;

using SelectDemo = SelectFieldDef<
  ItemDef<AsEditMode<>, AsLabel<StaticText<&text::select_demo>>, BodyAction<action::subIdx>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::s10>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s40>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s60>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s80>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s100>>, AsUnit<StaticText<&text::percent>>>
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

using ChooseDemo = ChooseFieldDef<
  ItemDef<AsEditMode<>, StaticText<&text::choose_demo>, BodyAction<action::subIdx>>,
  StaticBody<
    ItemDef<AsField<StaticText<&text::s10>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s40>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s60>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s80>>,  AsUnit<StaticText<&text::percent>>>,
    ItemDef<AsField<StaticText<&text::s100>>, AsUnit<StaticText<&text::percent>>>
  >,
  WrapNav,
  BodyAction<action::subIdx>
>;

auto dateField(const char* lbl) {
  return padDef(
    ItemDef<AsEditMode<>, AsLabel<Text>>{lbl},
    staticBody(
      ItemDef<
        EditField, ParentDraw,
        NumField<StaticNumRange<StaticRange<1900,2150,true>>, AsField<Watch<Default<Int,2026>>>>
      >{2026},
      ItemDef<
        AsEditMode<>, StaticText<&text::dateSep>, EditField, ParentDraw,
        NumField<StaticNumRange<StaticRange<1,12,true>>, AsField<Watch<Int>>>
      >{1},
      ItemDef<
        AsEditMode<>, StaticText<&text::dateSep>, EditField, ParentDraw,
        NumField<StaticNumRange<StaticRange<1,31,true>>, AsField<Watch<Int>>>
      >{1}
    )
  );
}

auto mainMenu = menuDef<WrapNav>(
  ItemDef<Text>{"OneMenu demo"},
  staticBody(
    ItemDef<Action<action::op1>, StaticText<&text::op1>>{},
    ItemDef<Action<action::op2>, StaticText<&text::op2>>{},
    ItemDef<Id<ids::op3_id>, Action<action::op3>, Watch<EnDis<false>>, StaticText<&text::op3>>{},
    menuDef<WrapNav>(
      ItemDef<StaticText<&text::settings>>{},
      staticBody(
        ItemDef<AsEditMode<>, AsLabel<Text>, EditField, ParentDraw, AsField<TextField<15>>>{"Name"},
        Power{55},
        ToggleDemo{},
        SelectDemo{},
        ChooseDemo{},
        dateField("Date")
      )
    )
    // no static "<Back" item anywhere in the tree — the web view's own
    // computed "‹ Back" link (menu.xslt, derived from menu/@at) supersedes
    // it; a real physical-device nav would still want a Back/Esc affordance
    // here, but this demo firmware is web-only.
  )
);

bool action::op2(Sz) {
  auto& op3 = mainMenu.find<SameAs<Id<ids::op3_id>>>();
  op3.enable(!op3.enabled());
  return true;
}

// Web nav — AsyncNav for stateless path jumps driven by translated commands.
NavDef<AsyncNav, TreeNav, Root<decltype(mainMenu), mainMenu>> webNav;

// Local JSON variant of webSocketOut.h's own WebSocketDisplay (which uses
// XmlFmt) — kept example-scoped rather than editing that shared header, so
// the plain-HTTP path (WebDisplay/XmlFmt/menu.xslt) stays untouched.
using WsJsonDisplay = OutDef<
  FullPrinter,
  JsonFmt,
  DataParser<>,
  Cursor<1, 1>,
  WebSocketOut,
  StaticPos<0, 0>,
  StaticArea<80, 25>
>;

WsJsonDisplay wsDisplay;
WebDisplay webDisplay;

// Plain-text Serial checkup route: TextFmt (one line per item, '>'/'-'/' '
// cursor markers, no color/ANSI) + FullPrinter, no Gate/ColorTrack/CtrlChars.
// Cursor<1,1> is still required — ItemPrinter's own static_assert demands
// IsCursor be present in the chain — even though SerialOut's own setPos()
// is a no-op for this streaming device.
using SerialDisplay = OutDef<
  FullPrinter,
  TextFmt,
  DataParser<>,
  Cursor<1, 1>,
  SerialOut,
  StaticPos<0, 0>,
  StaticArea<80, 25>
>;

SerialDisplay serialDisplay;
// Independent nav — its own cursor/level state, NOT shared with webNav —
// so this checkup route reflects the tree exactly as a plain, non-web
// OneMenu consumer would see it. Same mainMenu tree (shared field VALUES —
// Watch<>/m_sel/etc. live on the items themselves, not per-nav), so edits
// made over the web ARE visible here too; only the Nav's OWN cursor/level
// is independent.
NavDef<TreeNav, Root<decltype(mainMenu), mainMenu>> serialNav;

// PC keyboard over Serial: send arrow keys via a VT100-capable terminal
// (or a serial monitor that forwards raw escape sequences — plain
// Arduino IDE Serial Monitor does NOT, use a real terminal like
// `pio device monitor` in raw mode, minicom, or screen) for Up/Down/
// Enter; anything else typed goes to the currently-focused field as
// Cmd::Key (digits/letters for text/number editing).
InDef<SerialIn, PCKbd> serialIn;

// shorthand command table for client-issued jump commands
static const CmdEntry cmds[] = {
  {"#SETTINGS", "/3/"},  // jump straight to the Data fields submenu
};

void pushRender() {
  wsDisplay.lockMode(LockMode::None);
  webNav.printTo(wsDisplay);
  webNav.sync(wsDisplay);
}

// Strip the trailing "<idx>/" segment from an absolute path, e.g.
// "/3/2/" -> "/3/", "/3/" -> "/", "/" -> "/". Used to derive the plain-HTTP
// /set handler's own redirect target purely from the field's own path — no
// server-side session/cursor state involved (see /menu's own header note:
// every plain-HTTP request must be self-contained, deriving its response
// entirely from its own explicit ?at=/path=, never from wherever webNav
// happened to be left by some earlier, unrelated request).
void parentPath(const char* path, char* out, size_t outSz) {
  size_t len = strlen(path);
  if(len>1 && path[len-1]=='/') len--;    // trim trailing slash
  while(len>0 && path[len-1]!='/') len--; // walk back to the previous slash
  if(len==0) len=1;                       // keep at least "/"
  if(len>=outSz) len=outSz-1;
  memcpy(out,path,len);
  out[len]='\0';
}

// The index of the field just edited, as seen from the page it redirects
// back to ("at") — the first path segment right after at's own prefix.
// For a direct field (path="/3/1/", at="/3/") that's the field's own index
// (1, Power). For a pad-nested sub-field (path="/3/5/0/", at="/3/", since
// entering the pad item itself isn't safe — see /set's own note above)
// it's the pad item's OWN index (5, Date) — the deepest row "at" can
// actually highlight without descending into the pad. Passed to /menu as
// ?sel=, so the redirected-to page can move webNav's own selection there
// (Menu::go(), a pure index write — no Enter/padOpen side effects) before
// rendering, giving the just-edited row the same @ncur='@'/li.cur highlight
// (--accent-soft) a real nav cursor would — otherwise the shown "current"
// row was just whatever webNav's shared, cross-request state happened to
// still hold at that depth, unrelated to what the user just changed.
int fieldSelIndex(const char* path, const char* at) {
  size_t atLen = strlen(at);
  if(strncmp(path,at,atLen)!=0) return 0;
  const char* p = path+atLen;
  if(*p<'0'||*p>'9') return 0;
  int v=0;
  while(*p>='0'&&*p<='9') v=v*10+(*p++-'0');
  return v;
}

// Wire format for client->server commands: plain path/shorthand strings
// (same convention translateCmd/async() already use, unchanged) plus one
// minimal "S|<path>|<val>" prefix for field edits (menu.js's own
// replacement for the HTTP fallback's /set request) — deliberately NOT
// JSON: parsing arbitrary JSON on-device for a two-field command would
// need a real JSON library for no real benefit here, unlike the JSON
// server->client render payload (JsonFmt), which is pure serialization,
// no parsing.
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if(type == WStype_CONNECTED) { pushRender(); return; }
  if(type != WStype_TEXT) return;
  const char* msg = (const char*)payload;
  if(msg[0] == 'S' && msg[1] == '|') {
    char buf[64];
    strncpy(buf, msg + 2, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* bar = strchr(buf, '|');
    if(bar) {
      *bar = '\0';
      // webNav.changed(wsDisplay) below only probes whatever level webNav's
      // OWN shared cursor currently sits at — setAt() itself is path-driven
      // and works from anywhere, but a field edited OUTSIDE that level
      // would apply silently with no push at all. Navigate there first,
      // same async()+enter() pair /menu's own HTTP handler uses, so the
      // change lands somewhere the very next changed(out) probe (and
      // pushRender()'s own printTo()) actually looks.
      char parent[32];
      parentPath(buf, parent, sizeof(parent));
      webNav.async(parent);
      // enter() resets the submenu's own selection to its first child —
      // reposition to the field ACTUALLY being edited before rendering, or
      // every WS-driven set shows item 0 focused regardless of which field
      // the user touched.
      if(strcmp(parent, "/") != 0) webNav.enter();
      webNav.go(fieldSelIndex(buf, parent));
      webNav.setAt(buf, bar + 1);
      // Always push after a SET, skipping the changed(wsDisplay) gate below
      // entirely — RecallNavPos (Toggle/Select's own m_sel, item.h) has no
      // changed()/sync() override at all, so a pure selection edit is
      // invisible to that deep probe (NumField's own Watch<> DOES
      // register, which is why Power alone would work without this). The
      // gate is only an optimization against redundant broadcasts anyway —
      // one extra push per user edit costs nothing a human will notice.
      pushRender();
      return;
    }
  } else {
    char path[32];
    const char* target = translateCmd(msg, cmds, path, sizeof(path)) ? path : msg;
    webNav.async(target);
    // Same as /menu's own HTTP handler: async() alone only selects the
    // leaf (its own documented contract — "caller issues Enter"), it
    // doesn't expand/descend into it. Without this, a plain item click
    // over WS would move the cursor but never render the target submenu's
    // body.
    if(strcmp(target, "/") != 0) webNav.enter();
  }
  // changed(wsDisplay) gates whether to push at all (e.g. a SET with the
  // same value, or a no-op nav) — sync() (inside pushRender()) only needs
  // to run right after an actual push, so skipping both together here is
  // consistent: changed()==false already means nothing was dirty to clear.
  if(webNav.changed(wsDisplay)) pushRender();
}

static const char indexHtml[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<title>OneMenu WebSocketOut test</title></head><body style="font-family:monospace;background:#111;color:#0f0">
<p>See <a href="/menu" style="color:#0f0">/menu</a> for the real XSLT-rendered view.</p>
<pre id="out">connecting...</pre>
<div>
<a href="#" onclick="send('/3/')">Data fields</a> |
<a href="#" onclick="send('#SETTINGS')">#SETTINGS</a>
</div>
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage=function(e){document.getElementById('out').textContent=e.data;};
ws.onopen=function(){document.getElementById('out').textContent='connected';};
function send(s){ws.send(s);}
</script></body></html>)HTML";

void setup() {
  Serial.begin(115200);
#ifdef ESP8266
  SPIFFS.begin();
#else
  SPIFFS.begin(true);
#endif
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  while(WiFi.status() != WL_CONNECTED) { delay(300); Serial.print('.'); }
  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());

  WebSocketOut::begin(webSocket);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  WebOut::begin(server);

  // FTP onto the same SPIFFS this server already mounted above — lets
  // data/menu.xslt, style.css etc. be updated over the LAN (any FTP client,
  // e.g. lftp/FileZilla) without a full firmware reflash.
  ftpSrv.begin(ftp_user, ftp_pass);

  server.on("/", [](){
    server.send_P(200, "text/html", indexHtml);
  });

  // Real HTTP+xml-stylesheet-PI route: the browser fetches this XML
  // directly and applies data/menu.xslt client-side — this is the route
  // the adapted menu.xslt is built for, distinct from the WebSocket push
  // demo above.
  //
  // STATELESS by design: every request supplies its own ?at=<absolute path>
  // (default "/", the root menu) and the response is derived FRESH from
  // THAT alone, every single time. webNav's own cursor position never
  // carries meaning ACROSS separate requests — it's one server-wide object,
  // and "wherever the last request happened to leave it" breaks the moment
  // two browser tabs (or two different devices) talk to the same server, or
  // a client simply reloads the page. Every link menu.xslt generates embeds
  // its own target ?at=, so a client can always get back to any level
  // directly — there is no implicit "current page".
  server.on("/menu", [](){
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/xml", "");
    server.sendContent(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<?xml-stylesheet type=\"text/xsl\" href=\"/menu.xslt\"?>\n"
    );
    String at = server.hasArg("at") ? server.arg("at") : String("/");
    webResult::clear();
    webNav.async(at.c_str());
    // async() only selects the leaf (its own documented contract — "caller
    // issues Enter"); "/" alone means "just show the root", nothing to
    // descend into, so skip enter() there specifically — anything deeper
    // names a real target level to render.
    if(at != "/") webNav.enter();
    // Optional ?sel= (set by /set's own redirect, see fieldSelIndex() above):
    // overrides whatever selection index webNav's shared, cross-request
    // state happened to still hold at this depth, so the field just edited
    // shows as the current/highlighted row instead of an unrelated leftover
    // one. go() is a pure index write (TreeNav::Part::go) — no Enter/
    // padOpen side effects, safe to call unconditionally here.
    if(server.hasArg("sel")) webNav.go(server.arg("sel").toInt());
    // <view> (below) is one atomic render — Fmt::View fully brackets
    // Fmt::Menu inside XmlFmt's own walk, so there's no seam to inject an
    // <output> sibling between them from out here. Simplest fix: wrap the
    // whole response in a new <result> root instead, holding the captured
    // text (if any) ahead of the unchanged <view> — menu.xslt's own
    // result/output template renders it as a box below the menu panel.
    server.sendContent("<result>\n");
    if(webResult::buf.length()) {
      server.sendContent("<output><![CDATA[");
      server.sendContent(webResult::buf);
      server.sendContent("]]></output>\n");
    }
    webDisplay.lockMode(LockMode::None);
    webNav.printTo(webDisplay);
    // This HTTP route is ALSO how an action item's side effect reaches the
    // server (e.g. op2 toggling op3's own enabled state) — menu.js's own
    // nav links deliberately don't go over WS at all (shared-cursor risk,
    // see this handler's own header comment), so without this, another
    // connected tab would never learn an action just changed something.
    // Safe regardless of which level THIS request targets: client-side
    // patching is keyed by absolute path (menu.js's own patchItem), so a
    // tab showing an unrelated level just finds no matching element and
    // silently ignores it.
    pushRender();
    server.sendContent("</result>");
    // CONTENT_LENGTH_UNKNOWN makes WebServer use chunked transfer encoding
    // (WebServer.cpp: _chunked=true whenever content length is unknown on
    // HTTP/1.1) — a bare client().stop() here abruptly drops the connection
    // WITHOUT the terminating "0\r\n\r\n" chunk the chunked framing requires,
    // so the browser/curl has to wait out its own socket-read timeout
    // (~5s) before giving up and rendering whatever it got. sendContent("")
    // sends that proper terminator (WebServer::sendContent: contentLength==0
    // -> _chunked=false).
    server.sendContent("");
  });

  // Apply a value at path, then redirect back to the page that generated
  // this link — the caller's own ?at= (menu.xslt's own /view/menu/@at,
  // baked into every /set link it generates), not a value derived from
  // `path` itself: a pad sub-field (dateField's own year/month/day) has a
  // path TWO levels deeper than its real enclosing menu (e.g. "/3/5/0/"
  // for year, when "Data fields..." itself is "/3/"), so computing the
  // redirect via parentPath(path) alone would land on dateField's own
  // synthetic path, not a real navigable level, and requesting THAT via
  // /menu?at=... fires a real Enter on dateField (n.padOpen()), leaving
  // the nav's own edit-mode/pad state lingering and breaking subsequent
  // edits to OTHER, unrelated fields. webNav.setAt() (asyncNav.h) sidesteps
  // the other half of the same problem: it applies the value via a plain
  // path-driven setStr() walk, with NO go()/Enter/padOpen() navigation at
  // all. parentPath(path) is kept as a fallback for any caller that
  // doesn't supply ?at=, failing toward the field's own immediate parent.
  server.on("/set", [](){
    String path = server.arg("path");
    webNav.setAt(path.c_str(), server.arg("val").c_str());
    String at;
    if(server.hasArg("at")) at = server.arg("at");
    else { char parent[32]; parentPath(path.c_str(),parent,sizeof(parent)); at = parent; }
    int sel = fieldSelIndex(path.c_str(), at.c_str());
    // Broadcast to every connected WS client too — this HTTP path (Choose's
    // own in-submenu option buttons, and the plain fallback for any field
    // when WS is down) otherwise never touches the WS side at all. Then
    // reposition to the edited field (go(sel)) before pushRender() — the
    // redirect's own ?sel= only helps the ORIGINATING browser, which follows
    // it as a real second request; other WS-connected tabs only ever see
    // whatever's broadcast right here. RecallNavPos (Choose/Toggle/Select,
    // item.h) has no changed()/sync() override at all, so pushRender() is
    // unconditional rather than gated on changed(wsDisplay).
    webNav.async(at.c_str());
    if(at != "/") webNav.enter();
    webNav.go(sel);
    pushRender();
    server.sendHeader("Location", String("/menu?at=")+at+"&sel="+String(sel));
    server.send(302, "text/plain", "");
  });

  // menu.xslt itself, served straight from flash (SPIFFS) — a real static
  // front-end asset.
  server.serveStatic("/menu.xslt", SPIFFS, "/menu.xslt");
  // Styling lives in its own real static asset too (not embedded in
  // menu.xslt's own <head>) — same serveStatic pattern, lets the browser
  // cache CSS independently of the XML payload. Split into a shared
  // common file plus two theme complements (menu.xslt links all three,
  // the two complements via media="(prefers-color-scheme: ...)" so the
  // browser picks the right one from the system's own light/dark setting
  // — no JS, no server-side detection needed).
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/style-dark.css", SPIFFS, "/style-dark.css");
  server.serveStatic("/style-light.css", SPIFFS, "/style-light.css");
  // WS overlay's own client script (menu.js) — same static-asset pattern,
  // FTP-editable via the FTP server above, no reflash needed to iterate on it.
  server.serveStatic("/menu.js", SPIFFS, "/menu.js");

  server.begin();
}

// Unconditional timer, NOT gated on changed() — Watch<>'s own dirty bit
// (oneData.h) is a single shared snapshot per field, not tracked per
// consumer: webNav's own sync(wsDisplay) call (pushRender(), above) already
// clears it after every web-driven push, so a SEPARATE nav's changed()
// check here would almost always see "nothing changed" even when the field
// genuinely did, moments earlier, from webNav's own perspective. A plain
// periodic dump sidesteps that shared-state race entirely — also doubles
// as a "the checkup route itself is alive" heartbeat.
static unsigned long lastSerialCheckup = 0;
void loop() {
  server.handleClient();
  webSocket.loop();
  ftpSrv.handleFTP();

  // Keyboard input drives its own immediate redraw — doInput() returns
  // whether it actually processed anything this call (a keypress moved the
  // cursor, entered/left edit mode, or typed a character into a field),
  // so typing feels live rather than waiting on the periodic timer below.
  if(serialIn.doInput(serialNav)) {
    serialDisplay.lockMode(LockMode::None);
    serialDisplay.clear();
    serialNav.printTo(serialDisplay);
    lastSerialCheckup = millis(); // don't ALSO redraw again a moment later
  }

  if(millis() - lastSerialCheckup > 5000) {
    lastSerialCheckup = millis();
    serialDisplay.lockMode(LockMode::None);
    serialDisplay.clear();
    serialNav.printTo(serialDisplay);
  }
}
