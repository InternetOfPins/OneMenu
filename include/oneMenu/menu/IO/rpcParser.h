#pragma once

/**
 * @file rpcParser.h
 * @brief Raw RPC byte → Cmd::Go dispatcher for 433 MHz radio RPC nodes.
 *
 * Maps an 8-bit radio command byte to a Cmd::Go event so IndexGo can route
 * it to the matching ItemDef<Id<N>, Action<fn>> in a flat dispatch table.
 *
 * The byte is split into (CmdBits) command ID bits and (8-CmdBits) value bits:
 *   CmdBits=0 : entire byte = value, always Go(0)   — single-output nodes
 *   CmdBits=2 : upper 2 bits = cmd (0-3), lower 6 = value — 4-command nodes
 *   CmdBits=4 : upper 4 bits = cmd (0-15), lower 4 = value
 *
 * The value extracted from each packet is stored in RpcParser<N>::value and
 * can be read by the dispatched action.
 *
 * Usage:
 *   using Rpc = RpcParser<2>;
 *   InDef<Rf433In<0>, Rpc> in;
 *   NavDef<IndexGo, TreeNav, Root<...>> nav;
 */

#include "oneMenu/menu/in.h"

namespace oneMenu {
  //i would ike to use OneParse here and see the difference
  template<uint8_t CmdBits=2>
  struct RpcParser {
    inline static uint8_t value = 0;

    template<typename O>
    struct Part : O {
      static CKE parseKey(Key k) {
        if constexpr (CmdBits == 0) {
          RpcParser::value = uint8_t(k);
          return {Cmd::Go, Key(0), false, false};
        } else {
          constexpr uint8_t shift = 8 - CmdBits;
          constexpr uint8_t vmask = uint8_t((1u << shift) - 1u);
          RpcParser::value = uint8_t(k) & vmask;
          return {Cmd::Go, Key(uint8_t(k) >> shift), false, false};
        }
      }
    };
  };

} // oneMenu
