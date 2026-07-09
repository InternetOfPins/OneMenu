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
        if (k == '0')
          // '0' can't be encoded as Cmd::Go (0 would collide with go()'s
          // 1-based math, IndexGo::Part::in()) — tagged with key='0' instead
          // so IndexGo can reinterpret it as a literal digit while editing a
          // field; cmd itself stays Cmd::Esc, so every existing Cmd::Esc
          // consumer (none of which read cke.key) is unaffected, including
          // any nav chain with no IndexGo at all.
          return {Cmd::Esc, Key('0'), false, false};
        return O::parseKey(k);
      }
    };
  };

} // oneMenu
