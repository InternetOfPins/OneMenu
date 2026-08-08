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

  /// @brief Zero-copy binding for TextField/EDIT() over a caller-owned `char buf[]` (bound, not copied).
  /// @tparam address the array name itself (not `&buf`); must have static storage duration and linkage
  /// @tparam sz usable buffer length, excluding the trailing '\0'
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

      // Real set(const char*) override, not a bare `using Base::set;` — an external
      // caller (web/async value injection, or a menu action writing a presentation-
      // only field directly, e.g. a touch keypad) doing a whole-string write via this
      // needs edited=true too, exactly like a real nav-driven edit already gets via
      // the char-insert paths below. Without this, `using Base::set;` reached
      // Storage's own set() directly, which knows nothing about `edited` at all —
      // every such write was silently invisible to changed()/redraw unless the
      // caller remembered to flip edited itself right after.
      void set(const char* s) noexcept { edited=true; Base::set(s); }

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
      // XmlFmt-only: tag this item's own <item> tag with dropdown="1"
      // BEFORE Base::printItem (RecallNavPos<false>'s own <opt> list emission)
      // runs — same "constant attribute, emitted while the tag's own
      // attribute window is still open" shape as RecallNavPos's own Choice
      // marker. Toggle and Select both compose RecallNavPos<false> and emit
      // an identical <opt> list otherwise; this is the only thing telling a
      // web client "render me as an <select><option> dropdown" apart from
      // Toggle's own row-of-pills (Rui's own request, 2026-07-22).
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        if constexpr(hapi::query<IsXmlFmt,typename Out::Types>) {
          out.template fmtStart<Fmt::Dropdown>(ctx);
          out.template fmtStop<Fmt::Dropdown>(ctx);
        }
        Base::printItem(out,ctx);
      }
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
    Menu<T,B,IsChoiceBody,OO...>
  >;

  // Removed (2026-07-29) the forced-front AsEditMode<> this used to carry
  // (ItemDef<AsEditMode<>,T,EditField,O,OO...>). That placement existed
  // solely for XmlFmt: AsEditMode/AsIndex/etc are attribute-only Fmt tags
  // (XmlFmt's attr_tags) that must fire while the enclosing <item> tag is
  // still open — T (AsLabel<...>) opens+closes its own <lbl> child first,
  // force-closing <item>'s tag in the process, so AsEditMode<> after T
  // rendered its mode="..." attribute as malformed loose text after </lbl>
  // instead of a real <item> attribute (found 2026-07-22 on real ESP32
  // hardware). Deliberately reverted: ANSIFmt (and now GfxColorFmt) both
  // render a REAL visible glyph for every Fmt::EditMode firing, and the
  // forced-front instance was indistinguishable from a real, deliberately-
  // composed one placed by the item itself (e.g. nested inside AsLabel<> to
  // land at the label/field boundary) — every NumField unconditionally
  // showed a redundant marker at item start once a second, real one existed.
  // Known, accepted regression: NumField over XmlFmt now needs its own
  // explicit AsEditMode<> placed correctly (before T) if that attribute is
  // wanted there — not automatic anymore.
  template<typename T,typename O,typename... OO>
  using NumFieldDef
    =ItemDef<T,EditField,O,OO...>;

  // TextField<N, Mask> is self-contained (storage inside the field),
  // so TextFieldDef needs no DataRef/Watch layer — just title + size + mask.
  // TextField::Part is PadDraw (a ParentDraw subtype) so no extra ParentDraw needed.
  template<typename T, Sz N, typename Mask = CharMask::ASCII7>
  using TextFieldDef
    =ItemDef<T,AsEditMode<>,EditField,TextField<N,Mask>>;
};