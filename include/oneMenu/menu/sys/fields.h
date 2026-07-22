#pragma once

#include "oneMenu/menu/menu.h"
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/sys/printers.h"
#include "oneMenu/menu/item.h"
#include "oneMenu/menu/sys/charMask.h"
#include <oneData/oneData.h>

namespace oneMenu {
  // Detects "Mask::chk/up/down take (pos,char)" — CharMask::PerPos/PosSet's
  // shape — vs "take plain char" — CharMask::Range/Set/Ranges's existing,
  // unchanged shape. std::void_t/declval, same idiom as am4compat::
  // IsPlainEventFn/IsEventFn (compat/am4.h) — avr-gcc has no <type_traits>,
  // HAPI's avr_std.h shim (pulled in transitively) covers void_t/declval
  // there too. Backs TextField::PartEnd::nav()'s auto-dispatch below between
  // the two Mask shapes — zero caller-facing flag; every existing
  // uniform-mask field (e.g. examples/fields's TextField<15>, default
  // CharMask::ASCII7) keeps compiling and behaving byte-for-byte unchanged.
  template<typename M,typename=void>
  struct IsPositionalMask : std::false_type {};
  template<typename M>
  struct IsPositionalMask<M,std::void_t<decltype(
      M::chk(std::declval<int>(),std::declval<char>()))>>
    : std::true_type {};

  /// @brief zero-copy external char-buffer binding for TextField/EDIT() —
  /// binds directly over a caller-owned `char buf[]`, matching AM4's real
  /// EDIT() semantics (bound-not-copied, in place; "field will initialize
  /// its size by this string length"). This is NOT oneData::DataRef: DataRef
  /// (oneData.h) already special-cases get() correctly for a char* NTTP
  /// (returns the pointer itself, not *address) but its set() stays the
  /// generic scalar set(Type v)=set(char v) (writes one char) inherited from
  /// DataRef's size-less, shared "0 bytes RAM" contract — wrong for
  /// TextField::PartEnd::setStr()'s "set(const char*), whole-string" need,
  /// and DataRef has no capacity to bound a copy against even if it grew
  /// that overload. TextBufRef is the small, TextField-only adapter that
  /// adds exactly that one missing thing (a compile-time size), reusing
  /// Data<char[N]>::set()'s own bounded-copy shape against an external
  /// pointer instead of an owned array.
  /// @tparam address the buffer's own array-decayed pointer — pass the array
  ///   name itself (e.g. `TextBufRef<buf,sizeof(buf)-1>`), NOT `&buf` (whose
  ///   type is char(*)[N], not char*); buf must have static storage duration
  ///   and linkage (same pre-existing constraint DataRef<&(var)>-based
  ///   FIELD() bindings already have).
  /// @tparam sz usable buffer length, excluding the trailing '\0' — matches
  ///   AM4's "field size = the buffer's own string length" rule.
  template<char* address,Sz sz>
  struct TextBufRef {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static char* get() noexcept {return address;}

