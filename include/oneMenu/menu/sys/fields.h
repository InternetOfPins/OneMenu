#pragma once

#include "oneMenu/menu/menu.h"
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/sys/printers.h"
#include "oneMenu/menu/item.h"
#include "oneMenu/menu/sys/charMask.h"

namespace oneMenu {
  template<Sz sz,typename Mask=CharMask::ASCII7>
  struct TextField {
    template<typename I>
    struct PartEnd:I {
      using Base=I;
      char text[sz+1]{"Rui Azevedo"};
      char chk{0};
      bool edited{false};
      // constexpr Part():text{0} {}
      static constexpr Sz size() {return sz;}
      static constexpr Sz depth() {return 2;}
      bool changed() const {return edited;}
      void sync() {edited=false;}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        Sz i=ctx.sel();
        if (ctx) {
          out.put(&text[0],i);
          out.template fmtStart<Fmt::EditCursor>(ctx);
          if(text[i]) out.put(text[i]);
          out.template fmtStop<Fmt::EditCursor>(ctx);
          out.put(&text[i+1]);
        } else out.put(text,sz);
        Base::printItem(out,ctx);
      }

      template<bool isKbd,typename Nav>
      std::enable_if_t<!isKbd,bool> nav(Nav& n,const CKE& cke,const Path& path)
        {return Base::template nav<isKbd>(n,cke,path)||n.doNav(cke,std::min(sz,ss()+1),false);}

      template<bool isKbd,typename Nav>
      std::enable_if_t<isKbd,bool> nav(Nav& n,const CKE& cke,const Path& path) {
        if(n.navMode()==NavMode::Edit) {
          if(cke.cmd==Cmd::Key) {
            if(cke.key==8||cke.key==127) {//backspace
              if(path.sel()>0) for(int n=path.sel();n<=sz;n++) text[n-1]=text[n];
              edited=true;
              return n.doNav({Cmd::Down},ss(),false);
            } else if(cke.ext) {//extended keys
              if(cke.key==0x33) for(int n=path.sel();n<sz;n++) text[n]=text[n+1];//delete
              else if(cke.key==0x48) n.go(0);//home
              else if(cke.key==0x46) n.go(ss());//end
              else return true;
              edited=true;
              return true;
            } else if(Mask::chk(cke.key)) {//write char
              for(int n=sz-1;n>path.sel();n--) text[n]=text[n-1];
              text[path.sel()]=cke.key;
              edited=true;
              return n.doNav({Cmd::Up},ss()+1,false);
            }
          }
          return n.doNav(cke,ss()+1,false);
        } else dout<<xy<0,1><<colors<RED,BLACK><<"TextField::nav "<<path<<" cmd:"<<cke.cmd<<" |"<<cnt<>++<<padWith<10><<flush;
        return Base::template nav<isKbd>(n,cke,path);
      }
      protected: Sz ss() const {return strnlen(text,sz-1);}
    };
    template<typename I> using Part=PadDraw::template Part<PartEnd<I>>;
  };

  /// @brief toggle enumerated field values on enter key
  /// is implicitly: a RecallDraw and Recall
  struct ToggleBehave {
    template<typename I>
    struct Part:RecallNavPos::template Part<I> {
      using Base=typename RecallNavPos::template Part<I>;
      using Base::Base;
      template<typename... OO> Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      // static constexpr const Wraps wraps{Wraps::yes};
      bool changed() const {return m_changed/*||Base::changed()*/;}
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

  template<typename T,typename B,typename... OO>
  using SelectFieldDef=ItemDef<
    RecallNavPos,
    Menu<T,B,EditField,ParentDraw,OO...>
  >;

  template<typename T,typename B,typename... OO>
  using ChooseFieldDef=ItemDef<
    RecallNavPos,
    Menu<T,B,OO...>
  >;

  template<typename T,typename O,typename... OO>
  using NumFieldDef
    =ItemDef<T,AsEditMode<>,EditField,ParentDraw,O,OO...>;
};