// Positive controls: valid compositions, with and without a wrapper, must compile (validation of wrapper contents is on).
#include <oneMenu/oneMenu.h>
using namespace oneMenu;
static ItemDef<ParentDraw>                 direct;
static ItemDef<Hidden<ParentDraw>>         wrapped;
static ItemDef<Hidden<Hidden<ParentDraw>>> wrapped_twice;
static ItemDef<Hidden<ItemNav>>            nav_wrapped;
int main() {return 0;}
