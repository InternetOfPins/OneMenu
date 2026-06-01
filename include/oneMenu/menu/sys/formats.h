#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  struct Formats {
    template<typename O>
    struct Part:O {
      using IsFormat=std::true_type;
      // static_assert(O::template Excludes<IsPrinter>::value,"Printers must preseed Formats");//TODO:: then chop OutDef into Printers|Fmt|Dev
    };
  };

  struct ClearFreeFmt {
    template<typename O>
    struct Part:O {
      // static_assert(O::template Requires<IsCursor>::value,"ClearFree needs a valid Cursor (IsCursor class) not the API default fallback");
      using Base=O;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::clear;
      using Base::clearLine;
      using Base::clearFree;
      using Base::setColors;
      template<Fmt tag> 
      std::enable_if_t<tag&(Fmt::Title|Fmt::Item)>
      fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);
        if(!ctx.pad) clearLine();
      }
    };
  };

  /// @brief restores (nav|text edit) cursor position at printing end, 
  /// base chain class F must have `DeviceCursor`
  struct UseEditCursorFmt {
    template<typename F>
    struct Part:Formats::template Part<F> {
      using Base=typename Formats::template Part<F>;
      using Base::fmtStart;
      using Base::fmtStop;
      //force renew of editing state before printing, if still valid
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::EditCursor> fmtStart(const Ctx& ctx) {
        if(!F::locked()) F::m_editing=false;
        F::template fmtStart<tag>(ctx);
      }
      //restores meaningful cursor position after printing done
      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        F::template fmtStop<tag>(ctx);
        if(tag==Fmt::View) {
          if(F::locked()) F::lockMode(LockMode::Update);
          F::setPos(F::m_text_cursor_at);
          F::flush();
        }
      }
    };
  };

};//namespace oneMenu
