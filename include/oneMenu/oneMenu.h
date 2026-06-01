/**
 * @file oneMenu.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief hapi based menu system
 * @version 5
*/

#pragma once

#include "oneMenu/menu/sys/base.h"
#include "menu/sys/fields.h"
#include "menu/sys/formats.h"
#include "menu/body/staticBody.h"
#include "menu/out.h"
#include "menu/item.h"
#include "menu/menu.h"
#include "menu/nav.h"

//factories -----------------------------------------------
template<typename... OO> 
constexpr StaticBody<OO...> staticBody(OO&&... oo)
  {return StaticBody<OO...>{std::forward<OO>(oo)...};}

template<typename... OO,typename T,typename B,typename... PP> 
constexpr MenuDef<T,B,OO...> menuDef(T&& t,B&& b,PP&&... pp)
  {return {std::forward<T>(t),std::forward<B>(b),std::forward<PP>(pp)...};}

template<typename... OO,typename T,typename B,typename... PP> 
constexpr PadMenu<T,B,PadDraw,OO...> padDef(T&& t,B&& b,PP&&... pp)
  {return {std::forward<T>(t),std::forward<B>(b),std::forward<PP>(pp)...};}

