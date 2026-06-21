#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#include <WebServer.h>
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/htmlFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // WebOut: raw output device that streams directly to a WebServer via sendContent().
  // No host-side buffer — each put() sends immediately; TCP Nagle coalesces small writes.
  // Call WebOut::begin(server) once before serving requests.
  struct WebOut : aRawDevice {
    inline static WebServer* _server = nullptr;
    static void begin(WebServer& srv) { _server = &srv; }

    template<typename O>
    struct _Part : O {
      static void put(char c) {
        if(_server) { char b[2] = {c, '\0'}; _server->sendContent(b); }
      }
      static void put(const char* s) {
        if(_server) _server->sendContent(s);
      }
      static void put(const char* s, Sz n) {
        if(_server) { for(Sz i = 0; i < n && s[i]; i++) put(s[i]); }
      }
      static void nl()                     { put('\n'); O::nl(); }
      static constexpr void setPos(const Pos&) {}
      static void clear()                  { O::clear(); }
      static void resume()                 { O::resume(); }
      static constexpr void flush()        {}
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // Ready-made OutDef for HTTP/XML output.
  // Cursor<1,1> satisfies ItemPrinter's IsCursor requirement; position tracking is harmless.
  using WebDisplay = OutDef<
    FullPrinter,
    HtmlFmt,
    DataParser<>,
    Cursor<1, 1>,
    WebOut,
    StaticPos<0, 0>,
    StaticArea<80, 25>
  >;

} // namespace oneMenu
#endif
