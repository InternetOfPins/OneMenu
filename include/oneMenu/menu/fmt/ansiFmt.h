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

  struct ANSIFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "ANSIFmt: printer layers must be placed above ANSIFmt");
      return true;
    }
    template<typename O>
    struct Part:UseEditCursorFmt::template Part<O> {
      using Base=typename UseEditCursorFmt::template Part<O>;
      using Base::setColors;
      using Base::clear;
      using Base::nl;
      using Base::clearLine;
      using Base::clearFree;
      using Base::put;
      using Base::resume;

      static Colors<int> fb(const Ctx& ctx) {
          if(ctx.pad&&ctx.psel()) {//pad focus
            if(ctx) {//selected
              switch(ctx.after()) {//pad nav mode
                default: return {ctx&&ctx.enabled?WHITE:BLACK,GREEN};//parent nav
                case 2: return {ctx.enabled?GREEN:BLACK,WHITE};//pad nav
                case 3: return {BLUE,WHITE};//pad edit
              }
            } else return {ctx.enabled?WHITE:BLACK,GREEN};//on pad with parent selected
          } else if(ctx && (!ctx.pad || (ctx.psel() && ctx.after() > 1))) return {ctx.enabled?WHITE:BLACK,GREEN};//pad parent focus
          return {ctx.enabled?WHITE:BLACK,BLUE};//unselected
      }

      // start ---
      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        switch(tag) {//for colors
          default:break;
          case Fmt::View:
            setColors(WHITE,BLUE);
            clear();
            break;
          case Fmt::Title: setColors(BLUE,WHITE);break;
          // case Fmt::Field: setColors(ctx.enabled?WHITE:BLACK,fb(ctx).bg);break;
          case Fmt::Item: {
            Colors<int> o=fb(ctx);
            setColors(o.fg,o.bg);
          } break;
        }
        switch(tag){//for text
          case Fmt::EditMode:
            if(ctx && (!ctx.pad || (ctx.sel(ctx.pAt) == ctx.pIdx))) switch(ctx.mode) {
              default: break;
              case NavMode::Nav: put(':');break;
              case NavMode::Edit: put('=');break;
              case NavMode::Tune: put('.');break;
            } else if(!ctx.pad) put(' ');
            break;
          case Fmt::NavCursor: 
            if(!ctx.pad) {
              if (ctx) put(ctx.enabled?'>':'-');
              else put(' ');
            }
            break;
          case Fmt::Index: 
            if(!ctx.pad) put(ctx.idx<9?1+ctx.idx:' ');
            break;
          default:break;
        }
        Base::template fmtStart<tag>(ctx);
      }

      // stop --
      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        if((tag&Fmt::Item)&&ctx&&(ctx.sel(ctx.pAt)==ctx.pIdx)) setColors(ctx.enabled?WHITE:BLACK,GREEN);
        Base::template fmtStop<tag>(ctx);
      }
    };
  };

};//namespace oneMenu 