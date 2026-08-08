/**
 * @file colors.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief compile-time cascading color table
*/

#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief Compile-time color table cascading through the enabled x selected x role matrix; override only the branches that differ.
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

    /// @brief Focus/blur state; Selected defaults to It::Body, a leaf Colors<f,b> (not the whole Item<...>).
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

  /// @brief Marker component carrying a Color<Cor>::Table<...> type (no runtime behavior); place below ANSIFmt to override its palette.
  template<typename Table>
  struct ColorTable {
    using Type=Table;
    template<typename O> using Part=O;
  };

};//namespace oneMenu
