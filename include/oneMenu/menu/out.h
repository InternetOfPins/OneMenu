/**
 * @file out.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief output API
 * @version 5
 * @date 2026-04-17
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

#include "menu/sys/base.h"
#include "menu/sys/formats.h"

template<typename Cfg=Nil>
struct OutAPI:Cfg {
  using Config=Cfg;
  using IsBuffer=std::false_type;
  using IsArea=std::false_type;
  using IsDataParser=std::false_type;
  using IsParser=std::false_type;
  using IsFormat=std::false_type;
  using IsPrinter=std::false_type;
  using RawDevice=std::false_type;
  using IsCursor=std::false_type;
  using HasGate=std::false_type;
  using HasPartialUpdate=std::false_type;
  template<typename> using Requires=std::false_type;
  template<typename> using Excludes=std::true_type;
  using Cfg::obj;
  static constexpr Sz freeX() {return std::numeric_limits<Sz>::max();}
  static constexpr Sz freeY() {return std::numeric_limits<Sz>::max();}
  static constexpr Area free() {return {freeX(),freeY()};}
  static constexpr Sz posX() {return 0;}
  static constexpr Sz posY() {return 0;}
  static constexpr Pos pos() {return {posX(),posY()};}
  static void setPos(Sz,Sz) {}
  static void setPos(Pos p) {setPos(p.x,p.y);}
  static constexpr Sz orgX() {return 0;}
  static constexpr Sz orgY() {return 0;}
  static constexpr Pos org() {return {orgX(),orgY()};}
  static constexpr bool unlocked() {return true;}
  static constexpr bool updating() {return false;}
  static constexpr bool locked() {return false;}
  static constexpr LockMode lockMode() {return LockMode::None;}
  static constexpr void lockMode(LockMode) {}
  template<typename Cor> static void setColors(Cor,Cor) {}

  static constexpr void clear() {}
  static constexpr void nl() {}
  static constexpr void flush() {}
  static constexpr void resume() {}
  template<typename T> static constexpr void put(const T&) {}
  template<Fmt tag> static constexpr void fmtStart(const Ctx& ctx) {}
  template<Fmt tag> static constexpr void fmtStop(const Ctx& ctx) {}
  // static constexpr void fmtStart(Fmt tag,const Ctx& ctx) {}
  // static constexpr void fmtStop (Fmt tag,const Ctx& ctx) {}
  template<typename Item> static constexpr bool printItem(Item& item,Ctx& ctx) {return false;}
  template<typename Item> static constexpr bool printMenu(Item& item,Ctx& ctx) {return false;}
};

//deprecated, code size surges and we loose the ability to choose if we cnn the next chain module and when or whit what parameters
//still interesting
template<typename N>
struct OutLink:N {
  template<typename O>
  struct Part:N::template Part<O> {
    using Base=typename N::template Part<O>;
    using Base::Base;
    // template<Fmt tag> void fmtStart(const Ctx& ctx) {
    //   Base::template fmtStart<tag>(ctx);
    //   O::template fmtStart<tag>(ctx);
    // }
    // template<Fmt tag> void fmtStop(const Ctx& ctx) {
    //   O::template fmtStop<tag>(ctx);
    //   Base::template fmtStop<tag>(ctx);
    // }
  };
};

template<typename API,typename... OO> struct DefinedOut;

template<typename API,typename O,typename... OO>
struct DefinedOut<API,O,OO...>:APIOf<API,O,OO...>{//::template Map<OutLink>{
  using Base=APIOf<API,O,OO...>;//::template Map<OutLink>;
  using Base::printItem;
  using Base::obj;
  using Base::put;
  using Base::nl;
  static_assert(Base::template Excludes<IsCursor>::value||Base::template Requires<IsDataParser>::value,"Cursor requires preseeding DataParser<>");
  static_assert(Base::template Excludes<Class<class Clip>>::value||Base::template Requires<IsDataParser>::value,"Clip requires preseeding DataParser<>");
  static_assert(Base::template Excludes<Class<class TexWrap>>::value||Base::template Requires<IsDataParser>::value,"TextWrap requires preseeding DataParser<>");
  static_assert(Base::template Excludes<Class<class UTF8>>::value||Base::template Requires<IsDataParser>::value,"UTF8 requires preseeding DataParser<>");

  // template<typename T>
  // std::enable_if_t<!Base::HasPartialUpdate::value> put(T o) 
  //   {if(Base::lockMode()==LockMode::None) Base::put(o);}

  // template<typename T>
  // std::enable_if_t<!Base::HasPartialUpdate::value> put(T o) {Base::put(o);}

  // std::enable_if_t<!Base::HasPartialUpdate::value> nl() 
  //   {if(Base::lockMode()==LockMode::None) Base::nl();}

  template<typename Item>
  bool printTitle(const Item& item,Ctx& ctx){
    Base::fmtStart(Fmt::Title,ctx);
    item.print(obj(),ctx);
    Base::fmtStop(Fmt::Title,ctx);
    return item.changed();
  }
};

template<typename API>
struct DefinedOut<API>:APIOf<API>//OutLink<APIOf<API>>
  {using Base=APIOf<API>;};

template<typename... OO>
struct OutDef:DefinedOut<OutAPI<CRTP<OutDef<OO...>>>,OO...>{};

struct IOut {
  virtual void lockMode(LockMode)=0;
  virtual LockMode lockMode()=0;
  virtual void resume()=0;
  virtual void fmtStart(Fmt,const Ctx&)=0;
  virtual void fmtStop(Fmt,const Ctx&)=0;
  virtual Sz posX() const=0;
  virtual Sz posY() const=0;
  virtual void setPos(Pos)=0;
  virtual void put(const int)=0;
  virtual void put(const double)=0;
  virtual void put(const char)=0;
  virtual void put(const char*)=0;
  virtual void put(const char*,Sz)=0;
  virtual void put(const char* const*)=0;
  virtual void put(const char* const*& str)=0;

  template<Fmt tag> void fmtStart(const Ctx& ctx) {fmtStart(tag,ctx);}
  template<Fmt tag> void fmtStop(const Ctx& ctx) {fmtStart(tag,ctx);}
  Pos pos() const {return {posX(),posY()};}
  // virtual bool printItem(IItem& item,Ctx& ctx) {return false;}
  // virtual bool printMenu(IItem& item,Ctx& ctx) {return false;}
  // template<typename I> static constexpr bool printItem(I& item,Ctx& ctx) {return printItem(*reinterpret_cast<IItemDef<I>*>(&item),ctx);}
};

template<typename... OO>
struct IOutDef:IOut,DefinedOut<OutAPI<CRTP<IOutDef<OO...>>>,OO...>{
  using IOut::pos;
  using Base=DefinedOut<OutAPI<CRTP<IOutDef<OO...>>>,OO...>;
  virtual void lockMode(LockMode m) {Base::lockMode(m);}
  virtual LockMode lockMode() {return Base::lockMode();}
  virtual void resume() override {Base::resume();}
  using Base::fmtStart;
  using Base::fmtStop;
  virtual void fmtStart(Fmt tag,const Ctx& ctx) override {
    switch(tag){
      default: break;
      case Fmt::View: Base::template fmtStart<Fmt::View>(ctx);break;
      case Fmt::Title: Base::template fmtStart<Fmt::Title>(ctx);break;
      case Fmt::Menu: Base::template fmtStart<Fmt::Menu>(ctx);break;
      case Fmt::Body: Base::template fmtStart<Fmt::Body>(ctx);break;
      case Fmt::Item: Base::template fmtStart<Fmt::Item>(ctx);break;
      case Fmt::Index: Base::template fmtStart<Fmt::Index>(ctx);break;
      case Fmt::Accel: Base::template fmtStart<Fmt::Accel>(ctx);break;
      case Fmt::NavCursor: Base::template fmtStart<Fmt::NavCursor>(ctx);break;
      case Fmt::Field: Base::template fmtStart<Fmt::Field>(ctx);break;
      case Fmt::Label: Base::template fmtStart<Fmt::Label>(ctx);break;
      case Fmt::EditMode: Base::template fmtStart<Fmt::EditMode>(ctx);break;
      case Fmt::EditCursor: Base::template fmtStart<Fmt::EditCursor>(ctx);break;
      case Fmt::Data: Base::template fmtStart<Fmt::Data>(ctx);break;
      case Fmt::Unit: Base::template fmtStart<Fmt::Unit>(ctx);break;
    }
    // Base::template fmtStart<tag>(ctx);
  }
  virtual void fmtStop(Fmt tag,const Ctx& ctx) override {
    switch(tag){
      default: break;
      case Fmt::View: Base::template fmtStop<Fmt::View>(ctx);break;
      case Fmt::Title: Base::template fmtStop<Fmt::Title>(ctx);break;
      case Fmt::Menu: Base::template fmtStop<Fmt::Menu>(ctx);break;
      case Fmt::Body: Base::template fmtStop<Fmt::Body>(ctx);break;
      case Fmt::Item: Base::template fmtStop<Fmt::Item>(ctx);break;
      case Fmt::Index: Base::template fmtStop<Fmt::Index>(ctx);break;
      case Fmt::Accel: Base::template fmtStop<Fmt::Accel>(ctx);break;
      case Fmt::NavCursor: Base::template fmtStop<Fmt::NavCursor>(ctx);break;
      case Fmt::Field: Base::template fmtStop<Fmt::Field>(ctx);break;
      case Fmt::Label: Base::template fmtStop<Fmt::Label>(ctx);break;
      case Fmt::EditMode: Base::template fmtStop<Fmt::EditMode>(ctx);break;
      case Fmt::EditCursor: Base::template fmtStop<Fmt::EditCursor>(ctx);break;
      case Fmt::Data: Base::template fmtStop<Fmt::Data>(ctx);break;
      case Fmt::Unit: Base::template fmtStop<Fmt::Unit>(ctx);break;
    }
    // Base::template fmtStop<tag>(ctx);
  }
  virtual Sz posX() const override {return Base::posX();}
  virtual Sz posY() const override {return Base::posY();}
  virtual void setPos(Pos p) override {Base::setPos(p);}
  virtual void put(const int n) override {Base::put(n);}
  virtual void put(const double n) override {Base::put(n);}
  virtual void put(const char c) override {Base::put(c);}
  virtual void put(const char* str) override {Base::put(str);}
  virtual void put(const char* str,Sz n) override {Base::put(str,n);}
  virtual void put(const char* const* str) override {Base::put(str);}
  virtual void put(const char* const*& str) override {Base::put(str);}
  // virtual bool printItem(IItem& item,Ctx& ctx) override {return Base::printItem(item,ctx);}
  // virtual bool printMenu(IItem& item,Ctx& ctx) override {return Base::printMenu(item,ctx);}
  // template<typename I> static constexpr bool printItem(I& item,Ctx& ctx) {return printItem(*reinterpret_cast<IItemDef<I>*>(&item),ctx);}
};

//generic stream to outputs -------------------------------------
  template<typename... OO,typename T> 
  DefinedOut<OO...>& operator<<(DefinedOut<OO...>& out,const T& o) {out.put(o);return out;}

  template<template<typename...> class T,typename... NN,typename... OO> 
  DefinedOut<OO...>& operator<<(DefinedOut<OO...>& out,const T<NN...> o) {o.printItem(out);return out;}

  template<typename... OO> DefinedOut<OO...>& endl (DefinedOut<OO...>& s) {s.nl();return s;}

  template<typename... OO> DefinedOut<OO...>& flush(DefinedOut<OO...>& s) {s.flush();return s;}

  template<Sz x,Sz y,typename... OO> DefinedOut<OO...>& xy(DefinedOut<OO...>& s) {s.xy(x,y);return s;}

  template<size_t n,typename... OO> 
  DefinedOut<OO...>& operator<<(DefinedOut<OO...>& out,const char t[n]){for(int i=0;i<n;i++) out.put(t[i]);}

  template<typename... OO> 
  DefinedOut<OO...>& operator<<(DefinedOut<OO...>& out,DefinedOut<OO...>&(f)(DefinedOut<OO...>&))
    {f(out);return out;}

//internal components --

/// @brief signals this device has partial update capabilities
struct PartialDraw {
  template<typename O>
  struct Part:O {
    using HasPartialUpdate=std::true_type;
  };
};

struct Gate {
  template<typename O>
  struct Part:O {
    using IsParser=std::true_type;
    using HasGate=std::true_type;
    static_assert(O::template Excludes<IsDataParser>::value,"DataParser<> must preseed Gate");
    static_assert(O::template Excludes<IsFormat>::value,"formats must be above Gate");
    using Base=O;
    // using Base::lockMode;
    void nl() {if(unlocked()) Base::nl();}
    void clear() {if(unlocked()) Base::clear();}
    template<typename T>
    void put(const T o) {if(unlocked()) Base::put(o);}
    Pos measure() {
      lockMode(LockMode::Measure);
      return Base::getPos();
    }
    Area measure(Pos o) {
      lockMode(LockMode::Update);
      return {Base::posX()-o.x,Base::posY()-o.y};
    }
    bool unlocked() const {return lockMode()==LockMode::None;}
    bool updating() const {return lockMode()==LockMode::Update;}
    bool locked() const {return !unlocked();}
    LockMode lockMode() const {return m_lock_mode;}
    void lockMode(LockMode m) {m_lock_mode=m;}
    protected: LockMode m_lock_mode{LockMode::None};
  };
};

/// @brief use device cursor, must be placed on top of the device
/// will record the edit cursor position upon Fmt::TextEditCursor start
/// for ´ANSIEditFmt´ (or similar) to restore upon Fmt::Viewport stop
struct DeviceCursor {
  template<typename F>
  struct Part:F {
    static_assert(F::template Excludes<Class<class Gate>>::value,"Gate must be above DeviceCursor");
    using F::fmtStart;
    using F::fmtStop;
    template<Fmt tag>
    std::enable_if_t<tag&Fmt::Item> fmtStart(const Ctx& ctx) {
      F::template fmtStart<tag>(ctx);
      if(ctx && (!ctx.pad || (ctx.after()>1&&(ctx.sel(ctx.pAt) == ctx.pIdx)))) m_text_cursor_at=F::obj().pos();
    }
    template<Fmt tag>
    std::enable_if_t<tag&Fmt::EditCursor> fmtStart(const Ctx& ctx) {
      F::template fmtStop<tag>(ctx);
      m_editing=true;
      m_text_cursor_at=F::obj().pos();
    }
  protected:
    Pos m_text_cursor_at{0,0};
    bool m_editing{false};
  };
};

template<int w,int h>
struct StaticArea {
  template<typename O>
  struct Part:O {
    using IsArea=std::true_type;
    static constexpr Sz width() {return w;}
    static constexpr Sz height() {return h;}
    static constexpr Area area() {return {w,h};}
  };
};

template<int x,int y>
struct StaticPos {
  template<typename O>
  struct Part:O {
    using O::O;
    static constexpr Sz orgX() {return x;}
    static constexpr Sz orgY() {return y;}
    static constexpr Pos org() {return {orgX(),orgY()};}
  };
};

/// @brief provides raw access to the output device
struct Raw {
  template<typename O>
  struct Part:Gate::Part<O> {
    using RawDevice=std::true_type;
    using Base=typename Gate::Part<O>;
    static_assert(Base::template Excludes<IsFormat>::value,"formats must preseed the raw device");
    static_assert(Base::template Excludes<IsCursor>::value,"Cursor must preseed the raw device");
    static_assert(Base::template Excludes<IsPrinter>::value,"Printers must preseed the raw device");
    static_assert(Base::template Excludes<IsParser>::value,"Parsers must preseed the raw device");
    static_assert(Base::template Excludes<IsDataParser>::value,"DataParser<> must preseed the raw device");
    static void _nl() {Base::nl();}
    static void _flush() {Base::flush();}
    template<typename T> void _put(const T o) {Base::put(o);}
    void _setPos(Sz x,Sz y) {Base::setPos(x,y);}
    void _setPos(const Pos& o) {Base::setPos(o);}
    void padWith(Sz n,const char o=' ') {for(;n>0;n--) Base::obj().put(o);}
  };
};

// generic components ======================================================================================================================= --

/// @brief alternative printing, we need this to account for cursor movements
/// @tparam sz : intermediate buffer size ( make your choice ;)
template<Sz sz=16>
struct DataParser {
  template<typename O>
  struct Part:O {
    using IsDataParser=std::true_type;
    static_assert(O::template Excludes<IsFormat>::value,"formats must be above DataParser<>");
    using Base=O;
    void put(const char o) {Base::put(o);}
    void put(const char*o,Sz len) {for(Sz i=0;i<len&&o[i];i++) put((char)o[i]);}
    void put(const char o[]) {for(Sz i=0;o[i];i++) put((char)o[i]);}
    void put(const char* const* o) {for(Sz i=0;o[i];i++) put((*o)[i]);}
    #ifndef __AVR__
      template<typename P>
      void put(const P o,const char*fmt) {
        char buf[sz];
        std::snprintf(buf,sz,fmt,o);
        put(buf,sz);
      }
      void put(const char o,const char* fmt) {put<char>(o,fmt);}
      
      void put(const int o,const char* fmt="%i") {put<int>(o,fmt);}
      void put(const unsigned int o,const char* fmt="%u") {put<unsigned int>(o,fmt);}
      void put(const long o,const char* fmt="%li") {put<int>(o,fmt);}
      void put(const unsigned long o,const char* fmt="%lu") {put<unsigned int>(o,fmt);}
    #endif
    #ifdef ARDUINO
      // void put(const double o) {put(String(o, 5).c_str(),"%s");}
    #else
      void put(const double o,const char* fmt="%f") {put<double>(o,fmt);}
    #endif
  };
};

/// @brief support utf8 surrogates, only needed if using cursors and clipping
struct UTF8 {
  template<typename O>
  struct Part:O {
    using IsParser=std::true_type;
    // static_assert(O::Obj::template Requires<IsDataParser>,"DataParser<> must preseed UTF8");
    static_assert(O::template Excludes<IsFormat>::value,"formats must preseed UTF8");
    static_assert(O::template Excludes<IsBuffer>::value,"Buffer will not record UTF8");
    using Base=O;
    using This=Part<O>;
    /// @brief filter UTF8 surrogates, send surrogate codes to raw device shortcut, so that only one character is counted
    /// @param o : character or surrogate code
    inline void put(const char o) {
      if(m_raw) {
        m_raw--;
        Base::_put(o);
      } else {
        if(o>=(char)0xC0&&o<=(char)0xDF) m_raw=1;
        else if(o>=(char)0xE0&&o<=(char)0xEF) m_raw=2;
        else if (o>=(char)0xF0&&o<=(char)0xF7) m_raw=3;
        else m_raw=0;
        Base::put(o);
      }
    }
    protected:
      Sz m_raw{0};
  };
};

struct TextWrap {
  template<typename O>
  struct Part:O {
    using IsParser=std::true_type;
    // static_assert(O::template Excludes<IsDataParser>::value,"DataParser<> must preseed TextWrap");
    // static_assert(O::template Excludes<Class<UTF8>>::value,"UTF8 must preseed TextWrap");
    using Base=O;
    using This=Part<O>;
    inline void put(const char o) {
      if(Base::obj().free().x<=0) Base::nl();
      Base::put(o);
    }
  };
};

/// @brief clip output to defined area
/// this will require `DataParser` and possibly `UTF8` above 
/// and `Cursor` + `Gate` bellow
struct Clip {
  template<typename O>
  struct Part:O {
    using IsParser=std::true_type;
    // static_assert(O::template Excludes<IsDataParser>::value,"DataParser<> must preseed Clip");
    // static_assert(O::template Requires<IsCursor>::value,"Clip needs Cursor");
    // static_assert(O::template Requires<Class<Gate>>::value,"Clip needs Gate following");
    using Base=O;
    using This=Part<O>;
    using Base::put;
    inline void put(const char o) 
      {if(Base::freeX()>0&&Base::freeY()>0) Base::put(o);}
  };
};

template<typename Cor>
struct ColorTrack {
  template<typename O>
  struct Part:O {
    using Base=O;
  	void setColors(Cor f,Cor b) {m_fg=f;m_bg=b;Base::setColors(f,b);}
  	void setColors(const Colors<Cor>& o) {m_fg=o.fg;m_bg=o.bg;Base::setColors(o.fg,o.bg);}
    Colors<Cor> getColors() const {return {m_fg,m_bg};}
    void resume() {Base::setColors(m_fg,m_bg);Base::resume();}
    private:
      Cor m_fg;
      Cor m_bg;
  };
};

struct CtrlChars {
  template<typename O>
  struct Part:O {
    using Base=O;
    void put(const char o) {o=='\n'?Base::nl():Base::put(o);}
  };
};

struct Cursor {
  template<typename O>
  struct Part:O {
    // static_assert(O::Obj::template Requires<IsDataParser>::value,"DataParser<> must preseed UTF8");
    // static_assert(O::template Excludes<IsDataParser>::value,"DataParser<> must preseed Cursor");
    // static_assert(O::template Requires<IsArea>::value,"Cursor requires area information (StaticArea<>)");
    using IsCursor=std::true_type;
    using Base=O;
    using Base::obj;
    using Base::height;
    using Base::width;
    void clearLine() {Base::padWith(freeX());nl();}
    void clearFree() {do clearLine(); while(freeY());}
    Sz fieldWidth() const {return m_fieldWidth;}
    Pos pos() const {return m_at;}
    Sz posX() const {return m_at.x;}
    Sz posY() const {return m_at.y;}
    void setPos(Sz x,Sz y) {m_at.x=x;m_at.y=y;Base::setPos(x,y);}
    void setPos(const Pos& o) {setPos(o.x,o.y);}
    void resume() {Base::setPos(pos());Base::resume();}
    Pos area() const {return {fieldWidth(),m_at.y};}
    void clear() {
      m_at.x=0;
      m_at.y=0;
      m_fieldWidth=0;
      Base::clear();
    }
    void nl() {
      if(m_at.x>m_fieldWidth) m_fieldWidth=m_at.x;
      m_at.x=0;
      m_at.y++;
      Base::nl();
    }
    void put(const char o) {
      // if(o=='\n') nl();
      // else {
        m_at.x++;
        Base::put(o);
      // }
    }
    Sz freeX() const {return width()-posX();}
    Sz freeY() const {return height()-posY();}
    Area free() const {return {freeX(),freeY()};}
  protected: 
    Pos m_at{0,0};
    Sz m_fieldWidth;
  };
};

//panel buffer, can support cursor over serial
template<Scroll scrl=Scroll::yes,char c=' '>
struct Buffer {
  template<typename O>
  struct Part:O {
    using IsBuffer=std::true_type;
    // static_assert(O::template Requires<Class<Gate>>::value,"Buffer requires Gate");
    // static_assert(O::template Requires<IsCursor>::value,"Buffer requires Cursor");
    // static_assert(O::template Excludes<Class<Clip>>::value,"Clip must preseed Buffer");
    // static_assert(O::template Excludes<Class<TextWrap>>::value,"TextWrap must preseed Buffer");
    using Base=O;
    using Base::width;
    using Base::height;
    using Base::posY;
    using Base::posX;
    using Base::freeX;
    // using Base::freeY;
    using Base::obj;
    Part() {Base::lockMode(LockMode::Measure);}
    void erase() {
      memset(buffer,c,height()*width());
      Base::clear();
    }
    void scroll() {
      memmove(buffer,&buffer[width()],(height()-1)*width());
      memset(&buffer[(height()-1)*width()],c,width());
      Base::obj().setPos({0,height()-1});
    }
    Sz freeY() const {
      return scrl==Scroll::no?
        Base::freeY():
        Base::freeY()>0?
          Base::freeY():
          1-Base::freeY();
    }
    Area free() const {return {freeX(),freeY()};}
    void put(char o) {
      m_changed=true;
      if(scrl==Scroll::yes) 
        while(Base::freeY()<=0) scroll();
      buffer[posY()*width()+posX()]=o;
      Base::put(o);
    }
    bool changed() const {return m_changed;}
    void sync() {m_changed=false;}

    void print() {
      Base::lockMode(LockMode::None);
      Base::clear();
      Base::resume();
      Sz at=0;
      for(Sz y=0;y<height();y++) {
        for(Sz x=0;x<width();x++,at++) Base::put(buffer[at]);
        Base::nl();
      }
      Base::lockMode(LockMode::Measure);
    }
    protected:
      bool m_changed{true};
      char buffer[width()*height()]{0};
  };
};

