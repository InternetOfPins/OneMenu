#pragma once

#include "oneMenu/menu/out.h"

namespace oneMenu {

  /// @brief will call fmtStart(tag) + Base::print + fmtStop(tag)
  /// @tparam tag 
  template<Fmt tag>
  struct FmtPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using Base::fmtStart;
      using Base::fmtStop;
      using This=Part<O>;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        Base::template fmtStart<tag>(ctx);
        bool r=Base::printMenu(i,ctx);
        Base::template fmtStop<tag>(ctx);
        return r;
      }
    };
  };

  using ViewPrinter=FmtPrinter<Fmt::View>;
  using AccelPrinter=FmtPrinter<Fmt::Accel>;
  using LabelPrinter=FmtPrinter<Fmt::Label>;
  using FieldPrinter=FmtPrinter<Fmt::Field>;
  using UnitPrinter=FmtPrinter<Fmt::Unit>;
  using EditModePrinter=FmtPrinter<Fmt::EditMode>;
  using TextEditCursorPrinter=FmtPrinter<Fmt::EditCursor>;

  /// @brief groups some printer parts to form menu
  /// @tparam ...OO the body parts
  template<typename... OO>
  struct MenuPrinter : aPrinter {
    template<typename O>
    struct Part:Chain<OO...>::template Part<O> {
      using IsPrinter=std::true_type;
      using Base=typename Chain<OO...>::template Part<O>;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::obj;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        ctx.idx=0;
        Base::template fmtStart<Fmt::Menu>(ctx);
        // i.printTo(Base::obj());
        bool r=Base::printMenu(i,ctx);
        Base::template fmtStop<Fmt::Menu>(ctx);
        return r;
      }
    };
  };

  /// @brief print the title + format
  struct TitlePrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::Title>(ctx);
        i.print(O::obj(),ctx);//title
        O::template fmtStop<Fmt::Title>(ctx);
        return O::printMenu(i,ctx)||i.changed();
      }
    };
  };

  /// @brief start body printing process by redirecting to the item.
  /// Chains to Base::printMenu so FooterPrinter (and any other post-body printers)
  /// in the same MenuPrinter<...> pack are reached through the normal fmt pipeline.
  struct BodyPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using Base::obj;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        Base::template fmtStart<Fmt::Body>(ctx);
        bool r=i.printBody(O::obj(),ctx);
        Base::template fmtStop<Fmt::Body>(ctx);
        return Base::printMenu(i,ctx)||r;
      }
    };
  };

  /// @brief footer boundary — emits fmtStart/fmtStop(Footer) so format layers
  /// can draw a separator, colour band, or nl. No default content is printed here;
  /// actual footer text is driven by OnFocus/Put::ToOut in individual items.
  struct FooterPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        Base::template fmtStart<Fmt::Footer>(ctx);
        Base::template fmtStop<Fmt::Footer>(ctx);
        return Base::printMenu(i,ctx)||i.changed();
      }
    };
  };

  /// @brief print scroll menu body
  struct ScrollBodyPrinter : aPrinter {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "ScrollBodyPrinter: Cursor must be placed below ScrollBodyPrinter — scroll measurement needs tracked position");
      return true;
    }
    template<typename P>
    struct Part:BodyPrinter::Part<P> {
      using IsPrinter=std::true_type;
      using Base=typename BodyPrinter::Part<P>;
      using Base::lockMode;
      using Base::pos;
      using Base::free;
      using Base::setPos;

      template<typename I>
      bool printMenu(I& i,Ctx& ctx) {
        if(i.size()==0) return false;
        LockMode om=lockMode();
        Sz x=Base::pos().x;
        Sz y=Base::pos().y;
        if(ctx.sel(ctx.pAt)<0) ctx.path.data[(int)ctx.pAt]=0;
        else if(ctx.sel(ctx.pAt)>=i.size()) ctx.path.data[(int)ctx.pAt]=i.size()-1;
        if(ctx.sel(ctx.pAt)<ctx.top()) {
          ctx.top(ctx.sel());//--scroll down
          om=LockMode::None;//scroll => full redraw
        } else for(;;) {
          lockMode(LockMode::Measure);
          // measure body only — fmtStop<Footer> emits nl() which would corrupt free().y
          Base::template fmtStart<Fmt::Body>(ctx);
          i.printBody(Base::obj(),ctx);
          Base::template fmtStop<Fmt::Body>(ctx);
          Sz f=free().y;
          Sz ci=ctx.idx;
          ctx.idx=0;
          if(ctx.sel(ctx.pAt)<ci&&(!(ctx.sel(ctx.pAt)==(ci-1)&&f<0))) break;
          setPos(Pos{x,y});
          ctx.top(ctx.top()+1);//--scroll up
          om=LockMode::None;//scroll => full redraw
        };
        lockMode(om);
        setPos({x,y});
        bool r=Base::printMenu(i,ctx);
        return r;
      }
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        if(ctx.idx<ctx.top()) {
          ctx.idx++;
          return false;
        }
        return Base::free().y>0?Base::printItem(i,ctx):false;
      }
    };
  };

  /// @brief groups some printer parts to form a item body, will be formatted as a item
  /// also checks LockMode and act appropriately
  template<typename... OO>
  struct ItemPrinter : aPrinter {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Requires<IsCursor, After>, "ItemPrinter: Cursor must be placed below ItemPrinter — partial-update repositioning needs tracked position");
      return true;
    }
    template<typename O>
    struct Part:Chain<OO...>::template Part<O> {
      using IsPrinter=std::true_type;
      using Base=typename Chain<OO...>::template Part<O>;
      using Base::fmtStart;
      using Base::fmtStop;
      using Base::lockMode;
      using Base::setPos;
      using Base::pos;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        LockMode om=lockMode();
        if(om==LockMode::Update
          &&(i.changed()||(ctx.prev!=ctx.sel()&&(ctx.idx==ctx.prev||ctx.idx==ctx.sel())))
        ) lockMode(LockMode::None);
        ctx.enabled =i.enabled();
        Base::template fmtStart<Fmt::Item>(ctx);
        bool r=Base::printItem(i,ctx);
        Base::template fmtStop<Fmt::Item>(ctx);
        ctx.idx++;
        if(lockMode()==LockMode::Sync) i.sync();
        else if(om==LockMode::Update&&lockMode()==LockMode::None) lockMode(LockMode::Update);
        return r;
      }
    };
  };

  /// @brief print the item
  struct ItemBodyPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        i.printItem(Base::obj(),ctx);
        return Base::printItem(i,ctx)||i.changed();
      }
    };
  };

  /// @brief triggers the edit index accel number print
  struct IndexPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using This=Part<O>;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::Index>(ctx);
        O::template fmtStop<Fmt::Index>(ctx);
        return O::printItem(i,ctx);
      }
    };
  };

  /// @brief triggers the navigation cursor print
  struct NavCursorPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      using Base=O;
      using This=Part<O>;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::template fmtStart<Fmt::NavCursor>(ctx);
        O::template fmtStop<Fmt::NavCursor>(ctx);
        bool r=O::printItem(i,ctx);
        return r;
      }
    };
  };

  /// @brief allow inclusion of data on the printers queue as a item part
  /// @tparam Data: included static data
  template<typename Data>
  struct StaticDataPrinter : aPrinter {
    template<typename O>
    struct Part:O {
      using IsPrinter=std::true_type;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        O::put(Data::get());
        return O::printItem(i,ctx);
      }
    };
  };

  #ifdef MENU_DEBUG
    /// @brief debug printer part, printing path
    struct PathPrinter {
      template<typename O>
      struct Part:O {
        using IsPrinter=std::true_type;
        using Base=O;
        template<typename I>
        bool printItem(I& i,Ctx& ctx) {
          bool r=O::printItem(i,ctx);
          return r;
        }
      };
    };
  #endif

  //format printers to use with item description ----------------
  template<Fmt tag,typename... OO>
  struct AsFmt {
    struct PartEnd {
      template<typename O>
      struct Part:O {//TODO: put this content into Part::print and generalize PartEnd
        using IsPrinter=std::true_type;
        using Base=O;
        using Base::Base;
        template<typename Out>
        void printItem(Out& out,Ctx& ctx) {}
      };
    };
    template<typename O>
    struct Part:Chain<OO...,PartEnd>::template Part<O> {
      using Base=typename Chain<OO...,PartEnd>::template Part<O>;
      using Base::Base;
      template<typename Out> void print(Out&) const {}  // data flows inside printItem only
      template<typename Out>
      void printItem(Out& out,Ctx& ctx) {
        out.template fmtStart<tag>(ctx);
        out.template fmtStart<Fmt::Data>(ctx);
        Base::print(out);
        out.template fmtStop<Fmt::Data>(ctx);
        Base::printItem(out,ctx);
        out.template fmtStop<tag>(ctx);
        O::printItem(out,ctx);
      }
    };
  };

  template<typename... OO> using AsView=AsFmt<Fmt::View,OO...>;
  template<typename... OO> using AsTitle=AsFmt<Fmt::Title,OO...>;
  template<typename... OO> using AsMenu=AsFmt<Fmt::Menu,OO...>;
  template<typename... OO> using AsBody=AsFmt<Fmt::Body,OO...>;
  template<typename... OO> using AsItem=AsFmt<Fmt::Item,OO...>;
  template<typename... OO> using AsIndex=AsFmt<Fmt::Index,OO...>;
  template<typename... OO> using AsAccel=AsFmt<Fmt::Accel,OO...>;
  template<typename... OO> using AsNavCursor=AsFmt<Fmt::NavCursor,OO...>;
  template<typename... OO> using AsField=AsFmt<Fmt::Field,OO...>;
  template<typename... OO> using AsLabel=AsFmt<Fmt::Label,OO...>;
  template<typename... OO> using AsEditMode=AsFmt<Fmt::EditMode,OO...>;
  template<typename... OO> using AsEditCursor=AsFmt<Fmt::EditCursor,OO...>;
  template<typename... OO> using AsData=AsFmt<Fmt::Data,OO...>;
  template<typename... OO> using AsUnit=AsFmt<Fmt::Unit,OO...>;
    

  using ItemsPrinter=ItemPrinter<IndexPrinter,NavCursorPrinter,ItemBodyPrinter>;

  // Full printers: title + body + footer
  using FullPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,BodyPrinter,FooterPrinter,ItemsPrinter>
  >;
  using ScrollPrinter=Chain<
    ViewPrinter,
    MenuPrinter<TitlePrinter,ScrollBodyPrinter,FooterPrinter,ItemsPrinter>
  >;

  // No-title variants: body + footer only — default for small-display devices
  using NoTitlePrinter=Chain<
    ViewPrinter,
    MenuPrinter<BodyPrinter,FooterPrinter,ItemsPrinter>
  >;
  using NoTitleScrollPrinter=Chain<
    ViewPrinter,
    MenuPrinter<ScrollBodyPrinter,FooterPrinter,ItemsPrinter>
  >;
};//oneMenu