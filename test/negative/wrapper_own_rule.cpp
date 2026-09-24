// EXPECT-ERROR: ItemPrinter: Cursor must be placed below ItemPrinter
// ItemPrinter carries a rules() of its own AND validates its contents. Splicing its children into the rule walk must not
// drop that own rule: it must fire whether ItemPrinter is placed directly or inside another validating wrapper
// (MenuPrinter). This is the case the first version of the splice got wrong.
#include <oneMenu/oneMenu.h>
using namespace oneMenu; using namespace hapi;
struct API {};
struct C {template<typename O> using Part=O;};
static_assert(BuildRules<Chain<>,Chain<API,MenuPrinter<ItemPrinter<C>>>>::rules(), "nested in a wrapper");
int main() {return 0;}
