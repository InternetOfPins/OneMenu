#pragma once
#include <string.h>
#include <stdio.h>
#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  // One shorthand-command -> target-path mapping entry, e.g. {"#SAVE","/0/10"}
  // or {"#TZ","/1/2/"} (trailing '/' marks "expects a trailing parameter").
  struct CmdEntry { const char* cmd; const char* path; };

  // Translates a compact command string (e.g. a short remote-control token —
  // over a websocket, serial line, or any other narrow-bandwidth channel)
  // against a table of CmdEntry, producing a real nav path to hand to
  // Nav::async()/AsyncNav. When the matched entry's own path ends in '/',
  // whatever trails the command in `input` is appended verbatim (the
  // "expects a parameter" convention, e.g. "#TZ+2" -> "/1/2/+2"); otherwise
  // the path is used as-is and any trailing input is ignored.
  //
  // Returns true (and writes the translated path into buf, sz bytes, always
  // NUL-terminated) on a match; false (buf untouched) means "not a known
  // shorthand" — the caller's own fallback is normally to treat `input`
  // itself as a literal path (nav.async(input)).
  //
  // Deliberately table-driven, not a switch/if-chain: the table is normal
  // data (can live in flash via a constexpr array), and the same helper
  // serves any command vocabulary a sketch defines, not a fixed AM4-parity
  // set — this is a general OneMenu utility, not compat-layer-specific.
  //
  // Matching is strncmp-prefix, first-match-in-table-order — NOT longest-
  // match. If one command string is itself a prefix of another (e.g.
  // "#SAVE" vs "#SAVEWIFI"), list the longer/more specific one FIRST or it
  // will never be reached.
  template<Sz N>
  bool translateCmd(const char* input, const CmdEntry (&table)[N], char* buf, Sz sz) {
    for(Sz n = 0; n < N; n++) {
      Sz cmdLen = (Sz)strlen(table[n].cmd);
      if(strncmp(input, table[n].cmd, cmdLen) == 0) {
        const char* path = table[n].path;
        Sz pathLen = (Sz)strlen(path);
        if(pathLen && path[pathLen - 1] == '/')
          snprintf(buf, sz, "%s%s", path, input + cmdLen);
        else
          snprintf(buf, sz, "%s", path);
        return true;
      }
    }
    return false;
  }

} // namespace oneMenu
