// EXPECT-ERROR: ParentDraw: ItemNav cannot coexist with ParentDraw
// The rules() of components INSIDE a wrapper run as if they were placed directly in the ItemDef: this is the same
// violation as ItemDef<ParentDraw,ItemNav> (rejected), hidden in a Hidden<> (silently accepted before wrappers validated).
#include <oneMenu/oneMenu.h>
using namespace oneMenu;
static ItemDef<Hidden<ParentDraw,ItemNav>> x;
int main() {return 0;}
