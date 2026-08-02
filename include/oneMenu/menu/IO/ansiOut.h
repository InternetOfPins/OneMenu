#pragma once

/**
 * @file ansiOut.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief ANSI output driver
 * 
 */

#include "oneMenu/menu/out.h"
#include "oneMenu/menu/sys/platform/ansiCodes.h"

namespace oneMenu {
  /// @brief sends ANSI codes to the output
  struct ANSIOut:PartialDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<RawDevice, After>, "ANSIOut: Raw (or aRawDevice) must be placed below ANSIOut — ANSIOut sends escape codes directly via _put()");
      return true;
    }
    template<typename O>
    struct _Part:PartialDraw::Part<O> {
      using HasANSI=std::true_type;
      using Base=typename PartialDraw::template Part<O>;
      using Base::Base;
      void setPos(const Pos& o) {xy(Base::orgX()+o.x,Base::orgY()+o.y);}
      
      void nl() {
        O::nl();
        setPos({0,O::obj().getPos().y});
      }

      void clear() {
        fillRect(0,0,Base::width(),Base::height());
        setPos({0,0});
      }

      void xy(Sz x,Sz y) {
        preamble();
        Base::_put(y+1);Base::_put(';');
        Base::_put(x+1);Base::_put('H');
      }

      template<typename Cor>
      void setColors(Cor f,Cor b) {
        setForegroundColor(f);
        setBackgroundColor(b);
      }

      // (x,y,w,h) — same convention as every GFX fillRect (adaGfxVendor.h,
      // oledOut.h: x,y,w,h,+optional color/byte, ignored here). Was a private,
      // (x1,y1,x2,y2)-shaped `fill()` — slightly different signature but
      // compatible — renamed+widened to public so ScrollBodyPrinter's own
      // unconditional Base::fillRect(...) call (printers.h) resolves on a
      // plain ANSI chain too, not just GFX devices. Still beta: character-by-
      // character space-fill via cursor addressing, not a real hardware rect
      // fill — fine for a terminal, just don't expect GFX-speed.
      void fillRect(Sz x,Sz y,Sz w,Sz h,char ch=' ') {
        for (Sz row = y; row < y+h; row++) {
          setPos({x,row});
          for (Sz col = x; col < x+w; col++) Base::_put(ch);
        }
      }

    private:
      void esc(){Base::_put((char)ESCAPE);}
      void preamble() {esc();Base::_put((char)BRACE);}
      void pnv(int x, char v){preamble();Base::_put(x);Base::_put(v);}
      void setAttribute(int a){pnv(a,'m');}
      void setBackgroundColor(int color) {setAttribute(color + 40);}
      void setForegroundColor(int color) {setAttribute(color + 30);}
    };
    template<typename O> using Part=typename DeviceCursor::Part<_Part<O>>;
  };

};//namespace oneMenu 