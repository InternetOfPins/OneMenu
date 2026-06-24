#pragma once

/**
 * @file asyncNav.h
 * @brief AsyncNav — nav component for stateless path-based navigation.
 *
 * Adds async(path) to any nav chain. Designed for web/HTTP use where each
 * request arrives with a full absolute path and the nav can jump freely.
 *
 * Path format: "/idx/idx/.../leafIdx/" — all indices 0-based, trailing '/'.
 * The leaf item is selected but not entered; the caller decides what to do:
 *
 *   webNav.async("/1/3/");                        // navigate to submenu 1, select item 3
 *   webNav.doCmd<false>(Cmd::Enter);              // click — triggers action or edit mode
 *
 * Web nav should be a separate instance sharing the same Root<menu>:
 *
 *   NavDef<AsyncNav, TreeNav, Root<menu>> webNav;
 *   NavDef<TreeNav,  Root<menu>>          hwNav;   // unaffected
 */

#include "oneMenu/menu/nav.h"

namespace oneMenu {

  /// @brief stateless path-based nav component for web/HTTP use; async(path) jumps freely by absolute index path
  struct AsyncNav {
    template<typename N>
    struct Part : N {
      using Base = N;

      // Set value on the currently-focused field by string.
      // Call after async() has positioned the nav at the target item.
      // Returns true if the field handled it (TextField: strcpy, NumField: parsed+clamped).
      bool set(const char* s) {
        return root().setStr(obj(), s, focus(level()+1));
      }

      // Navigate to the path position. Resets to root first — safe to call
      // from any current position. Returns false only if path is null.
      bool async(const char* path) {
        if (!path) return false;

        // reset to root — no event firing, just direct state switch
        while (Base::level() > 0)
          Base::template doCmd<false>(Cmd::Esc);

        if (*path == '/') path++;  // skip leading '/'

        while (*path) {
          // parse one decimal index
          Sz n = 0;
          while (*path >= '0' && *path <= '9')
            n = Sz(n * 10u + unsigned(*path++ - '0'));

          if (*path == '/') path++;  // consume separator

          if (*path == '\0') {
            // last segment — select without entering; caller issues Enter
            Base::go(n);
            return true;
          }
          // intermediate segment — enter this submenu and descend
          Base::go(n);
          Base::template doCmd<false>(Cmd::Enter);
        }
        return true;
      }
    };
  };

} // oneMenu
