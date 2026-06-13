/**
 * @file oneMenu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi based menu system
 * @version 5
*/

#pragma once

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/fields.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/body/staticBody.h"
#include "oneMenu/menu/out.h"
#include "oneMenu/menu/item.h"
#include "oneMenu/menu/menu.h"
#include "oneMenu/menu/nav.h"

//factories -----------------------------------------------
namespace oneMenu {
  template<typename... OO,typename T,typename B,typename... PP> 
  constexpr MenuDef<T,B,OO...> menuDef(T&& t,B&& b,PP&&... pp)
    {return {std::forward<T>(t),std::forward<B>(b),std::forward<PP>(pp)...};}

  template<typename... OO,typename T,typename B,typename... PP> 
  constexpr PadMenu<T,B,OO...> padDef(T&& t,B&& b,PP&&... pp)
    {return {std::forward<T>(t),std::forward<B>(b),std::forward<PP>(pp)...};}
};
