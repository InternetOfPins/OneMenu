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
    template<typename Out>
    static constexpr bool printBody(Out&,Ctx&,Sz=0) noexcept {return false;}
  };

  template<typename Tag,typename O,typename... OO>
  struct PartitionBody<Tag,StaticBody<O,OO...>> {
    using Body=StaticBody<O,OO...>;
    Body& body;

    static constexpr Sz size() noexcept {return Body::size();}  // preserves indices for find<>

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

  // ── View<Tag, Body> ──────────────────────────────────────────────────────────
  // Extends PartitionBody with a compile-time Types alias — a real hapi::Chain of
  // the Tag-matching item TYPES, in body order, built via hapi::Filter (not a new
  // filtering mechanism of its own). Runtime access (body&, size(), printBody())
  // is inherited from PartitionBody UNCHANGED and deliberately does not compact
  // or renumber anything: OneMenu's nav/Cursor/Action(index)/BodyAction(index)/
  // RecallNavPos machinery all share ONE index space tied to the original body's
  // real positions, with no translation layer anywhere — a compacted 0..k-1
  // renumbering of just the matches would silently break that the moment any
  // partition-aware nav/focus code is built on top of this. Types exists purely
  // for compile-time/HAPI-generic consumption (hapi::query, hapi::Map, membership
  // checks), never for indexing back into the live body.
  //
  // Types naturally reduces to hapi::Chain<> when nothing matches Tag — already a
  // fully-valid, safe-by-default HAPI type (App/Ins/Map all no-op on it), unlike
  // hapi::FindFirst<Q> which hard-fails on zero matches. Consumers of View::Types
  // must be written the same way: safe on empty, no assumption of >=1 match.
  //
  // Note: distinct from Fmt::View (sys/enums.h) — that's the outermost
  // screen-chrome formatting region; this View is a filtered item subset.
  template<typename Tag,typename Body> struct View; // mirrors PartitionBody's own forward-decl

  template<typename Tag>
  struct View<Tag,StaticBody<>>:PartitionBody<Tag,StaticBody<>> {
    using Types = typename hapi::Filter<hapi::FromTypes<hapi::SameAs<OutId<Tag>>>>
                    ::template Check<StaticBody<>>;   // reduces to hapi::Chain<>
  };

  template<typename Tag,typename O,typename... OO>
  struct View<Tag,StaticBody<O,OO...>>:PartitionBody<Tag,StaticBody<O,OO...>> {
    using Base  = PartitionBody<Tag,StaticBody<O,OO...>>;
    using Query = hapi::FromTypes<hapi::SameAs<OutId<Tag>>>;
    using Types = typename hapi::Filter<Query>::template Check<StaticBody<O,OO...>>;
    // body&, size(), printBody() all inherited unchanged from Base.
  };

  /// @brief factory: view<FooterTag>(body).printBody(out,ctx); view<FooterTag>(body)::Types
  template<typename Tag,typename... OO>
  View<Tag,StaticBody<OO...>> view(StaticBody<OO...>& body)
    {return {body};}

};//namespace oneMenu
