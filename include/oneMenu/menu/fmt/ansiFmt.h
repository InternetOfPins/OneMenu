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
#pragma once

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/sys/colors.h"
#include "oneMenu/menu/fmt/textFmt.h"

namespace oneMenu {

  /// @brief ANSI terminal format: color sequences, cursor positioning, partial repaint support.
  /// Colors come from a Color<int>::Table<...> — either supplied via a ColorTable<...>
  /// component placed below ANSIFmt in the output chain, or this built-in default
  /// (enforced: ColorTable<> must be placed below ANSIFmt, see rules() below).
  struct ANSIFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "ANSIFmt: printer layers must be placed above ANSIFmt");
      static_assert(Excludes<hapi::IsInstanceOf<ColorTable>, Before>,
        "ANSIFmt: ColorTable<> must be placed below ANSIFmt (or omitted to use the built-in default palette)");
      return true;
    }
    template<typename O>
    struct Part:UseEditCursorFmt::template Part<O> {
      using Base=typename UseEditCursorFmt::template Part<O>;
      using C=MenuChars;
      using Base::setColors;
      using Base::clear;
      using Base::nl;
      using Base::clearToEOL;
      using Base::clearLine;
      using Base::clearFree;
      using Base::put;
      using Base::resume;

      // built-in default palette — reproduces the previous hardcoded ANSIColors exactly
      using DefaultPalette = typename Color<int>::template Table<
        /*Title*/   typename Color<int>::template Colors<BLUE,WHITE>,
        /*Default*/ typename Color<int>::template Colors<WHITE,BLUE>,
        /*View*/    typename Color<int>::template Colors<WHITE,BLUE>,
        /*Nav*/ typename Color<int>::template Nav<
          /*Enabled*/ typename Color<int>::template Enabled<
            typename Color<int>::template Item<
              /*Body*/     typename Color<int>::template Colors<WHITE,BLUE>,
              /*Field*/    typename Color<int>::template Colors<GREEN,WHITE>,
              /*EditMode*/ typename Color<int>::template Colors<BLUE,WHITE>
            >,
            /*Selected*/ typename Color<int>::template Colors<WHITE,GREEN>
          >,
          /*Disabled*/ typename Color<int>::template Enabled<
            typename Color<int>::template Item<
              /*Body*/     typename Color<int>::template Colors<BLACK,BLUE>,
              /*Field*/    typename Color<int>::template Colors<BLACK,WHITE>,
              /*EditMode*/ typename Color<int>::template Colors<BLUE,WHITE>
            >,
            /*Selected*/ typename Color<int>::template Colors<BLACK,GREEN>
          >
        >
      >;

      // resolve the active table: a ColorTable<...> placed anywhere in the chain wins,
      // else fall back to DefaultPalette above. See rules(): must sit below ANSIFmt.
      using Found = typename hapi::FindFirstOr<hapi::IsInstanceOf<ColorTable>, ColorTable<DefaultPalette>>
                      ::template Check<typename O::Types>;
      using P = typename Found::Type;
      using NavEn  = typename P::Nav::Enabled;
      using NavDis = typename P::Nav::Disabled;

      // unwrap a compile-time Colors<f,b> tag into the runtime {fg,bg} pair setColors() needs
      template<int f,int b>
      static constexpr Colors<int> unwrap(typename Color<int>::template Colors<f,b>) {return {f,b};}

      static Colors<int> fb(const Ctx& ctx) {
        if(ctx.pad&&ctx.psel()) {
          if(ctx) switch(ctx.after()) {
            default: return ctx.enabled?unwrap(typename NavEn::Selected{}):unwrap(typename NavDis::Selected{});
            case 2:  return ctx.enabled?unwrap(typename NavEn::Item::Field{}):unwrap(typename NavDis::Item::Field{});
            case 3:  return unwrap(typename NavEn::Item::EditMode{});
          } else return ctx.enabled?unwrap(typename NavEn::Selected{}):unwrap(typename NavDis::Selected{});
        } else if(ctx && (!ctx.pad||(ctx.psel()&&ctx.after()>1)))
          return ctx.enabled?unwrap(typename NavEn::Selected{}):unwrap(typename NavDis::Selected{});
        return ctx.enabled?unwrap(typename NavEn::Item::Body{}):unwrap(typename NavDis::Item::Body{});
      }

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        switch(tag) {
          default:break;
          case Fmt::View:  { auto o=unwrap(typename P::View{}); setColors(o.fg,o.bg); } clear(); break;
          case Fmt::Title: { auto o=unwrap(typename P::Title{}); setColors(o.fg,o.bg); } break;
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
            // must resolve to put(char) (out.h DataParser, single raw glyph), not put(int)
            // (DataParser's "%i" decimal-text path): the ternary's branches are int (1+ctx.idx)
            // and char (' ') — usual arithmetic conversions promote ' ' to int 32, so an
            // unqualified put(...) prints the *decimal text* "32" for idx>=9 instead of a
            // blank glyph (1-9 looked correct only by coincidence: decimal text of 1..9 is a
            // single character). The explicit char(...) cast forces put(char) in both branches.
            if(!ctx.pad) put(char(ctx.idx<9?'1'+ctx.idx:' '));
            break;
          default:break;
        }
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        if(tag&Fmt::Item) {
          if(ctx&&(ctx.sel(ctx.pAt)==ctx.pIdx)) {
            auto o=ctx.enabled?unwrap(typename NavEn::Selected{}):unwrap(typename NavDis::Selected{});
            setColors(o.fg,o.bg);
          }
          clearToEOL();
          Base::nl();
        } else if(tag==Fmt::Title) {
          clearToEOL();
          Base::nl();
        } else if(tag==Fmt::View) {
          { auto o=unwrap(typename P::View{}); setColors(o.fg,o.bg); }
          clearFree();
        }
        Base::template fmtStop<tag>(ctx);
      }
    };
  };

};//namespace oneMenu
