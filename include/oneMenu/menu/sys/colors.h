/**
 * @file colors.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief compile-time cascading color table
*/

#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief compile-time color table: one Colors<f,b> pair at the root cascades
  /// through the whole enabled x selected x role matrix. Override only the
  /// branch that needs to differ; everything else defaults from its parent.
  /// @tparam Cor color value type (e.g. int for ANSI codes)
  template<typename Cor>
  struct Color {
    /// @brief a compile-time fg/bg pair — a pure tag type, no storage.
    template<Cor f, Cor b> struct Colors {};

    /// @brief per-role bundle for one state. Field/EditMode default from Body.
    template<typename Bd, typename Fld=Bd, typename Ed=Fld>
    struct Item {
      using Body=Bd;
      using Field=Fld;
      using EditMode=Ed;
    };

    /// @brief focus/blur: Selected (focused/highlighted) defaults to Item's own colors.
    /// Sel defaults to It::Body (a leaf Colors<f,b>), not It itself (the whole Item<...>
    /// struct) — every consumer of ::Selected (e.g. GfxFmt::itemInverted, ANSIFmt::fb) expects
    /// a leaf, and It is always an Item<...> by construction at every real call site.
    template<typename It, typename Sel=typename It::Body>
    struct Enabled { using Item=It; using Selected=Sel; };

    /// @brief enabled/disabled: Disabled defaults to the same as Enabled.
    template<typename En, typename Dis=En>
    struct Nav { using Enabled=En; using Disabled=Dis; };

    /// @brief the whole table. A single Colors<f,b> as Title cascades everywhere
    /// (Default<-Title, View<-Default, Nav<-Nav<Enabled<Item<Default>>>).
    template<
      typename Tit,
      typename Def=Tit,
      typename Vw=Def,
      typename Nv=Nav<Enabled<Item<Def>>>
    >
    struct Table {
      using Title=Tit;
      using Default=Def;
      using View=Vw;
      using Nav=Nv;
    };
  };

  /// @brief marker component: carries a Color<Cor>::Table<...> type, zero runtime
  /// behavior. Drop anywhere below ANSIFmt in the output chain to override its
  /// palette; omit it and ANSIFmt falls back to its own built-in default table.
  template<typename Table>
  struct ColorTable {
    using Type=Table;
    template<typename O> using Part=O;
  };

};//namespace oneMenu
