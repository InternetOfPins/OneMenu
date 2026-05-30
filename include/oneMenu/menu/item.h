/**
 * @file item.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * 
*/

#pragma once

// struct Nil {};

// data items ----------------------------------------------------------------

template<typename T,T data>
struct StaticData {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    using Type=decltype(data);
    static constexpr const Type& get() {return data;}
    constexpr void set(std::decay_t<Type> o){data=std::move(o);}
    template<typename Out> void print(Out& out) const {out<<data;Base::print(out);}
  };
};

template<typename T>
struct Data {
  template<typename O>
  struct Part:O {
    using Base=O;
    using Base::Base;
    using Type=T;
    Type data;

    template<typename... OO> constexpr Part(T&& o,OO&&... oo):Base{std::forward<OO>(oo)...},data{o}{}

    constexpr Type& get() {return data;}
    constexpr void set(Type o){data=o;}

    template<typename Out,typename... PP>
    void print(Out& out,const PP&... pp) const 
      {out.put(data);Base::print(out,pp...);}
  };
};

using Text=Data<const char*>;
using Int=Data<int>;
using Bool=Data<bool>;
using Char=Data<unsigned char>;
template<const char*& text> using TextRef=StaticData<const char*,text>;

//some parts ----------------------------------------------------
template<int id> struct Id {template<typename O> using Part=O;};

using ActionFunc=bool(&)(int);

template<ActionFunc action>
struct Action {
  template<typename I>
  struct Part:I {
    using Base=I;
    using Base::Base;
    static constexpr bool act(int i) {return action(i);}
    template<bool isKbd,typename Nav>
    static constexpr bool nav(Nav& n,const CKE& cke,Path path) 
      {return cke.cmd==Cmd::Enter&&action(path.sel());}
  };
};

//attach an action on enter
template <ActionFunc f>
struct BodyAction {
  template <typename I>
  struct Part : I {
    using Base=I;
    using Base::Base;
    using Base::enabled;
    template<bool isKbd,typename Nav>
    bool nav(Nav& n,const CKE& cke,const Path p) {
      if(cke.cmd==Cmd::Enter&&p.len) f(p.last());
      return Base::template nav<isKbd>(n,cke,p);
    }
  };
};

