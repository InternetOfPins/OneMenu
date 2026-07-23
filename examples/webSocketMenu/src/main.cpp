// Web menu example, hardware-verified on ESP32 (lolin32) and ESP8266
// (d1_mini): oneMenu::WebSocketOut/WebSocketDisplay (push-based menu output
// over a real WebSocketsServer) + oneMenu::translateCmd (shorthand-command
// dispatch) + WebServer::serveStatic (static front-end assets from SPIFFS,
// ESP32 only — see webOut.h note below). The real menu view is /menu, an
// HTTP+xml-stylesheet-PI route rendered client-side by data/menu.xslt
// (upload the data/ folder via "pio run -t uploadfs"); "/" is a secondary,
// minimal WebSocket-push demo. Fill in your own WiFi credentials below.
#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <FS.h>
  // ESP8266's own SDK (lwip2/gluedebug.h, pulled in transitively by
  // ESP8266WiFi.h) #defines a global `nl()` macro — collides with OneMenu's
  // own nl() method used throughout its output chain. Same bug class as the
  // known ansiCodes.h global-namespace collision (see feedback_ansi_codes_
  // global_namespace memory); undefine it before any OneMenu header is
  // included, same disclosed-per-port workaround, not a library fix.
  #undef nl
  using WebServerT = ESP8266WebServer;
#else
  #include <WiFi.h>
  #include <WebServer.h>
  #include <SPIFFS.h>
  using WebServerT = WebServer;
#endif
#include <WebSocketsServer.h>
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/IO/arduino/webSocketOut.h>
#ifndef ESP8266
  // webOut.h (WebOut/WebDisplay, the HTTP+xml-stylesheet-PI route the new
  // menu.xslt is designed for) hardcodes <WebServer.h> — ESP32's class name,
  // not ESP8266's ESP8266WebServer. Untested on ESP8266 (disclosed gap, see
  // project_am4_compat_layer memory) — real ESP32 hardware only for now.
  #include <oneMenu/menu/IO/arduino/webOut.h>
#endif
#include <oneMenu/menu/cmdTable.h>
#include <oneMenu/menu/asyncNav.h>
#include <hapi/hapi.h>
#include <oneData/oneData.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

static const char* wifi_ssid = "YOUR_WIFI_SSID";
static const char* wifi_pass = "YOUR_WIFI_PASSWORD";

WebServerT server(80);
WebSocketsServer webSocket(81);

// ── Text — grabbed from examples/fields/src/main.cpp (Rui's own menu tree,
// covers plain items, a disabled-by-default item, a numeric field, and a
// submenu with TextField/NumField/Toggle/Select/Choose/date-pad) ───────────
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

namespace action {
  bool op1(Sz) { return true; }
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
// fire while the item's own XML tag is still open (see fields.h/xmlFmt.h
// fix notes, 2026-07-22).
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

WebSocketDisplay wsDisplay;
#ifndef ESP8266
WebDisplay webDisplay;
#endif

// shorthand command table, same convention as AquaGrow's own web.cpp cmds[][2]
static const CmdEntry cmds[] = {
  {"#SETTINGS", "/3/"},  // jump straight to the Data fields submenu
};

void pushRender() {
  wsDisplay.lockMode(LockMode::None);
  webNav.printTo(wsDisplay);
  webNav.sync(wsDisplay);
}

#ifndef ESP8266
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
#endif

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if(type == WStype_CONNECTED) { pushRender(); return; }
  if(type != WStype_TEXT) return;
  char path[32];
  const char* target = translateCmd((const char*)payload, cmds, path, sizeof(path))
    ? path : (const char*)payload;
  webNav.async(target);
  pushRender();
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
#ifndef ESP8266
  WebOut::begin(server);
#endif

