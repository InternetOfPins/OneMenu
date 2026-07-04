#pragma once

/**
 * @file btFmt.h
 * @brief Compact format for BT/BLE payloads: only Field/Data value bytes, comma-separated,
 *        no paths/labels/nav-cursor/edit-mode chrome (unlike XmlFmt/JsonFmt, which dump the
 *        whole tree for a remote-viewer UI). Sized for small GATT characteristics, not HTTP.
 *
 * Always forwards fmtStart<tag>/fmtStop<tag> to Base for every tag (same as XmlFmt/JsonFmt) —
 * only put() is gated, so structural/cursor bookkeeping in lower layers is undisturbed; nothing
 * is emitted except what happens between a Field/Data fmtStart and its matching fmtStop.
 */

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  struct BtFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "BtFmt: printer layers must be placed above BtFmt");
      return true;
    }
    template<typename O>
    struct Part : Formats::template Part<O> {
      using Base = typename Formats::template Part<O>;

      bool inValue{false};
      bool needSep{false};

      template<typename T>
      void put(T v) {
        if(!inValue) return;
        if(needSep) { Base::put(','); needSep=false; }
        Base::put(v);
      }
      void put(const char* s, Sz n) {
        if(!inValue) return;
        if(needSep) { Base::put(','); needSep=false; }
        Base::put(s,n);
      }

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        if constexpr (tag==Fmt::Field||tag==Fmt::Data) inValue=true;
        Base::template fmtStart<tag>(ctx);
      }
      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);
        if constexpr (tag==Fmt::Field||tag==Fmt::Data) { inValue=false; needSep=true; }
      }
    };
  };

};//namespace oneMenu
