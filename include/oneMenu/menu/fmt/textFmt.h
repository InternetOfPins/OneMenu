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
  struct TextFmt {
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
            put(ctx?(ctx.enabled?'>':'-'):' ');
            break;
          case Fmt::EditMode:
            if(ctx) switch(ctx.mode) {
              case NavMode::Nav: put(':');break;
              case NavMode::Edit: put('=');break;
              case NavMode::Tune: put('.');break;
            } else if(!ctx.pad) put(' ');
            break;
        }
      }

      template<Fmt tag>
      void fmtStop(const Ctx&ctx) {
        switch(tag){
          default:break;
          case Fmt::View:
          case Fmt::Title:
          case Fmt::Item:
            if(!ctx.pad) Base::nl();
        }
      }

    };
  };
};//namespace oneMenu 