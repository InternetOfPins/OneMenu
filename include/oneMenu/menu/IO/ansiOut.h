#pragma once

/**
 * @file ansiOut.h
 * @author Rui Azevedo (ruihfazevedo@gamil.com)
 * @brief ANSI output driver
 * @version 5
 * @date 2024-11-13 ~ 2026-03-11 ~ 2026-03-31
 * 
 */

// #include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/sys/platform/ansiCodes.h"

namespace oneMenu {
  /// @brief sends ANSI codes to the output
  struct ANSIOut {
    template<typename O>
    struct _Part:PartialDraw::Part<O> {
      // static_assert(O::template Requires<RawDevice>::value,"`ANSIOut` needs a `RawDevice` following");
      using HasANSI=std::true_type;
      using Base=typename PartialDraw::template Part<O>;
      using Base::Base;
      void setPos(Sz x,Sz y) {xy(Base::orgX()+x,Base::orgY()+y);}
      void setPos(const Pos& o) {setPos(o.x,o.y);}
      
      void nl() {
        O::nl();
        setPos(0,O::obj().pos().y);
      }

      void clear() {
        fill();
        setPos(0,0);
      }

      void xy(Sz x,Sz y) {
        preamble();
        Base::_put(y+1);Base::_put(';');
        Base::_put(x+1);Base::_put('H');
      }

      // template<typename Cor> static void _setColors(Cor f,Cor b) {_setColors(f,b);}
      template<typename Cor>
      void setColors(Cor f,Cor b) {
        setForegroundColor(f);
        setBackgroundColor(b);
      }

    private:
      void esc(){Base::_put((char)ESCAPE);}
      void preamble() {esc();Base::_put((char)BRACE);}
      void pnv(int x, char v){preamble();Base::_put(x);Base::_put(v);}
      void setAttribute(int a){pnv(a,'m');}
      void setBackgroundColor(int color) {setAttribute(color + 40);}
      void setForegroundColor(int color) {setAttribute(color + 30);}
      void fill(int x1, int y1, int x2, int y2,char ch=' ') {
        for (int y = y1; y < y2; y++) {
          setPos(x1,y);
          for (int x = x1; x < x2; x++) Base::_put(ch);
        }
      }
      void fill(char ch=' ') {fill(0,0,Base::width(),Base::height(),ch);}
    };
    template<typename O> using Part=typename DeviceCursor::Part<_Part<O>>;
  };

};//namespace oneMenu 