      static void set(const char* s) noexcept {
        strncpy(address,s,sz);
        address[sz]='\0';
      }
    };
  };

  template<Sz sz,typename Mask=CharMask::ASCII7,
           typename Storage=oneData::Data<char[sz+1]>>
  struct TextField {
    template<typename I>
    struct PartEnd : Storage::template Part<I> {
      using Base    = typename Storage::template Part<I>;
      using Base::Base;
      using Base::get;
      using Base::set;   // set(const char*) — for web / async value injection
      using Base::sync;  // keep ItemAPI's inherited sync(Out&) template
                         // reachable — this Part's own sync() (0-arg) would
                         // otherwise hide it via ordinary C++ name hiding,
                         // breaking IItem's virtual sync(IOut&) override
                         // (item.h) for any chain built through IItemDef.

      template<typename Nav,typename P>
      bool setStr(Nav&,const char* s,P p) {
        if(p.len==0) { set(s); return true; }
        return false;
      }

      char chk{0};
      bool edited{false};

      static constexpr Sz size() {return sz;}
      static constexpr Sz depth() {return 2;}
      bool changed() const {return edited;}
      void sync() {edited=false;}

      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        const char* text = get();
        Sz i=ctx.sel();
        if (ctx) {
          out.put(&text[0],i);
          out.template fmtStart<Fmt::EditCursor>(ctx);
          if(text[i]) out.put(text[i]);
          out.template fmtStop<Fmt::EditCursor>(ctx);
          out.put(&text[i+1]);
        } else out.put(text,sz);
        I::printItem(out,ctx);  // skip Data<char[]>::printItem — PartEnd owns all output
      }

      template<bool isKbd,typename Nav>
      std::enable_if_t<!isKbd,bool> nav(Nav& n,const CKE& cke,const Path& path) {
        if(n.navMode()==NavMode::Edit) switch(cke.cmd) {
          case Cmd::Left:  return n.doNav({Cmd::Up},  std::min(sz,ss()+1),false);
          case Cmd::Right: return n.doNav({Cmd::Down}, std::min(sz,ss()+1),false);
          case Cmd::Down: {   // ↑ key → cycle char up through mask
            Sz pos=path.sel();
            if(pos>=sz-1) return true;
            char* text=get();
            if(!text[pos]) text[pos+1]='\0';
            text[pos]=maskUp(pos,text[pos]);
            edited=true;
            return true;
          }
          case Cmd::Up: {     // ↓ key → cycle char down through mask
            Sz pos=path.sel();
            if(pos>=sz-1) return true;
            char* text=get();
            if(!text[pos]) text[pos+1]='\0';
            text[pos]=maskDown(pos,text[pos]);
            edited=true;
            return true;
          }
          default: break;
        }
        return Base::template nav<isKbd>(n,cke,path);
      }

      template<bool isKbd,typename Nav>
      std::enable_if_t<isKbd,bool> nav(Nav& n,const CKE& cke,const Path& path) {
        char* text = get();
        if(n.navMode()==NavMode::Edit) {
          if(cke.cmd==Cmd::Key) {
            if(cke.key==8||cke.key==127) {//backspace
              if(path.sel()>0) for(int k=path.sel();k<=sz;k++) text[k-1]=text[k];
              edited=true;
              return n.doNav({Cmd::Down},ss(),false);
            } else if(cke.ext) {//extended keys
              if(cke.key==0x33) for(int k=path.sel();k<sz;k++) text[k]=text[k+1];//delete
              else if(cke.key==0x48) n.go(0);//home
              else if(cke.key==0x46) n.go(ss());//end
              else return true;
              edited=true;
              return true;
            } else if(maskChk(path.sel(),cke.key)) {//write char
              for(int k=sz-1;k>path.sel();k--) text[k]=text[k-1];
              text[path.sel()]=cke.key;
              edited=true;
              return n.doNav({Cmd::Up},ss()+1,false);
            }
          }
          return n.doNav(cke,ss()+1,false);
        }
        return Base::template nav<isKbd>(n,cke,path);
      }
      protected: Sz ss() const {return strnlen(get(),sz-1);}

      // Auto-dispatch helpers — IsPositionalMask<Mask> picks the 2-arg
      // (pos,char) PerPos/PosSet path or the original 1-arg char path, at
      // compile time. `pos` is only ever meaningful for the positional
      // shape; the uniform shape ignores it (same call, compiler picks —
      // matches am4.h's own "auto-dispatch, not a caller-facing flag"
      // convention).
      static char maskUp(Sz pos,char c) {
        if constexpr (IsPositionalMask<Mask>::value) return Mask::up(pos,c);
        else return Mask::up(c);
      }
      static char maskDown(Sz pos,char c) {
        if constexpr (IsPositionalMask<Mask>::value) return Mask::down(pos,c);
        else return Mask::down(c);
      }
      static bool maskChk(Sz pos,char c) {
        if constexpr (IsPositionalMask<Mask>::value) return Mask::chk(pos,c);
        else return Mask::chk(c);
      }
    };
    template<typename I> using Part=PadDraw::template Part<PartEnd<I>>;
  };

  /// @brief toggle enumerated field values on enter key
  /// is implicitly: a RecallDraw and Recall
  struct ToggleBehave {
    template<typename I>
    struct Part:RecallNavPos<false>::template Part<I> {
      using Base=typename RecallNavPos<false>::template Part<I>;
      using Base::Base;
      using Base::sync;  // keep ItemAPI's inherited sync(Out&) template
                         // reachable — this Part's own sync() (0-arg) would
                         // otherwise hide it via ordinary C++ name hiding,
                         // breaking IItem's virtual sync(IOut&) override
                         // (item.h) for any chain built through IItemDef.
      template<typename... OO> Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      bool changed() const {return m_changed;}
      bool sync() {return m_changed=false;Base::sync();}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        I::template nav<isKbd>(n,cke,path);
        if(cke.cmd==Cmd::Enter) {
          n.go(Base::m_sel);
          n.doNav({Cmd::Up},Base::Body::size(),Base::wraps());
          m_changed=Base::m_sel!=n.sel();
          Base::m_sel=n.sel();
          n.close();
        }
        return changed();
      }
      protected: bool m_changed{true};
    };
  };

  template<typename T,typename B,typename... OO>
  using ToggleFieldDef=ItemDef<
    ToggleBehave,
    Menu<T,B,ParentDraw,WrapNav,OO...>
  >;

  // SelectBehave: like RecallNavPos but calls n.padOpen() instead of delegating to
  // Menu::nav on root Enter, so the sub-list stays at the parent display level.
  // ParentDraw (not PadDraw) in Menu<> prevents body.printInline from showing all items.
  struct SelectBehave {
    template<typename I>
    struct Part:RecallNavPos<false>::template Part<I> {
      using Base=typename RecallNavPos<false>::template Part<I>;
      using Base::Base;
      template<typename... OO> Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        if(cke.cmd==Cmd::Enter) {
          if(path.len) {
            Base::m_sel=path.sel();
            return I::template nav<isKbd>(n,cke,path);
          } else {
            n.padOpen();
            n.go(Base::m_sel);
            return true;
          }
        }
        return I::template nav<isKbd>(n,cke,path);
      }
    };
  };

  template<typename T,typename B,typename... OO>
  using SelectFieldDef=ItemDef<
    SelectBehave,
    Menu<T,B,EditField,ParentDraw,OO...>
  >;

  template<typename T,typename B,typename... OO>
  using ChooseFieldDef=ItemDef<
    RecallNavPos<>,
    Menu<T,B,OO...>
  >;

  // AsEditMode<> comes BEFORE T (the label component): AsEditMode/AsIndex/etc
  // are attribute-only Fmt tags (XmlFmt's attr_tags) that must fire while the
  // enclosing <item> tag is still open — T (AsLabel<...>) opens+closes its
  // own <lbl> child first, which force-closes <item>'s own tag in the
  // process (XmlFmt::fmtStart closes any pending open tag before printing
  // inline content) — putting AsEditMode<> after T meant its mode="..."
  // attribute fired too late, landing as malformed loose text after </lbl>
  // instead of as a real <item> attribute (found 2026-07-22 rendering a real
  // NumField over XmlFmt on real ESP32 hardware).
  template<typename T,typename O,typename... OO>
  using NumFieldDef
    =ItemDef<AsEditMode<>,T,EditField,O,OO...>;

  // TextField<N, Mask> is self-contained (storage inside the field),
  // so TextFieldDef needs no DataRef/Watch layer — just title + size + mask.
  // TextField::Part is PadDraw (a ParentDraw subtype) so no extra ParentDraw needed.
  template<typename T, Sz N, typename Mask = CharMask::ASCII7>
  using TextFieldDef
    =ItemDef<T,AsEditMode<>,EditField,TextField<N,Mask>>;
};