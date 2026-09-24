// EXPECT-ERROR: ItemPrinter: Cursor must be placed below ItemPrinter
// Control for wrapper_own_rule.cpp: ItemPrinter placed directly (its own rule was always live here).
#include <oneMenu/oneMenu.h>
using namespace oneMenu; using namespace hapi;
struct API {};
struct C {template<typename O> using Part=O;};
static_assert(BuildRules<Chain<>,Chain<API,ItemPrinter<C>>>::rules(), "direct");
int main() {return 0;}
