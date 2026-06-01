/**
 * @file in.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief input API
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/
#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {
  //input--
  template<typename K>
  struct InAPI:K {
    using Base=K;
    static constexpr bool available() {return false;}
    template<typename Nav> static constexpr bool parseKey(Nav& nav,Key k) {return false;}
  };

  template<typename... KK>
  struct InDef:APIOf<InAPI<Nil>,KK...>{
    using Base=APIOf<InAPI<Nil>,KK...>;
  };

};//namespace oneMenu
