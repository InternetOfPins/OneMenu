/**
 * @file item.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief item API
 * @version 5
 * @date 2026-04-17
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

#include "oneData/oneData.h"
using oneData::Bool;

namespace oneMenu {
  
  template<typename Cfg=Nil>
  struct ItemAPI:oneItem::ItemAPI<Cfg> {
    using Base=oneItem::ItemAPI<Cfg>;
    constexpr ItemAPI() {}
    // static constexpr const Wraps wraps{Wraps::no};
    template<typename> using Requires=std::false_type;
    template<typename> using Excludes=std::true_type;
    static constexpr Depth depth() {return 1;}
    static constexpr bool enabled() {return true;}
    static constexpr bool wraps() {return false;}
    static constexpr bool isPad() {return false;}
    static constexpr void enable(bool=true) {}
    static constexpr bool changed() {return false;}
    static constexpr Pos pos() {return {0,0};}
    static constexpr void sync() {}
    template<typename Out> static constexpr void sync(Out&) {}
    static constexpr bool up() {return false;}
    static constexpr bool down() {return false;}
    template<typename Out> static constexpr bool printMenu(Out&,Ctx&) {return false;}
    template<typename Out> static constexpr bool printBody(Out&,Ctx&) {return false;}
    using Base::print;
    template<typename Out> constexpr void printItem(Out&,Ctx&) {}
    template<bool isKbd,typename Nav> static constexpr bool nav(Nav& n,const CKE& cke,Path) {return false;}
    template<typename Nav,typename P> static constexpr bool setStr(Nav&,const char*,P) {return false;}
    //Id--
    // static constexpr int getId() {return -1;}
    // template<int> using HasId=std::false_type;
    // template<int> using WithId=ItemAPI<hapi::CRTP<ItemAPI<Nil>>>;
  };

  template<typename T>
  struct MenuData {
    template<typename O>
    struct Part : T::template Part<O> {//deriving from OneData *::Part<> we should have the data hapi here get/set
      using Base = typename T::template Part<O>;
      using Base::Base;
      template<typename Out> bool printItem(Out& out,Ctx& ctx) {
        Base::print(out);
        Base::printItem(out,ctx);
        return Base::changed();
      }
    };
  };

  // Chain transformation: lifts oneData:: components into MenuData<> wrappers.
  // Passthrough for everything else.
  template<typename T>              struct wrap_menu_data               { using Type = T; };
  template<typename T>              struct wrap_menu_data<oneData::Data<T>>       { using Type = MenuData<oneData::Data<T>>; };
  template<auto V>                  struct wrap_menu_data<oneData::StaticData<V>> { using Type = MenuData<oneData::StaticData<V>>; };
  template<const char* const* P>    struct wrap_menu_data<oneData::StaticText<P>> { using Type = MenuData<oneData::StaticText<P>>; };
  template<auto A>                  struct wrap_menu_data<oneData::DataRef<A>>    { using Type = MenuData<oneData::DataRef<A>>; };

  template<typename... OO>
  struct ItemDef:APIOf<ItemAPI<>,typename wrap_menu_data<OO>::Type...>{
    using Base=APIOf<ItemAPI<>,typename wrap_menu_data<OO>::Type...>;
    using Base::Base;
    using Base::printMenu;
    using Base::enabled;
    using Base::print;

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path p) {
      return enabled()?Base::template nav<isKbd>(n,cke,p):cke.cmd==Cmd::Enter;
    }

    template<typename Nav,typename P>
    bool setStr(Nav& n,const char* s,P p) {
      return enabled()?Base::template setStr(n,s,p):false;
    }

    template<typename Out> bool printItem(Out& out,Ctx& ctx) {
      // out.template fmtStart<Fmt::Data>(ctx);
      // print(out);
      // out.template fmtStop<Fmt::Data>(ctx);
      Base::printItem(out,ctx);
      return Base::changed();
    }
    template<typename Out> bool printItem(Out& out,Ctx&& ctx={{}})
      {return printItem(out,static_cast<Ctx&>(ctx));}

    template<typename... XX> using Ins=oneMenu::ItemDef<XX...,OO...>;
    template<typename... XX> using App=oneMenu::ItemDef<OO...,XX...>;

  };

  struct IItem {
    virtual bool printMenu(IOut& out,Ctx& ctx)=0;
    virtual bool printBody(IOut& out,Ctx&)=0;

    virtual bool enabled() const=0;
    virtual void enable(bool=true)=0;
    virtual bool changed()=0;
    virtual void sync()=0;
    virtual void sync(IOut& out)=0;
    virtual bool up() const=0;
    virtual bool down() const=0;
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p)=0;
    virtual bool _nav(INav& n,const CKE& cke,const Path p)=0;

    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path& path) {
      return isKbd?_kbdNav(n,cke,path):_nav(n,cke,path);
    }

    virtual void printItem(IOut& out,Ctx& ctx) {}

    template <typename Out>
    static constexpr bool printMenu(Out& out,Ctx& ctx)
      {return printMenu(out,ctx);}

  };

  template<typename... II>
  struct IItemDef:IItem, ItemDef<II...> {
    using Base=ItemDef<II...>;
    using Base::Base;

    // virtual Depth depth() const override {return Base::depth();}
    virtual bool printMenu(IOut& out,Ctx& ctx) override {return Base::printMenu(out,ctx);}
    virtual bool printBody(IOut& out,Ctx& ctx) override {return Base::printBody(out,ctx);}
    virtual void printItem(IOut& out,Ctx& ctx) override {Base::printItem(out,ctx);}
    virtual bool enabled() const override {return Base::enabled();}
    virtual void enable(bool o=true) override {return Base::enable(o);}
    virtual bool changed() override {return Base::changed();}
    // virtual bool changed(IOut& out) override {return Base::changed(out);}
    virtual void sync() override {Base::sync();}
    virtual void sync(IOut& out) override {Base::sync(out);}
    virtual bool up() const {return Base::up();};
    virtual bool down() const {return Base::down();};
    virtual bool _nav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<false>(n,cke,p);}
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<true>(n,cke,p);}
    template <typename Out> static constexpr bool printMenu(Out& out,Ctx& ctx) {return Base::printMenu(out,ctx);}
    template<typename Out> static constexpr void printItem(Out& out,Ctx& ctx) {return Base::printItem(out,ctx);}

    //Id--
    static constexpr int getId() {return -1;}
    template<int> using HasId=std::false_type;
    template<int> using WithId=ItemAPI<hapi::CRTP<ItemAPI<Nil>>>;
  };

  //---------------------------------------------------------------------------------------------
  using ActionFunc=bool(&)(int);

  template<ActionFunc action>
  struct Action {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      // constexpr Part(){}
      static constexpr bool act(int i) {return action(i);}
      template<bool isKbd,typename Nav>
      static constexpr bool nav(Nav& n,const CKE& cke,Path path) 
        {return cke.cmd==Cmd::Enter&&action(path.sel());}
    };
  };

  //attach an action on enter
  template <ActionFunc f>
  struct BodyAction {
    template <typename I>
    struct Part : I {
      using Base=I;
      using Base::Base;
      using Base::enabled;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) {
        if(cke.cmd==Cmd::Enter&&p.len) f(p.last());
        return Base::template nav<isKbd>(n,cke,p);
      }
    };
  };

  template<typename... II>
  struct Hidden {
    template<typename I>
    struct Part:oneItem::Hidden<II...>::template Part<I> {
      using Base=typename oneItem::Hidden<II...>::template Part<I>;
      using Base::Base;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {I::printItem(out,ctx);}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p)
        {return Base::template nav<isKbd>(n,cke,p);}
    };
  };

  template<typename... II>
  struct Decor {
    template<typename I>
    struct Part:oneItem::Decor<II...>::template Part<I> {
      using Base=typename oneItem::Decor<II...>::template Part<I>;
      using Base::Base;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {Base::printItem(out,ctx);}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p) noexcept
        {return I::template nav<isKbd>(n,cke,p);}
    };
  };

  template<bool ens>
  struct EnDis {
    using Type = bool;
    template<typename I>
    struct Part:Chain<Hidden<Default<Bool,ens>>>::template Part<I> {
      using Base=typename Chain<Hidden<Default<Bool,ens>>>::template Part<I>;
      using Base::Base;
      bool enabled(const Path ={}) const {return Base::get();}
      void enable(bool e=true) {Base::set(e);}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path)
        {return enabled()?Base::template nav<isKbd>(n,cke,path):false;}
    };
  };

  //fields ---
  struct EditField {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r=false;
        if(cke.cmd==Cmd::Enter) {
          // dout<<xy<0,3><<colors<BLUE,BLACK><<"EditField::Nav(Cmd::Enter) |"<<cnt<>++<<padWith<10><<flush;
          n.navMode(path.len?NavMode::Nav:NavMode::Edit);
          r=true;
        }
        return I::template nav<isKbd>(n,cke,path)||r;
      }
    };
  };

  //use range and data field to change value
  template<typename...II>
  struct NumField {
    template<typename I>
    struct Part:Chain<II...>::template Part<I> {
      using Base=typename Chain<II...>::template Part<I>;
      using Base::Base;
      using Base::set;
      using Base::get;
      using Base::up;
      using Base::down;
      using Type=typename Base::Type;
      constexpr bool valid() const {return Base::valid(get());}
      constexpr bool clamp() {return Base::set(Base::clamp(get()));}
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path) {
        if(n.navMode()==NavMode::Edit) switch(cke.cmd){
          case Cmd::Up: Base::up(); return true;
          case Cmd::Down: Base::down(); return true;
          default: break;
        }
        return Base::template nav<isKbd>(n,cke,path);
      }
    };
  };

  /// @brief store and restore navigation position
  struct RecallNavPos {
    /// @brief provide a default value to a index Recall
    /// @tparam val Sz index
    // template<Sz val>
    // using Default=Chain<Default<Data<Sz>,val>>;
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<typename... OO>
      constexpr Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      template<typename Out> 
      void printItem(Out& out,Ctx& ctx) {
        Base::printItem(out,ctx);
        out.setPos(out.pos());
        if(ctx) {
          if(ctx.mode==NavMode::Edit) Base::body.printItem(out,ctx,ctx.path.last());
          else Base::body.printItem(out,ctx,m_sel);
        }
        else Base::body.printItem(out,ctx,m_sel);
      }
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        if(cke.cmd==Cmd::Enter) {
          if(path.len) m_sel=path.sel();//store selected item index
          else {
            bool r=I::template nav<isKbd>(n,cke,path);//still check base
            n.go(m_sel);//restore selected index
            return r;
          }
        }
        return I::template nav<isKbd>(n,cke,path);//still check base
      }
      protected: Sz m_sel{0};
    };
  };

  struct ParentDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<RecallNavPos>, After>, "ParentDraw: RecallNavPos must be placed above ParentDraw in the ItemDef chain");
      return true;
    }
    template<typename I>
    struct Part:I {
      using I::I;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r=I::template nav<isKbd>(n,cke,path);
        if(!r&&cke.cmd==Cmd::Enter)
          r=path.len>0?n.close():n.padOpen();
        return r;
      }
    };
  };

  /// @brief put nav focus on this item on Cmd::Enter
  struct ItemNav {
    template<typename Before, typename After>
    static constexpr bool rules() {
      return true;
    }
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path path) {
        // Call base first — deeper component (ParentDraw etc.) gets priority.
        // ItemNav only acts when nothing deeper consumed Cmd::Enter.
        bool r=Base::template nav<isKbd>(n,cke,path);
        if(!r&&cke.cmd==Cmd::Enter) {
          if(path.len==0) r=n.open();
          else if(path.len==1) r=n.close();
        }
        return r;
      }
    };
  };

  /// @brief alternative representation for a value
  /// @tparam Title title type
  /// @tparam Value value type
  /// @todo why not derive from the target and redirect only the print? Enum depends on this!
  template<typename Title,typename Value>
  struct Alias {
    template<typename O>
    struct Part:Title::template Part<O> {
      using Base=typename Title::template Part<O>;
      ItemDef<Value> value{};
      using Base::Base;
      template<typename... OO>
      Part(Value&& v,OO&&... oo):value{v},Base{std::forward<OO&&>(oo)...}{}
    };
  };

  template<typename R,R& ref>
  struct ItemRef {
    template<typename O>
    struct Part:O {
      using Base=O;
      using Base::Base;
      using RefType=R;
      operator RefType&() const {return ref;}
      static constexpr const Depth depth() {return ref.depth();}
      // static constexpr Sz size(Path p={}) {return ref.size(p);}
      static constexpr bool enabled() {return ref.enable(); }
      static constexpr void enable(bool o=true) {ref.enable(o);}
      static constexpr bool changed() {return ref.changed();}
      static constexpr void sync() {ref.sync();}
      static constexpr bool up() {return ref.up();}
      static constexpr bool down() {return ref.down();}

      template <typename Out>
      static constexpr bool printMenu(Out& out,Ctx& ctx) 
        {return ref.printMenu(out.ctx);}
      
      template<typename Out>
      static constexpr bool printBody(Out& out,Ctx&ctx)
        {return ref.printBody(out,ctx);}

      template<typename Out> 
      static constexpr void printItem(Out& out,Ctx& ctx)
        {ref.printItem(out,ctx);}

      template<bool isKbd,typename Nav> 
      static constexpr bool nav(Nav& n,const CKE& cke,const Path p) 
        {return ref.nav(n,cke,p);}

      //Id--
      static constexpr int getId() {return -1;}
      template<int i> using HasId=typename R::template HasId<i>;
      template<int i> using WithId=typename R::template WithId<i>;
    };
  };

  template<typename... OO>
  struct Put {
    template<typename Alt,Alt& alt,Clear clr=Clear::no>
    struct ToOut {
      struct End {
        template<typename O>
        struct Part:O {
          using Base=O;
          using Base::Base;
          template<typename Out> static constexpr void printItem(Out& out,Ctx& ctx) {}
          // Block print so StaticText/data components inside Put<OO...> don't leak
          // their text back through the outer chain's print() call.
          template<typename Out> static constexpr void print(Out& out) noexcept {}
        };
      };
      template<typename O>
      struct Part:Chain<OO...,End>::template Part<O> {
        using Base=typename Chain<OO...,End>::template Part<O>;
        using Base::Base;
        // OO... content belongs only on alt, never on the main output's print chain.
        template<typename Out> void print(Out& out) const {}
        template<typename Out>
        void printItem(Out& out,Ctx& ctx) {
          if(!(out.lockMode()==LockMode::None||out.lockMode()==LockMode::Update)) return;
          LockMode lm=out.lockMode();
          alt.resume();
          if(clr==Clear::yes) alt.clear();
          // OO... components have print() not printItem() — use print to get their content.
          Base::print(alt);
          // Restore out's terminal state (colors + cursor) so the next item in out prints
          // correctly. footer.resume() leaves the terminal in alt's ANSI state.
          out.resume();
          out.lockMode(lm);
        }
      };
    };
  };

  template<typename... OO>
  struct OnFocus {
    struct End {
      template<typename O>
      struct Part:O {
        using Base=O;
        using Base::Base;
        template<typename Out> static constexpr void printItem(Out& out,Ctx& ctx) {}
      };
    };
    template<typename O>
    struct Part:Chain<OO...,End>::template Part<O> {
      using Base=typename Chain<OO...,End>::template Part<O>;
        using Base::Base;
      // Block OO... from the `print` chain: `print` is for inline data display
      // (called by MenuData::printItem → Base::print). The OO... pack must NOT
      // contribute to `print` — it must only fire via `printItem` (conditionally).
      // Forward to O::print to keep the outer chain intact beyond this component.
      template<typename Out> void print(Out& out) const { O::print(out); }
      template<typename Out> void printItem(Out& out,Ctx& ctx) {
        O::printItem(out,ctx);
        if(ctx) Base::printItem(out,ctx);
      }
    };
  };

  // compile-time ids --------------
  template<int id> struct Id {template<typename O> using Part=O;};
  template<auto V> inline constexpr hapi::SameAs<Id<V>> byId{};

};//namespace oneMenu

//rules ItemDef query specialization --
//TODO: why not Map?
template<typename Q,typename... OO>
constexpr const bool hapi::template query<Q,oneMenu::template ItemDef<OO...>>{(hapi::template query<Q,OO>||...)};

