#pragma once

/**
 * @file fourKeyIn.h
 * @brief Four-key ±*/ to Up/Down/Enter/Esc mapper (originally AM4Keys).
 *
 * Sits in the parseKey chain above a raw-byte source (SerialIn, Rf433In…).
 * Maps ASCII +-*/ to navigation commands; all other keys fall through to O.
 *
 *   InDef<SerialIn, FourKeyIn, PCKbd> in;  // serial-driven menu
 *   InDef<Rf433In<INT>, FourKeyIn>    in;  // RF remote
 */

#include "oneMenu/menu/in.h"

namespace oneMenu {

  /// @brief maps +/-/*// keys to Up/Down/Enter/Esc; use above SerialIn or Rf433In
  struct FourKeyIn {
    template<typename O>
    struct Part : O {
      static CKE parseKey(Key k) {
        switch (k) {
          case '+': return {Cmd::Up};
          case '-': return {Cmd::Down};
          case '*': return {Cmd::Enter};
          case '/': return {Cmd::Esc};
          default:  return O::parseKey(k);
        }
      }
    };
  };

} // oneMenu
