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
    template<typename Out> static constexpr void printHidden(Out&,Ctx&) noexcept {}
    template<typename Out> static constexpr bool printHiddenMenu(Out&,Ctx&) noexcept {return false;}
    using Base::print;
    template<typename Out> static constexpr void printItem(Out&,Ctx&) noexcept {}
    template<bool isKbd,typename Nav> static constexpr bool nav(Nav& n,const CKE& cke,Path) {return false;}
    template<typename Nav,typename P> static constexpr bool setStr(Nav&,const char*,P) {return false;}
  };

  template<typename... OO>
  struct ItemDef:APIOf<ItemAPI<>,OO...>{
    using Base=APIOf<ItemAPI<>,OO...>;
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
      Base::printItem(out,ctx);
      return Base::changed();
    }
    template<typename Out> bool printItem(Out& out,Ctx&& ctx={{}})
      {return printItem(out,static_cast<Ctx&>(ctx));}

    // find<Q>()/find(Q) are NOT declared here: for ItemDef<Menu<...>> they must be
    // inherited unshadowed from Menu::Part (own chain or body search); a plain ItemDef
    // (no nested Menu) has nothing to search and isn't a valid find<> receiver.

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

    virtual bool printMenu(IOut& out,Ctx& ctx) override {return Base::printMenu(out,ctx);}
    virtual bool printBody(IOut& out,Ctx& ctx) override {return Base::printBody(out,ctx);}
    virtual void printItem(IOut& out,Ctx& ctx) override {Base::printItem(out,ctx);}
    virtual bool enabled() const override {return Base::enabled();}
    virtual void enable(bool o=true) override {return Base::enable(o);}
    virtual bool changed() override {return Base::changed();}
    virtual void sync() override {Base::sync();}
    virtual void sync(IOut& out) override {Base::sync(out);}
    virtual bool up() const {return Base::up();};
    virtual bool down() const {return Base::down();};
    virtual bool _nav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<false>(n,cke,p);}
    virtual bool _kbdNav(INav& n,const CKE& cke,const Path p) override {return Base::template nav<true>(n,cke,p);}
    template <typename Out> static constexpr bool printMenu(Out& out,Ctx& ctx) {return Base::printMenu(out,ctx);}
    template<typename Out> static constexpr void printItem(Out& out,Ctx& ctx) {return Base::printItem(out,ctx);}
  };

  //---------------------------------------------------------------------------------------------
  using ActionFunc=bool(&)(int);

  template<ActionFunc action>
  struct Action {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
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
    struct End {
      template<typename O>
      struct Part:O {
        using Base=O;
        using Base::Base;
        template<typename Out> static void print(Out&) noexcept {}
        template<typename Out> static void printItem(Out&,Ctx&) noexcept {}
      };
    };
    template<typename I>
    struct Part:hapi::Chain<II...,End>::template Part<I> {
      using Base=typename hapi::Chain<II...,End>::template Part<I>;
      using Base::Base;
      // skip II... in flat output chain
      template<typename Out>
      void print(Out& out) const noexcept {I::print(out);}
      // skip II... in printItem chain
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) noexcept {I::printItem(out,ctx);}
      // render II... only — Base inherits from Chain<II...,End>::Part<I>; End stops before I
      template<typename Out>
      void printHidden(Out& out,Ctx& ctx) {
        if(!ctx) return;
        static_cast<Base&>(*this).printItem(out,ctx);
      }
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path p)
        {return I::template nav<isKbd>(n,cke,p);}
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
      bool m_levelOpened{false};  // true when I::nav opened a level on Enter (e.g. TextField padOpen)
      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r=false;
        if(cke.cmd==Cmd::Enter) {
          auto lv=n.level();
          n.navMode(path.len?NavMode::Nav
            :(n.navMode()==NavMode::Edit?NavMode::Nav:NavMode::Edit));
          r=true;
          bool ir=I::template nav<isKbd>(n,cke,path);
          m_levelOpened=(n.level()!=lv&&n.navMode()==NavMode::Edit);
          return ir||r;
        } else if(cke.cmd==Cmd::Esc&&!path.len&&n.navMode()==NavMode::Edit) {
          n.navMode(NavMode::Nav);
          // if I::nav opened a level on Enter, let close() fire to undo it (r stays false)
          // if not (NumField, date sub-items), consume Esc so close() is skipped
          if(!m_levelOpened) r=true;
          m_levelOpened=false;
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
          case Cmd::Up: Base::down(); return true;
          case Cmd::Down: Base::up(); return true;
          default: break;
        }
        return Base::template nav<isKbd>(n,cke,path);
      }
    };
  };

  /// @brief store and restore navigation position
  struct RecallNavPos {
    template<typename I>
    struct Part:I {
      using Base=I;
      using Base::Base;
      template<typename... OO>
      constexpr Part(OO&&... oo):Base{std::forward<OO>(oo)...}{}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        Base::printItem(out,ctx);
        out.setPos(out.getPos());
        if(ctx) {
          if(ctx.mode==NavMode::Edit||ctx.after()>1) Base::body.printItem(out,ctx,ctx.path.last());
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
      template<typename Fn>
      auto visit(Fn&& fn) {return Base::body.visit(m_sel,std::forward<Fn>(fn));}
      protected: Sz m_sel{0};
    };
  };

  /// @brief tags a choice item with its real value, retrieved via RecallNavPos::visit(fn)
  template<auto V>
  struct EnumValue {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static constexpr decltype(V) value() noexcept { return V; }
    };
  };

  /// @brief syncs the selected choice's EnumValue<V> out to an external storage W on Enter —
  /// enum fields (Select/Choose/Toggle) otherwise only ever hold their own m_sel index
  /// (RecallNavPos), with no equivalent to how NumField's value can be owned or externally
  /// referenced/composed. W follows the same Data-chain shape as everywhere else — Data<T>
  /// for an owned member, DataRef<&ext>/DataFn<Src> for external — so SyncValue<Data<T>>,
  /// SyncValue<DataRef<&ext>>, SyncValue<DataFn<Src>> are all equally valid.
  /// Place above a RecallNavPos-based field (SelectFieldDef/ChooseFieldDef/ToggleFieldDef).
  template<typename W>
  struct SyncValue {
    template<typename I>
    struct Part : I {
      using Base = I;
      using Base::Base;
      using Storage = typename W::template Part<oneData::DataAPI<>>;
      Storage m_value{};

      auto value() const noexcept { return m_value.get(); }

      template<bool isKbd,typename Nav>
      bool nav(Nav& n,const CKE& cke,const Path& path) {
        bool r = Base::template nav<isKbd>(n,cke,path);
        if(cke.cmd==Cmd::Enter) Base::visit([this](auto& item){ m_value.set(item.value()); });
        return r;
      }
    };
  };

  struct ItemNav; // forward — needed by ParentDraw::rules()

  struct ParentDraw {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<hapi::SameAs<RecallNavPos>, After>,
        "ParentDraw: RecallNavPos must be placed above ParentDraw in the ItemDef chain");
      static_assert(Excludes<hapi::SameAs<ItemNav>, Before, After>,
        "ParentDraw: ItemNav cannot coexist with ParentDraw — ParentDraw handles Enter/close by itself; remove ItemNav");
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
      static_assert(Excludes<hapi::SameAs<ParentDraw>, Before, After>,
        "ItemNav: cannot coexist with ParentDraw — ParentDraw handles Enter/close; remove ItemNav");
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
          template<typename Out> static constexpr void printItem(Out&,Ctx&) noexcept {}
          template<typename Out> static constexpr void print(Out&) noexcept {}
        };
      };
      template<typename O>
      struct Part:Chain<OO...,End>::template Part<O> {
        using Base=typename Chain<OO...,End>::template Part<O>;
        using Base::Base;
        template<typename Out> void print(Out& out) const { O::print(out); }
        template<typename Out>
        void printItem(Out& out,Ctx& ctx) {
          if(!(out.lockMode()==LockMode::None||out.lockMode()==LockMode::Update)) return;
          LockMode lm=out.lockMode();
          alt.resume();
          if(clr==Clear::yes) alt.clear();
          Base::printItem(alt,ctx);
          out.resume();  // restores lockMode→None, then cursor→origin, then colors
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
        template<typename Out> static constexpr void printItem(Out&,Ctx&) noexcept {}
        template<typename Out> static constexpr void print(Out&) noexcept {}
      };
    };
    template<typename O>
    struct Part:Chain<OO...,End>::template Part<O> {
      using Base=typename Chain<OO...,End>::template Part<O>;
      using Base::Base;
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

  // liquid position — item jumps to a fixed screen position before rendering ----
  // Incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter): scroll
  // measurement assumes items advance the cursor sequentially; a liquid item warping
  // away and back corrupts that math. Enforced below, not just documented — a rule.
  // On a non-cursor device (e.g. plain serial) position is meaningless, so Liquid
  // falls back to a normal in-sequence print instead of touching setPos at all.

  /// @brief compile-time item position: printItem moves cursor to (x,y), renders,
  /// then restores the original position so the next sequential item isn't displaced.
  template<Sz x,Sz y>
  struct Liquid {
    template<typename I>
    struct Part:I {
      using Base=I;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Excludes<IsScrollBody,typename Out::Types>,
          "Liquid: incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "scroll measurement needs sequential item positions; use FullPrinter/NoTitlePrinter instead");
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Pos saved=out.getPos();
          out.setPos({x,y});
          Base::printItem(out,ctx);
          out.setPos(saved);
        } else Base::printItem(out,ctx);  // no addressable cursor: fall back to normal print
      }
    };
  };

  /// @brief runtime item position: set liquidPos({x,y}) to relocate item on screen.
  /// Same save/restore and non-cursor fallback behavior as Liquid<x,y>.
  struct LiquidPos {
    template<typename I>
    struct Part:I {
      using Base=I;
      Pos m_liquidPos{0,0};
      void liquidPos(Pos p) {m_liquidPos=p;}
      Pos liquidPos() const {return m_liquidPos;}
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Excludes<IsScrollBody,typename Out::Types>,
          "LiquidPos: incompatible with ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "scroll measurement needs sequential item positions; use FullPrinter/NoTitlePrinter instead");
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          Pos saved=out.getPos();
          out.setPos(m_liquidPos);
          Base::printItem(out,ctx);
          out.setPos(saved);
        } else Base::printItem(out,ctx);  // no addressable cursor: fall back to normal print
      }
    };
  };

  /// @brief item claims the rest of the current page: after printing its own content it
  /// pads down to the bottom of the free area (Cursor::clearFree()) so the enclosing
  /// body's per-item free().y accounting sees this item as having consumed the whole
  /// page. Requires ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — its
  /// scroll-to-fit loop is what turns "this item eats all remaining room" into
  /// single-item-per-screen paging as the selection moves; under a non-scrolling body
  /// (FullPrinter/NoTitlePrinter) this would just pad trailing blank lines with no
  /// paging effect. Unlike Liquid/LiquidPos this never jumps the cursor backward — it
  /// only pads forward — so it doesn't corrupt ScrollBodyPrinter's sequential position
  /// measurement the way a decal that warps away and back would.
  struct FullScreen {
    template<typename I>
    struct Part:I {
      using Base=I;
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        static_assert(hapi::Requires<IsScrollBody,typename Out::Types>,
          "FullScreen: requires ScrollBodyPrinter (ScrollPrinter/NoTitleScrollPrinter) — "
          "use one of those instead of FullPrinter/NoTitlePrinter");
        Base::printItem(out,ctx);
        if constexpr(hapi::query<IsCursor,typename Out::Types>) {
          // Runs before the enclosing ItemPrinter's own fmtStop<Fmt::Item> (clearToEOL()+nl(),
          // same shape as Cursor::clearLine()) finalizes *this* row — so free().y here still
          // counts the still-open current row as "free". Padding down to free().y>1 (not >0)
          // leaves exactly that one row for the outer finalization's own nl() to consume;
          // padding to 0 here would double-nl() this row and overshoot by one (verified via a
          // scroll-loop trace: every top setting converged one row over budget, f=-1 instead
          // of 0, until this fix).
          while(out.free().y>1) out.clearLine();
        }
      }
    };
  };

  // output partition tag -------------------------------------------------------
  /// @brief marks an item as belonging to output partition Tag.
  /// - SkipOutId<Tag> in the main OutDef skips these items on the main output.
  /// - PartitionBody<Tag,Body> renders only these items to a secondary output.
  /// - Items are nav-invisible when placed after the body's navSize() boundary.
  template<typename Tag>
  struct OutId {
    template<typename O> using Part=O;  // pure tag: zero behavior, queryable via SameAs<OutId<Tag>>
  };

};//namespace oneMenu

//rules ItemDef query specialization --
template<typename Q,typename... OO>
constexpr const bool hapi::template query<Q,oneMenu::template ItemDef<OO...>>{(hapi::template query<Q,OO>||...)};

