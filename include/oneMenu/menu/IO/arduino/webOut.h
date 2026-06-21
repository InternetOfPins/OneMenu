#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/xmlFmt.h"
#include "oneMenu/menu/sys/printers.h"

namespace oneMenu {

  // WebOut: raw output device that buffers to a static Arduino String.
  // Used with XmlFmt to serve menu state as XML over HTTP.
  // One shared buffer — only one WebDisplay instance expected.
  struct WebOut : aRawDevice {
    static String& buf() { static String s; return s; }

    template<typename O>
    struct _Part : O {
      static void put(char c)              { WebOut::buf() += c; }
      static void put(const char* s)       { WebOut::buf() += s; }
      static void put(const char* s, Sz n) {
        for(Sz i = 0; i < n && s[i]; i++) WebOut::buf() += s[i];
      }
      static void nl()                     { WebOut::buf() += '\n'; O::nl(); }
      static constexpr void setPos(const Pos&) {}
      static void clear()                  { WebOut::buf() = ""; O::clear(); }
      static void resume()                 { WebOut::buf() = ""; O::resume(); }
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
    XmlFmt,
    DataParser<>,
    Cursor<1, 1>,
    WebOut,
    StaticPos<0, 0>,
    StaticArea<80, 25>
  >;

} // namespace oneMenu
#endif
