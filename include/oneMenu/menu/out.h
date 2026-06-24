/**
 * @file out.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief output API
 * @version 5
*/

#pragma once

#include <hapi/hapi.h>
#include <oneOutput/oneOutput.h>
#include <oneMenu/menu/sys/base.h>

namespace oneMenu {

  /// @brief extending OneOutput API
  /// @tparam Cfg 
  template<typename Cfg=hapi::Nil>
  struct OutAPI:oneOutput::OutAPI<Cfg> {
    using Base=oneOutput::OutAPI<Cfg>;
    template<Fmt tag> static constexpr void fmtStart(const Ctx& ctx) {}
    template<Fmt tag> static constexpr void fmtStop(const Ctx& ctx) {}
    template<typename Item> static constexpr bool printItem(Item& item,Ctx& ctx) {return false;}
    template<typename Item> static constexpr bool printMenu(Item& item,Ctx& ctx) {return false;}
    static constexpr Pos pos() {return {0,0};}
    static constexpr void setPos(const Pos&) {}  // terminal: absorbs oneMenu::Pos before reaching oneOutput::OutAPI
    static constexpr void resume() {}
    using Base::put;
    static constexpr void put(const char*,Sz) {}
  };

  // template<typename API,typename... OO> struct OutImpl;

  template<typename API,typename... OO>
  struct OutImpl:APIOf<API,OO...>{
    using Base=APIOf<API,OO...>;
    using Base::printItem;
    using Base::obj;
    using Base::put;
    using Base::nl;

    template<typename Item>
    bool printTitle(const Item& item,Ctx& ctx){
      Base::fmtStart(Fmt::Title,ctx);
      item.print(obj(),ctx);
      Base::fmtStop(Fmt::Title,ctx);
      return item.changed();
    }
  };

  // template<typename API>
  // struct OutImpl<API>:APIOf<API> {using Base=APIOf<API>;};

  /// @brief chain head injected by OutDef — propagates resume() down to Gate/Cursor/ColorTrack
  struct Resume {
    template<typename O>
    struct Part:O {
      using Base=O;
      void resume() {Base::resume();}
    };
  };

  /// @brief compose a complete output chain (printer + format + parsers + cursor + device + geometry)
  template<typename... OO>
  struct OutDef:OutImpl<OutAPI<hapi::CRTP<OutDef<OO...>>>,Resume,OO...>{};

  /// @brief output interface base for runtime-polymorphic output dispatch
  struct IOut {
    virtual void lockMode(LockMode)=0;
    virtual LockMode lockMode()=0;
    virtual void resume()=0;
    virtual void fmtStart(Fmt,const Ctx&)=0;
    virtual void fmtStop(Fmt,const Ctx&)=0;
    virtual void setPos(const Pos&)=0;
    virtual void put(const int)=0;
    virtual void put(const double)=0;
    virtual void put(const char)=0;
    virtual void put(const char*)=0;
    virtual void put(const char*,Sz)=0;
    virtual void put(const char* const*)=0;
    virtual void put(const char* const*& str)=0;

    template<Fmt tag> void fmtStart(const Ctx& ctx) {fmtStart(tag,ctx);}
    template<Fmt tag> void fmtStop(const Ctx& ctx) {fmtStop(tag,ctx);}
  };

