#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#include <WebServer.h>
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/xmlFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // WebOut: raw output device that buffers one full render into a String,
  // then sends it as a single WebServer::sendContent() call on flush()
  // (called automatically by TreeNav::printTo/DefinedNav::doOutput, out.h).
  // NOT one sendContent() per character (an earlier, real design mistake,
  // found 2026-07-22 — a ~200-byte XML render took 5+ real seconds over
  // WiFi: WebServer.cpp uses chunked transfer encoding whenever content
  // length is unknown, so every single-character put() was its own HTTP
  // chunk — hundreds of tiny chunked writes, each subject to real-world
  // Nagle/delayed-ACK stalls). Same buffer-then-flush pattern WebSocketOut
  // already used correctly from the start.
  // Call WebOut::begin(server) once before serving requests.
  struct WebOut : aRawDevice {
    inline static WebServer* _server = nullptr;
    static void begin(WebServer& srv) { _server = &srv; }

    static const char* xsl() {
      static const char s[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">"
        "<xsl:output method=\"html\"/>"
        "<xsl:template match=\"/view\">"
        "<html><head><title>OneMenu</title>"
        "<meta http-equiv=\"refresh\" content=\"5\"/>"
        "<style>"
        "body{font-family:monospace;background:#111;color:#0f0;padding:10px}"
        ".m{border:1px solid #0f0;display:inline-block;min-width:180px}"
        ".t{font-size:1.2em;font-weight:bold;padding:4px 8px;border-bottom:1px solid #0f0}"
        ".i{padding:3px 8px}.s{background:#0f0;color:#111}"
        ".n{margin-top:8px}.n a{color:#0f0;border:1px solid #0f0;padding:3px 10px;"
        "text-decoration:none;margin-right:3px}.n a:hover{background:#0f0;color:#111}"
        "</style></head><body>"
        "<div class=\"m\"><xsl:apply-templates/></div>"
        "<div class=\"n\">"
        "<a href=\"/cmd?nav=up\">&#x2191;</a>"
        "<a href=\"/cmd?nav=dn\">&#x2193;</a>"
        "<a href=\"/cmd?nav=en\">Enter</a>"
        "<a href=\"/cmd?nav=esc\">Esc</a>"
        "</div></body></html>"
        "</xsl:template>"
        "<xsl:template match=\"menu\"><xsl:apply-templates/></xsl:template>"
        "<xsl:template match=\"title\"><div class=\"t\"><xsl:value-of select=\".\"/></div></xsl:template>"
        "<xsl:template match=\"body\"><xsl:apply-templates/></xsl:template>"
        "<xsl:template match=\"item\">"
        "<div><xsl:attribute name=\"class\"><xsl:choose>"
        "<xsl:when test=\"@ncur='@'\">i s</xsl:when>"
        "<xsl:otherwise>i</xsl:otherwise>"
        "</xsl:choose></xsl:attribute>"
        "<xsl:value-of select=\".\"/></div>"
        "</xsl:template>"
        "<xsl:template match=\"footer\"><div><xsl:value-of select=\".\"/></div></xsl:template>"
        "</xsl:stylesheet>";
      return s;
    }

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
        if(_server) _server->sendContent(_buf);
        _buf = "";
      }
      static constexpr Sz charWidth()      { return 1; }
      static constexpr Sz lineSpacing()    { return 1; }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // Ready-made OutDef for HTTP/XML output.
  // Cursor<1,1> satisfies ItemPrinter's IsCursor requirement; position tracking is harmless.
  using WebDisplay = OutDef<
    FullPrinter,
    XmlFmt,
    DataParser<>,
    Cursor<1, 1>,
    WebOut,
    StaticPos<0, 0>,
    StaticArea<80, 25>
  >;

} // namespace oneMenu
#endif
