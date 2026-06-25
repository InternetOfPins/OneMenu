/**
 * @file ansiFmt.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief ANSI format file
 * @version 5
 * @date 2026-04-29
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  /// @brief Default ANSI color palette
  struct ANSIColors {
    static constexpr int view_fg=WHITE,  view_bg=BLUE;
    static constexpr int title_fg=BLUE,  title_bg=WHITE;
    static constexpr int sel_bg=GREEN,   unsel_bg=BLUE;
    static constexpr int ena_fg=WHITE,   dis_fg=BLACK;
    static constexpr int pad_nav_fg=GREEN, pad_nav_dis_fg=BLACK, pad_nav_bg=WHITE;
    static constexpr int pad_edit_fg=BLUE, pad_edit_bg=WHITE;
  };

  /// @brief ANSI terminal format: color sequences, cursor positioning, partial repaint support.
  /// @tparam Palette  color scheme (default ANSIColors); @tparam Chars  cursor chars (default MenuChars)
  template<typename Palette=ANSIColors, typename Chars=MenuChars>
  struct ANSIFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "ANSIFmt: printer layers must be placed above ANSIFmt");
      return true;
    }
    template<typename O>
    struct Part:UseEditCursorFmt::template Part<O> {
      using Base=typename UseEditCursorFmt::template Part<O>;
      using P=Palette;
      using C=Chars;
      using Base::setColors;
      using Base::clear;
      using Base::nl;
      using Base::clearToEOL;
      using Base::clearLine;
      using Base::clearFree;
      using Base::put;
      using Base::resume;

      static Colors<int> fb(const Ctx& ctx) {
        if(ctx.pad&&ctx.psel()) {
          if(ctx) switch(ctx.after()) {
            default: return {ctx.enabled?P::ena_fg:P::dis_fg, P::sel_bg};
            case 2:  return {ctx.enabled?P::pad_nav_fg:P::pad_nav_dis_fg, P::pad_nav_bg};
            case 3:  return {P::pad_edit_fg, P::pad_edit_bg};
          } else return {ctx.enabled?P::ena_fg:P::dis_fg, P::sel_bg};
        } else if(ctx && (!ctx.pad||(ctx.psel()&&ctx.after()>1)))
          return {ctx.enabled?P::ena_fg:P::dis_fg, P::sel_bg};
        return {ctx.enabled?P::ena_fg:P::dis_fg, P::unsel_bg};
      }

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        switch(tag) {
          default:break;
          case Fmt::View:  setColors(P::view_fg, P::view_bg); clear(); break;
          case Fmt::Title: setColors(P::title_fg,P::title_bg); break;
          case Fmt::Item: { auto o=fb(ctx); setColors(o.fg,o.bg); } break;
        }
        switch(tag) {
          case Fmt::EditMode:
            if(ctx && (!ctx.pad||(ctx.sel(ctx.pAt)==ctx.pIdx))) switch(ctx.mode) {
              default: break;
              case NavMode::Nav:  put(C::sepNav);  break;
              case NavMode::Edit: put(C::sepEdit); break;
              case NavMode::Tune: put(C::sepTune); break;
            } else if(!ctx.pad) put(C::blur);
            break;
          case Fmt::NavCursor:
            if(!ctx.pad) put(ctx?(ctx.enabled?C::focus:C::focusDis):C::blur);
            break;
          case Fmt::Index:
            if(!ctx.pad) put(ctx.idx<9?1+ctx.idx:' ');
            break;
          default:break;
        }
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        if(tag&Fmt::Item) {
          if(ctx&&(ctx.sel(ctx.pAt)==ctx.pIdx)) setColors(ctx.enabled?P::ena_fg:P::dis_fg, P::sel_bg);
          clearToEOL();
          Base::nl();
        } else if(tag==Fmt::Title) {
          clearToEOL();
          Base::nl();
        } else if(tag==Fmt::View) {
          setColors(P::view_fg,P::view_bg);
          clearFree();
        }
        Base::template fmtStop<tag>(ctx);
      }
    };
  };

};//namespace oneMenu