  /// @brief OutDef variant with virtual dispatch for runtime-polymorphic output
  template<typename... OO>
  struct IOutDef:IOut,OutImpl<OutAPI<hapi::CRTP<IOutDef<OO...>>>,Resume,OO...>{
    using Base=OutImpl<OutAPI<hapi::CRTP<IOutDef<OO...>>>,Resume,OO...>;
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
        // case Fmt::Footer: Base::template fmtStart<Fmt::Footer>(ctx);break;
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
        // case Fmt::Footer: Base::template fmtStop<Fmt::Footer>(ctx);break;
      }
      // Base::template fmtStop<tag>(ctx);
    }
    // virtual Sz posX() const override {return Base::posX();}
    // virtual Sz posY() const override {return Base::posY();}
    virtual void setPos(const Pos& p) override {Base::setPos(p);}
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
    OutImpl<OO...>& operator<<(OutImpl<OO...>& out,const T& o) {out.put(o);return out;}

    template<template<typename...> class T,typename... NN,typename... OO> 
    OutImpl<OO...>& operator<<(OutImpl<OO...>& out,const T<NN...> o) {o.printItem(out);return out;}

    template<typename... OO> OutImpl<OO...>& endl (OutImpl<OO...>& s) {s.nl();return s;}

    template<typename... OO> OutImpl<OO...>& flush(OutImpl<OO...>& s) {s.flush();return s;}

    template<Sz x,Sz y,typename... OO> OutImpl<OO...>& xy(OutImpl<OO...>& s) {s.xy(x,y);return s;}

    template<size_t n,typename... OO> 
    OutImpl<OO...>& operator<<(OutImpl<OO...>& out,const char t[n]){for(int i=0;i<n;i++) out.put(t[i]);}

    template<typename... OO> 
    OutImpl<OO...>& operator<<(OutImpl<OO...>& out,OutImpl<OO...>&(f)(OutImpl<OO...>&))
      {f(out);return out;}

  //internal components --

  /// @brief signals this device has partial update capabilities
  struct PartialDraw {
    template<typename O>
    struct Part:O {
      using HasPartialUpdate=std::true_type;
    };
  };

  /// @brief lock-mode gate: controls when output is allowed (None/Update/Sync/Measure/Changed)
  struct Gate : aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser, After>, "Gate: DataParser<sz> must be placed above Gate — character parsing must happen before gating");
      static_assert(Excludes<IsFormat, After>,     "Gate: format layers must be placed above Gate — formatting must happen before gating");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsParser=std::true_type;
      using HasGate=std::true_type;
      using Base=O;
      // using Base::lockMode;
      void nl() {if(unlocked()) Base::nl();}
      void clear() {if(unlocked()) Base::clear();}
      template<typename T>
      void put(const T o) {if(unlocked()) Base::put(o);}
      void put(const char* s,Sz n) {if(unlocked()) Base::put(s,n);}
      void setPos(const Pos& p) {if(unlocked()) Base::setPos(p);}
      template<typename T>
      void _put(T c) {if(unlocked()) Base::_put(c);}
      template<typename... Args>
      void fillRect(Args... args) {if(unlocked()) Base::fillRect(args...);}
      template<typename... Args>
      void drawRoundRect(Args... args) {if(unlocked()) Base::drawRoundRect(args...);}
      void setInverted(bool v) {if(unlocked()) Base::setInverted(v);}
      Pos measure() {
        lockMode(LockMode::Measure);
        return Base::pos();
      }
      Area measure(Pos o) {
        lockMode(LockMode::Update);
        return {Base::posX()-o.x,Base::posY()-o.y};
      }
      void resume() {m_lock_mode=LockMode::None;Base::resume();}
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
      using F::fmtStart;
      using F::fmtStop;
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item> fmtStart(const Ctx& ctx) {
        if(ctx && (!ctx.pad || (ctx.after()>1&&(ctx.sel(ctx.pAt) == ctx.pIdx)))) m_text_cursor_at=F::obj().getPos();
        F::template fmtStart<tag>(ctx);
      }
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::EditCursor> fmtStart(const Ctx& ctx) {
        m_text_cursor_at=F::obj().getPos();
        m_editing=true;
        F::template fmtStart<tag>(ctx);
      }
    protected:
      Pos m_text_cursor_at{0,0};
      bool m_editing{false};
    };
  };

  /// @brief compile-time fixed output area (width × height in device units)
  template<int w,int h>
  struct StaticArea : anArea {
    static_assert(w > 0, "StaticArea<w,h>: width must be positive");
    static_assert(h > 0, "StaticArea<w,h>: height must be positive");
    template<typename O>
    struct Part:O {
      using IsArea=std::true_type;
      static constexpr Sz width() {return w;}
      static constexpr Sz height() {return h;}
      static constexpr Area area() {return {w,h};}
    };
  };

  /// @brief compile-time fixed origin position in device coordinates
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
  struct Raw : aRawDevice, aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsFormat,     After>, "Raw: format layers must be placed above the raw device");
      static_assert(Excludes<IsCursor,     After>, "Raw: Cursor must be placed above the raw device");
      static_assert(Excludes<IsPrinter,    After>, "Raw: printer layers must be placed above the raw device");
      static_assert(Excludes<IsParser,     After>, "Raw: parser layers (Gate/UTF8/etc.) must be placed above the raw device");
      static_assert(Excludes<IsDataParser, After>, "Raw: DataParser<sz> must be placed above the raw device");
      return true;
    }
    template<typename O>
    struct Part:Gate::Part<O> {
      using RawDevice=std::true_type;
      using Base=typename Gate::Part<O>;
      static void _nl() {Base::nl();}
      // static void _flush() {Base::flush();}//not locked
      template<typename T> void _put(const T o) {Base::put(o);}
      // void _setPos(Sz x,Sz y) {Base::setPos(x,y);}//old stuff
      // void _setPos(const Pos& o) {Base::setPos(o);}
      void padWith(Sz n,const char o=' ') {for(;n>0;n--) Base::obj().put(o);}
    };
  };

  // generic components ======================================================================================================================= --

  /// @brief alternative printing, we need this to account for cursor movements
  /// @tparam sz : intermediate buffer size ( make your choice ;)
  template<Sz sz=16>
  struct DataParser : aDataParser, aParser {
    static_assert(sz > 0, "DataParser<sz>: buffer size must be positive");
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsFormat, After>, "DataParser<sz>: format layers must be placed above DataParser<> — formatting occurs before character decomposition");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsDataParser=std::true_type;
      using Base=O;
      void put(const char o) {Base::put(o);}
      void put(const char*o,Sz len) {for(Sz i=0;i<len&&o[i];i++) put((char)o[i]);}
      void put(const char o[]) {for(Sz i=0;o[i];i++) put((char)o[i]);}
      void put(const char* const* o) {for(Sz i=0;o[i];i++) put((*o)[i]);}
      #ifndef __AVR__
        template<typename P>
        void put(const P o,const char*fmt) {
          static_assert(std::is_arithmetic_v<P>,
            "DataParser::put<P>: P must be arithmetic — verify each typed overload calls put<P> with its own type, not a narrower one");
          char buf[sz];
          std::snprintf(buf,sz,fmt,o);
          put(buf,sz);
        }
        void put(const char o,const char* fmt) {put<char>(o,fmt);}

        void put(const int o,const char* fmt="%i") {put<int>(o,fmt);}
        void put(const unsigned int o,const char* fmt="%u") {put<unsigned int>(o,fmt);}
        void put(const long o,const char* fmt="%li") {put<long>(o,fmt);}
        void put(const unsigned long o,const char* fmt="%lu") {put<unsigned long>(o,fmt);}
      #endif
      #ifdef ARDUINO
        // void put(const double o) {put(String(o, 5).c_str(),"%s");}
      #else
        void put(const double o,const char* fmt="%f") {put<double>(o,fmt);}
      #endif
    };
  };

  /// @brief support utf8 surrogates, only needed if using cursors and clipping
  struct UTF8 : aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser, After>, "UTF8: DataParser<sz> must be placed above UTF8 — DataParser feeds the raw pipeline that UTF8 bypasses for surrogates");
      static_assert(Excludes<IsFormat,     After>, "UTF8: format layers must be placed above UTF8 in the OutDef chain");
      static_assert(Excludes<IsBuffer,     After>, "UTF8: Buffer cannot be placed below UTF8 — Buffer would miss surrogate continuation bytes");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsParser=std::true_type;
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

  /// @brief wraps long text at word boundaries within the current area
  struct TextWrap : aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser,                     After>, "TextWrap: DataParser<sz> must be placed above TextWrap");
      static_assert(Excludes<hapi::SameAs<UTF8>,               After>, "TextWrap: UTF8 must be placed above TextWrap — wrapping must count characters after surrogate filtering");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsParser=std::true_type;
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
  struct Clip : aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser,      After>, "Clip: DataParser<sz> must be placed above Clip");
      static_assert(Requires<IsCursor,          After>, "Clip: Cursor must be placed below Clip — clipping needs tracked position");
      static_assert(Requires<hapi::SameAs<Gate>,After>, "Clip: Gate must be placed below Clip — clipping relies on Gate's lock mode");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsParser=std::true_type;
      using Base=O;
      using This=Part<O>;
      using Base::put;
      inline void put(const char o) 
        {if(Base::free().x>0&&Base::free().y>0) Base::put(o);}
    };
  };

  /// @brief tracks current foreground/background color; provides setColors()
  template<typename Cor>
  struct ColorTrack {
    template<typename O>
    struct Part:O {
      using Base=O;
      void setColors(Cor f,Cor b) {m_fg=f;m_bg=b;m_set=true;Base::setColors(f,b);}
      void setColors(const Colors<Cor>& o) {m_fg=o.fg;m_bg=o.bg;m_set=true;Base::setColors(o.fg,o.bg);}
      Colors<Cor> getColors() const {return {m_fg,m_bg};}
      void resume() {Base::resume();if(m_set) Base::setColors(m_fg,m_bg);}
      private:
        Cor m_fg{};
        Cor m_bg{};
        bool m_set{false};
    };
  };

  /// @brief intercepts control characters (\n, \r, \t) before they reach the device
  struct CtrlChars {
    template<typename O>
    struct Part:O {
      using Base=O;
      void put(const char o) {o=='\n'?Base::nl():Base::put(o);}
    };
  };

  /// @brief tracks cursor position (x,y) in device units; provides getPos/setPos/clearToEOL/clearFree
  // CharW: advance per character in device units (1=char-based, 6=font5x8 pixels, etc.)
  // LineH: advance per line in device units (1=char-based, 8=font5x8 pixels, etc.)
  // Adv:   optional per-glyph advance fn (char→Sz) for proportional/variable-width fonts.
  //        When non-null, CharW is ignored for x-advance. clearLine uses CharW as fallback
  //        for fill-width estimation when Adv is set (space advance should be known).
  using AdvFn = Sz(*)(char);
  template<Sz CharW=1, Sz LineH=1, AdvFn Adv=nullptr>
  struct Cursor : aCursor {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser, After>, "Cursor: DataParser<sz> must be placed above Cursor — position tracking only works after character parsing");
      static_assert(Requires<IsArea, After>, "Cursor: StaticArea<w,h> must be placed below Cursor — area dimensions required for boundary tracking");
      return true;
    }
    template<typename O>
    struct Part:PartialDraw::template Part<O> {
      using IsCursor=std::true_type;
      using Base=typename PartialDraw::template Part<O>;
      using Base::obj;
      using Base::height;
      using Base::width;
      static constexpr Sz glyphWidth(char c) {
        if constexpr(Adv != nullptr) return Adv(c);
        else return CharW;
      }
      // clearToEOL: pad remainder of current row with spaces (no newline)
      // clearLine:  clearToEOL + nl — for clearing whole blank rows
      void clearToEOL() {Base::padWith(free().x/glyphWidth(' '));}
      void clearLine()  {clearToEOL(); nl();}
      void clearFree()  {while(free().y>0) clearLine();}
      Sz fieldWidth() const {return m_fieldWidth;}
      // Pos pos() const {return m_at;}
      Pos getPos() const {return m_at;}
      void setPos(const Pos& o) {m_at.x=o.x;m_at.y=o.y;Base::setPos(o);}
      void resume() {Base::resume();Base::setPos(m_at);}
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
        m_at.y+=LineH;
        Base::nl();
      }
      void put(const char o) {
        m_at.x+=glyphWidth(o);
        Base::put(o);
      }
      template<typename T>
      void put(const T) {
        static_assert(std::is_same_v<T,void>,
          "Cursor::put<T>: non-char put reached Cursor — DataParser must be placed above Cursor to convert all types to chars first");
      }
      Area free() const {return {width()-m_at.x, height()-m_at.y};}
    protected:
      Pos m_at{0,0};
      Sz m_fieldWidth{0};
    };
  };

  /// @brief intermediate character buffer; flushes to device on nl() or flush()
  //panel buffer, can support cursor over serial
  template<Scroll scrl=Scroll::yes,char c=' '>
  struct Buffer : aBuffer {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<hapi::SameAs<Gate>,    After>, "Buffer: Gate must be placed below Buffer — Buffer uses Gate's lock mode to control reprinting");
      static_assert(Requires<IsCursor,              After>, "Buffer: Cursor must be placed below Buffer — Buffer indexes its storage via cursor position");
      static_assert(Excludes<hapi::SameAs<Clip>,    After>, "Buffer: Clip must be placed above Buffer — clipping should happen before buffering");
      static_assert(Excludes<hapi::SameAs<TextWrap>,After>, "Buffer: TextWrap must be placed above Buffer — wrapping should happen before buffering");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsBuffer=std::true_type;
      using Base=O;
      using Base::width;
      using Base::height;
      // using Base::posY;
      // using Base::posX;
      // using Base::freeX;
      // using Base::freeY;
      using Base::free;
      using Base::pos;
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
      Area free() const {
        return {
          Base::free().x,
          scrl==Scroll::no?
            Base::free().y:
            Base::free().y>0?
              Base::free().y:
              1-Base::free().y
          };
        }
      void put(char o) {
        m_changed=true;
        if(scrl==Scroll::yes) 
          while(Base::free().y<=0) scroll();
        buffer[Base::m_at.y*width()+Base::m_at.x]=o;
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
};//oneMenu
