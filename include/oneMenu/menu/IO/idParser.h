#pragma once

/**
 * @file idParser.h
 * @brief Digit '1'-'9' → Cmd::Go shortcut parser.
 *
 * Translates digit keys to a Cmd::Go event carrying the 1-based item index.
 * The nav side must include IndexGo (nav.h) to consume Cmd::Go events.
 *
 *   NavDef<IndexGo, TreeNav, Root<...>> nav;
 *   InDef<LinuxKeyIn, IdParser, PCKbd>  in;  // digits jump to item N directly
 */

#include "oneMenu/menu/in.h"

namespace oneMenu {

  /// @brief maps digit keys '1'-'9' to Cmd::Go; nav must include IndexGo to consume it
  struct IdParser {
    template<typename O>
    struct Part : O {
      static CKE parseKey(Key k) {
        if (k >= '1' && k <= '9')
          return {Cmd::Go, Key(k - '0'), false, false};
        return O::parseKey(k);
      }
    };
  };

} // oneMenu
