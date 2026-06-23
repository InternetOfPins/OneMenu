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
  // Input chain terminal.
  // cmd()      — poll for a command event; returns CKE{Cmd::None} = no event.
  // parseKey() — translate a raw key character; terminal silently drops it.
  // available()— true if the component has a pending event ready to deliver.
  template<typename K>
  struct InAPI : K {
    using Base = K;
    static constexpr bool available()       { return false; }
    static constexpr CKE  cmd()             { return {}; }
    static constexpr CKE  parseKey(Key)     { return {}; }
  };

  template<typename... KK>
  struct InDef : APIOf<InAPI<Nil>, KK...> {
    using Base = APIOf<InAPI<Nil>, KK...>;
  };

};//namespace oneMenu
