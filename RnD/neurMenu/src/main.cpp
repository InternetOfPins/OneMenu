#include <oneMenu/oneMenu.h>
#include <oneMenu/IO/streamOut.h>
// using namespace oneData;
// using namespace oneIten;
// using namespace oneOutput;
using namespace oneMenu;
using namespace hapi;

#include <iostream>
using namespace std;
/// static space character
using Sp=StaticData<char,' '>;
/// static newline character
using NL=StaticData<char,'\n'>;

/// some action functions
bool op1(int i)  {cout<<"op1";return true;}
bool op2(int i)  {cout<<"op2";return true;}
bool op3(int i)  {cout<<"op3";return true;}
bool quit(int i) {cout<<"quit";return false;}

struct FlatNav {
  SizeT at;
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    // template<typename Out>
    // void print(Out& out,int i) const 
    //   {out.put(i==at?'>':' ');Base::printItem(out,i);}
  };
};

FlatNav myNav;

template<FlatNav& nav>
struct FlatCursor {
  using IsNavCursor = std::true_type;//<-- creating a new tag
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    template<typename Out>
    void printItem(Out& out,int i) const 
      {out.put(nav.at==i?'>':' ');Base::printItem(out,i);}
  };
};

//a predicate to search for IsNavCursor tag
struct IsNavCursor {
  template<typename O,typename =void> struct Check {
    static constexpr const bool value{false};
  };
  template<typename O> struct Check<O,std::void_t<typename O::IsNavCursor>> {
    static constexpr const bool value{O::IsNavCursor::value};
  };
};

/// a title = text + newline
template<typename... OO>
using Title=ItemDef<Text,NL,OO...>;

/// a prompt = Text + newline + action function
template<ActionFunc func,typename... OO>
using Prompt=ItemDef<FlatCursor<myNav>,Text,NL,Action<func>,OO...>;

auto menu=menuDef<Id<1>>(
  Title<Id<2>>{"Main menu"},
  staticBody(
    Prompt< op1,Id<11>>{"Option 1"},
    Prompt< op2,Id<12>>{"Option 2"},
    Prompt< op3,Id<13>>{"Option 3"},
    Prompt<quit,Id<10>>{"Quit"}
  )
);

OutDef<ConsoleOut> out;

int main() {
  cout<<std::boolalpha;
  menu.printMenu(out);
  cout<<"has id  1:"<<query<SameAs<Id<1>>,decltype(menu)><<endl;
  cout<<"has id  2:"<<query<SameAs<Id<2>>,decltype(menu)><<endl;
  cout<<"has id  3:"<<query<SameAs<Id<3>>,decltype(menu)><<endl;
  cout<<"has id 13:"<<query<SameAs<Id<13>>,decltype(menu)><<endl;
  cout<<"has IsNavCursor tag:"<<query<IsNavCursor,decltype(menu)><<endl;
  return 0;
}