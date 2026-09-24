/**
 * @file test_view.cpp
 * @brief oneMenu::View<Tag,Body> (partitionBody.h): Types is the Chain of the Tag-matching items TAKEN WHOLE, and
 * printing through a view matches printing through a partition.
 *
 * The compile-time half pins how ItemDef is opened by HAPI's walks (item.h, `Expand<ItemDef>`): queries look
 * inside an item, but Filter/Map/Partition take it whole. View selects items with Filter<FromTypes<..>>, so if a
 * later change let Filter descend into ItemDef again, View::Types would silently lose every item and these
 * static_asserts fail. (It did exactly that until the ItemDef Expand entry split the two behaviours.)
 *
 * Native-only, direct g++ like test_footer.cpp / test_idtext.cpp.
 */
#include <cstdio>
#include <oneMenu/oneMenu.h>
#include <oneMenu/menu/body/partitionBody.h>

using namespace hapi;
using namespace oneData;
using namespace oneMenu;

struct TagX {};
struct TagY {};

using Plain  = ItemDef<Data<int>>;
using Tagged = ItemDef<Data<int>,OutId<TagX>>;

// 2 plain items and 2 TagX-tagged items, interleaved
using Body4 = StaticBody<Plain,Tagged,Plain,Tagged>;
using Body0 = StaticBody<Plain,Plain,Plain>;

// ── compile time ─────────────────────────────────────────────────────────────
static_assert(std::is_same_v<View<TagX,Body4>::Types, Chain<Tagged,Tagged>>,
  "View::Types is the tagged items, whole, in body order");
static_assert(std::is_same_v<View<TagX,Body0>::Types, Chain<>>,
  "no match: an empty Chain, not a hard error");
static_assert(std::is_same_v<View<TagY,Body4>::Types, Chain<>>,
  "a tag nothing carries: an empty Chain");

// the two behaviours View needs from ItemDef at once
static_assert( query<SameAs<OutId<TagX>>,Tagged>,  "a query looks inside an item (OutId<TagX> is one of its components)");
static_assert(!query<SameAs<OutId<TagX>>,Plain>,   "...and finds nothing that is not there");
static_assert(std::is_same_v<Eval<Filter<SameAs<Tagged>>,Chain<Plain,Tagged>>, Chain<Tagged>>,
  "Filter takes an item whole (it can select the ItemDef itself)");
static_assert(std::is_same_v<Eval<Filter<SameAs<OutId<TagX>>>,Chain<Plain,Tagged>>, Chain<>>,
  "Filter does not reach the components inside an item");

// ── run time ─────────────────────────────────────────────────────────────────
struct RecordOut {
  int calls = 0;
  template<typename I> bool printItem(I&, Ctx&) {calls++; return true;}
};

int main() {
  bool ok = true;
  Ctx ctx{{}};

  Body4 bodyA{Plain{},Tagged{},Plain{},Tagged{}};
  Body4 bodyB{Plain{},Tagged{},Plain{},Tagged{}};
  RecordOut outA, outB;
  bool rA = partition<TagX>(bodyA).printBody(outA,ctx);
  bool rB = view<TagX>(bodyB).printBody(outB,ctx);
  ok &= outA.calls == 2;                 // only the two tagged items are printed
  ok &= outB.calls == outA.calls && rA == rB;   // a view prints exactly what a partition prints
  ok &= partition<TagX>(bodyA).size() == 4 && view<TagX>(bodyB).size() == 4;   // both keep the body's own index space

  Body0 body0{Plain{},Plain{},Plain{}};
  RecordOut out0;
  bool r0 = view<TagX>(body0).printBody(out0,ctx);
  ok &= out0.calls == 0 && r0 == false;  // zero matches: nothing printed, not a failure

  printf("View takes items whole, prints like a partition: %s\n", ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
