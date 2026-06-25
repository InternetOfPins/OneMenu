/**
 * @file textFmt.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief text format
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  /// @brief Default cursor/separator characters for TextFmt
  struct MenuChars {
    static constexpr char focus    = '>';  // focused item, enabled
    static constexpr char focusDis = '-';  // focused item, disabled
    static constexpr char blur     = ' ';  // unfocused item
    static constexpr char sepNav   = ':';  // edit-mode separator, NavMode::Nav
    static constexpr char sepEdit  = '=';  // edit-mode separator, NavMode::Edit
    static constexpr char sepTune  = '.';  // edit-mode separator, NavMode::Tune
  };

  /// @brief plain text format: one line per item, no color.
  struct TextFmt : aFormat {
    using Chars = MenuChars;
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "TextFmt: printer layers must be placed above TextFmt");
      return true;
    }
    template<typename O>
    struct Part:Formats::template Part<O> {
      using Base=typename Formats::template Part<O>;
      using Base::nl;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::put;
      using Base::resume;

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        switch(tag) {
          default:break;
          case Fmt::NavCursor:
            put(ctx?(ctx.enabled?Chars::focus:Chars::focusDis):Chars::blur);
            break;
          case Fmt::EditMode:
            if(ctx) switch(ctx.mode) {
              case NavMode::Nav:  put(Chars::sepNav);  break;
              case NavMode::Edit: put(Chars::sepEdit); break;
              case NavMode::Tune: put(Chars::sepTune); break;
            } else if(!ctx.pad) put(Chars::blur);
            break;
        }
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        switch(tag){
          case Fmt::View:
          case Fmt::Title:
          case Fmt::Item:
            if(!ctx.pad) Base::nl();
            break;
          default:break;
        }
        Base::template fmtStop<tag>(ctx);
      }
    };
  };

};//namespace oneMenu