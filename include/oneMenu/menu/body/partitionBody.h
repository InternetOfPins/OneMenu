#pragma once

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/item.h"

namespace oneMenu {

  // ── SkipOutId<Tag> ─────────────────────────────────────────────────────────
  /// @brief output chain component: intercepts printItem and skips items tagged OutId<Tag>.
  /// Place above the printer in the main OutDef to suppress footer items on the main output.
  ///
  ///   OutDef<SkipOutId<FooterTag>, ScrollPrinter, ANSIFmt, ..., StaticArea<30,9>> main_out;
  template<typename Tag>
  struct SkipOutId {
    template<typename O>
    struct Part:O {
      using Base=O;
      template<typename I>
      bool printItem(I& i,Ctx& ctx) {
        if constexpr(hapi::query<hapi::SameAs<OutId<Tag>>,I>) return false;
        return Base::printItem(i,ctx);
      }
    };
  };

  // ── PartitionBody<Tag, Body> ────────────────────────────────────────────────
  // Recursive specialisations over StaticBody<>.
  template<typename Tag,typename Body> struct PartitionBody;

  template<typename Tag>
  struct PartitionBody<Tag,StaticBody<>> {
    StaticBody<>& body;
    static constexpr Sz   size()    noexcept {return 0;}
    static constexpr bool changed() noexcept {return false;}
    static constexpr void sync()    noexcept {}
    template<typename Out>
    static constexpr bool printBody(Out&,Ctx&,Sz=0) noexcept {return false;}
  };

  template<typename Tag,typename O,typename... OO>
  struct PartitionBody<Tag,StaticBody<O,OO...>> {
    using Body=StaticBody<O,OO...>;
    Body& body;

    static constexpr Sz size() noexcept {return Body::size();}  // preserves indices for find<>

    bool changed() const {
      bool r=false;
      if constexpr(hapi::query<hapi::SameAs<OutId<Tag>>,O>) r=body.head.changed();
      return PartitionBody<Tag,StaticBody<OO...>>{body.tail}.changed()||r;
    }
    void sync() {
      if constexpr(hapi::query<hapi::SameAs<OutId<Tag>>,O>) body.head.sync();
      PartitionBody<Tag,StaticBody<OO...>>{body.tail}.sync();
    }

    template<typename Out>
    bool printBody(Out& out,Ctx& ctx,Sz bidx=0) {
      bool r=false;
      if constexpr(hapi::query<hapi::SameAs<OutId<Tag>>,O>)
        r=out.printItem(body.head,ctx);
      return PartitionBody<Tag,StaticBody<OO...>>{body.tail}.printBody(out,ctx,bidx+1)||r;
    }
  };

  /// @brief factory: partition<FooterTag>(body).printBody(footer_out, ctx)
  template<typename Tag,typename... OO>
  PartitionBody<Tag,StaticBody<OO...>> partition(StaticBody<OO...>& body)
    {return {body};}

};//namespace oneMenu
