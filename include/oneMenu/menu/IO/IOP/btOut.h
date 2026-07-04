#pragma once
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/fmt/btFmt.h"

namespace oneMenu {

  // BtOut<Ble,Id,BufSz>: accumulates the rendered stream into a local buffer and
  // flushes it as one characteristic write (Ble::char_write(Id,...)) on flush() —
  // same slot as UartSerialOut/WebOut, place inside OutDef. Pair with BtFmt above it
  // in the chain (see menu/fmt/btFmt.h) so only Field/Data values reach here — this is
  // the coarse per-tree/per-subtree record; oneData::BTRec is the per-field path and
  // writes its own characteristic directly via Ble::char_write, bypassing this device.
  // Ble is duck-typed: char_write(id,data,len)/char_read/char_written (see oneBus::BleAPI).
  template<typename Ble, uint16_t Id, Sz BufSz = 20>
  struct BtOut : aRawDevice {
    template<typename O>
    struct _Part : O {
      using Base = O;
      inline static char _buf[BufSz]{};
      inline static Sz   _len{0};

      static void put(char c) {
        if(_len < BufSz) _buf[_len++] = c;
        Base::put(c);
      }
      static void put(const char* s) {
        while(*s) { if(_len < BufSz) _buf[_len++] = *s; s++; }
        Base::put(s);
      }
      static void put(const char* s, Sz n) {
        for(Sz i = 0; i < n && s[i]; i++) if(_len < BufSz) _buf[_len++] = s[i];
        Base::put(s, n);
      }
      static void nl()               { Base::nl(); } // one record = one accumulated value, no lines
      static void setPos(const Pos&) {}
      static void flush() {
        Ble::char_write(Id, reinterpret_cast<const uint8_t*>(_buf), (uint8_t)_len);
        _len = 0;
        Base::flush();
      }
    };
    template<typename O> using Part = Raw::Part<_Part<O>>;
  };

  // Ready-made OutDef for a compact per-tree BT record — pairs BtFmt (values only,
  // no chrome) with BtOut (accumulate + flush to one characteristic). Cursor<1,1>
  // satisfies ItemPrinter's IsCursor requirement, same as WebDisplay.
  template<typename Ble, uint16_t Id, Sz BufSz = 20>
  using BtDisplay = OutDef<
    FullPrinter,
    BtFmt,
    DataParser<>,
    Cursor<1, 1>,
    BtOut<Ble, Id, BufSz>,
    StaticPos<0, 0>,
    StaticArea<80, 25>
  >;

} // namespace oneMenu