  server.on("/", [](){
    server.send_P(200, "text/html", indexHtml);
  });

#ifndef ESP8266
  // Real HTTP+xml-stylesheet-PI route: the browser fetches this XML
  // directly and applies data/menu.xslt client-side (same architecture as
  // AquaGrow's own web.cpp pageStart(), and WebOut's own existing design)
  // — this is the route the adapted menu.xslt is built for, distinct from
  // the WebSocket push demo above.
  //
  // STATELESS by design: every request supplies its own ?at=<absolute path>
  // (default "/", the root menu) and the response is derived FRESH from
  // THAT alone, every single time. webNav's own cursor position never
  // carries meaning ACROSS separate requests — it's one server-wide object,
  // and "wherever the last request happened to leave it" breaks the moment
  // two browser tabs (or two different devices) talk to the same server, or
  // a client simply reloads the page (found 2026-07-22: got stuck inside
  // Choose's own submenu with no way back, since /menu had no way to ask
  // for anything OTHER than "wherever webNav currently sits"). Every link
  // menu.xslt generates embeds its own target ?at=, so a client can always
  // get back to any level directly — there is no implicit "current page".
  server.on("/menu", [](){
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/xml", "");
    server.sendContent(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<?xml-stylesheet type=\"text/xsl\" href=\"/menu.xslt\"?>\n"
    );
    String at = server.hasArg("at") ? server.arg("at") : String("/");
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
    webDisplay.lockMode(LockMode::None);
    webNav.printTo(webDisplay);
    // CONTENT_LENGTH_UNKNOWN makes WebServer use chunked transfer encoding
    // (WebServer.cpp: _chunked=true whenever content length is unknown on
    // HTTP/1.1) — a bare client().stop() here abruptly drops the connection
    // WITHOUT the terminating "0\r\n\r\n" chunk the chunked framing requires,
    // so the browser/curl has to wait out its own socket-read timeout
    // (~5s) before giving up and rendering whatever it got. sendContent("")
    // sends that proper terminator (WebServer::sendContent: contentLength==0
    // -> _chunked=false) — found 2026-07-22, "why does /menu take 5.39s".
    server.sendContent("");
  });

  // Apply a value at path, then redirect back to the page that generated
  // this link — the caller's own ?at= (menu.xslt's own /view/menu/@at,
  // baked into every /set link it generates), not a value derived from
  // `path` itself. Found 2026-07-22, real ESP32 hardware: a pad sub-field
  // (dateField's own year/month/day) has a path TWO levels deeper than its
  // real enclosing menu (e.g. "/3/5/0/" for year, when "Data fields..."
  // itself is "/3/") — computing the redirect via parentPath(path) alone
  // landed on "/3/5/" (dateField's own synthetic path, not a real,
  // navigable level), and requesting THAT via /menu?at=... fired a real
  // Enter on dateField (async()'s own per-segment Enter-firing plus
  // /menu's own explicit enter() call), which calls n.padOpen() — leaving
  // the nav's own edit-mode/pad state lingering and silently breaking
  // subsequent edits to OTHER, unrelated fields. webNav.setAt() (new,
  // asyncNav.h) sidesteps the other half of the same problem: it applies
  // the value via a plain path-driven setStr() walk, with NO go()/Enter/
  // padOpen() navigation at all — unlike async()+set(), which walked the
  // path via async()'s own Enter-firing and could trigger the exact same
  // padOpen() side effect during the SET itself, not just the redirect.
  // parentPath(path) is kept as a fallback for any caller that doesn't
  // supply ?at= (there shouldn't be one left, but failing toward the
  // field's own immediate parent is still safer than failing toward "/").
  server.on("/set", [](){
    String path = server.arg("path");
    webNav.setAt(path.c_str(), server.arg("val").c_str());
    String at;
    if(server.hasArg("at")) at = server.arg("at");
    else { char parent[32]; parentPath(path.c_str(),parent,sizeof(parent)); at = parent; }
    int sel = fieldSelIndex(path.c_str(), at.c_str());
    server.sendHeader("Location", String("/menu?at=")+at+"&sel="+String(sel));
    server.send(302, "text/plain", "");
  });

  // menu.xslt itself, served straight from flash (SPIFFS) — real static
  // front-end asset, same pattern as AquaGrow's own onWeb().
  server.serveStatic("/menu.xslt", SPIFFS, "/menu.xslt");
  // Styling now lives in its own real static asset too (not embedded in
  // menu.xslt's own <head>) — same serveStatic pattern, lets the browser
  // cache CSS independently of the XML payload. Split into a shared
  // common file plus two theme complements (menu.xslt links all three,
  // the two complements via media="(prefers-color-scheme: ...)" so the
  // browser picks the right one from the system's own light/dark setting
  // — no JS, no server-side detection needed).
  server.serveStatic("/style.css", SPIFFS, "/style.css");
  server.serveStatic("/style-dark.css", SPIFFS, "/style-dark.css");
  server.serveStatic("/style-light.css", SPIFFS, "/style-light.css");
#endif

  server.begin();
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
