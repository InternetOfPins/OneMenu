// EXPECT-ERROR: ParentDraw: ItemNav cannot coexist with ParentDraw
// Control for wrapper_hidden_rule.cpp: the same two components placed directly. Must be rejected with the same text.
#include <oneMenu/oneMenu.h>
using namespace oneMenu;
static ItemDef<ParentDraw,ItemNav> x;
int main() {return 0;}
