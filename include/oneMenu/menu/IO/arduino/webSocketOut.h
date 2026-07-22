#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#include <WebSocketsServer.h>
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/xmlFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // WebSocketOut: raw output device that buffers one full render into a String,
  // then pushes it as a single WebSocket text frame via broadcastTXT() on
  // flush() — unlike WebOut (HTTP request/response, one render per poll), this
  // device is driven the same way SerialOut/OledDisplay already are:
  //   if(nav.changed(wsDisplay)) {nav.printTo(wsDisplay); nav.sync(wsDisplay);}
  // in the main loop, so every connected browser gets pushed the new render
  // the moment nav state actually changes — no client-side polling/meta-
  // refresh needed. Call WebSocketOut::begin(server) once; call
  // server.loop() every loop() (real vendor WebSocketsServer requirement,
  // same as WebOut::begin()'s own server.handleClient() precedent).
  struct WebSocketOut : aRawDevice {
    inline static WebSocketsServer* _server = nullptr;
    static void begin(WebSocketsServer& srv) { _server = &srv; }

    template<typename O>
    struct _Part : O {
      inline static String _buf;
      static void put(char c) { _buf += c; }
      static void put(const char* s) { _buf += s; }
      static void put(const char* s, Sz n) { for(Sz i = 0; i < n && s[i]; i++) put(s[i]); }
      static void nl()                     { put('\n'); O::nl(); }
      static constexpr void setPos(const Pos&) {}
      static void clear()                  { _buf = ""; O::clear(); }
      static void flush() {
        if(_server) _server->broadcastTXT(_buf);
        _buf = "";
      }
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // Ready-made OutDef for websocket-pushed output — same XML shape as WebOut's
  // own WebDisplay (so both can share one client-side XSL/JS if a page wants
  // to render either transport identically), just pushed instead of pulled.
  using WebSocketDisplay = OutDef<
    FullPrinter,
    XmlFmt,
    DataParser<>,
    Cursor<1, 1>,
    WebSocketOut,
    StaticPos<0, 0>,
    StaticArea<80, 25>
  >;

} // namespace oneMenu
#endif
