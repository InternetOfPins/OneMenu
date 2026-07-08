#pragma once

#ifdef MENU_DEBUG
  #include <iostream>
  #include "oneMenu/menu/sys/enums.h"
  #include "oneMenu/menu/sys/platform/ansiCodes.h"

  namespace oneMenu {
    enum {debug};
    template<int id=debug> int cnt{0};

    struct EmptyPart {template<typename O> using Part=O;};

    template<typename Out,Out& out>
    struct DebugOut {
      static constexpr const Out& device{out};
      static constexpr void nl() {endl(out);}
      static constexpr void flush() {out.flush();}
      static constexpr void hex() {out<<hex;}
      template<typename T> static constexpr void put(const T& o) {out<<o;}
      static void _nl() {nl();}
      static void _flush() {flush();}
      template<typename T> static void _put(const T o) {put(o);}
      void padWith(int n,const char o=' ') {for(;n>0;n--) put(o);}
      void xy(int x,int y) {
        preamble();
        put(y+1);put(';');
        put(x+1);put('H');
      }

      template<typename Cor>
      static void setColors(Cor f,Cor b) {
        setForegroundColor(f);
        setBackgroundColor(b);
      }
      static void esc(){put((char)ESCAPE);}
      static void preamble() {esc();put((char)BRACE);}
      static void pnv(int x, char v){preamble();put(x);put(v);}
      static void setAttribute(int a){pnv(a,'m');}
      static void setBackgroundColor(int color) {setAttribute(color + 40);}
      static void setForegroundColor(int color) {setAttribute(color + 30);}
      template<Fmt tag> static constexpr void fmtStart(const Ctx& ctx) {}
      template<Fmt tag> static constexpr void fmtStop(const Ctx& ctx) {}
    };

    using DOut=DebugOut<decltype(std::cerr),std::cerr>;
    DOut dout;

    template<template<typename...> class T,typename... NN>
    DOut& operator<<(DOut& out,T<NN...>& o) {Ctx ctx;o.printItem(out,ctx);return out;}

    DOut& endl (DOut& s) {s.nl();return s;}
    DOut& flush(DOut& s) {s.flush();return s;}
    DOut& hex(DOut& s) {s.hex();return s;}
    template<typename O,O& o> DOut& resume(DOut& s) {o.resume();return s;}

    template<Sz x,Sz y> DOut& xy(DOut& s) {s.xy(x,y);return s;}
    template<Sz n,char c=' '> DOut& padWith(DOut& s) {s.padWith(n,c);return s;}
    template<int f,int b> DOut& colors(DOut& s) {s.setColors(f,b);return s;}

    template<size_t n> using NChars=char[n];
      template<size_t n> 
    DOut& operator<<(DOut& out,const NChars<n> t){for(int i=0;i<n;i++) out.put(t[i]);return out;}

    template<typename N> 
    std::enable_if_t<std::is_integral_v<N>,DOut&> operator<<(DOut& out,N i){out.put(i);return out;}

    template<typename... OO> 
    DOut& operator<<(DOut& out,const char* t){for(int i=0;t[i];i++) out.put(t[i]);return out;}

    template<typename... OO> 
    DOut& operator<<(DOut& out,DOut&(f)(DOut&))
      {f(out);return out;}
      
  };//namespace oneMenu
      
#endif
