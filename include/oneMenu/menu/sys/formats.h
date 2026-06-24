#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief base mixin for format layers; marks layer as IsFormat
  struct Formats : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "Formats: printer layers must be placed above format layers — printers drive the format pipeline, not the reverse");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsFormat=std::true_type;
    };
  };

  /// @brief clears unused lines below body and trailing pixels after each item/title
  struct ClearFreeFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "ClearFreeFmt: Cursor must be placed below ClearFreeFmt — clearFree is a no-op without a real tracked cursor");
      return true;
    }
    template<typename O>
    struct Part:O {
      using Base=O;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::clear;
      using Base::clearToEOL;
      using Base::clearLine;
      using Base::clearFree;
      using Base::setColors;
      template<Fmt tag>
      std::enable_if_t<tag==Fmt::View>
      fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);
        if(!ctx.pad) clearFree();
      }
      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Title|Fmt::Item)>
      fmtStop(const Ctx& ctx) {
        if(!ctx.pad) clearToEOL();       // erase trailing pixels before base nl
        Base::template fmtStop<tag>(ctx);
      }
    };
  };

  /// @brief restores (nav|text edit) cursor position at printing end, 
  /// base chain class F must have `DeviceCursor`
  struct UseEditCursorFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "UseEditCursorFmt: printer layers must be placed above UseEditCursorFmt");
      return true;
    }
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
