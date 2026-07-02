/**
 * @file fonts.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief compile-time cascading font table — same shape as Color<Cor>, one value
 *        per leaf instead of an fg/bg pair.
*/

#pragma once

#include "oneMenu/menu/sys/base.h"

namespace oneMenu {

  /// @brief compile-time font table: one Value<v> at the root cascades through the
  /// whole enabled x selected x role matrix, same skeleton as Color<Cor>. Override
  /// only the branch that needs to differ.
  /// @tparam Fnt font selector type (e.g. bool for big/normal; a font-id enum later)
  template<typename Fnt>
  struct Font {
    /// @brief a compile-time font value — a pure tag type, no storage.
    template<Fnt v> struct Value {};

    /// @brief per-role bundle for one state. Field/EditMode default from Body.
    template<typename Bd, typename Fld=Bd, typename Ed=Fld>
    struct Item {
      using Body=Bd;
      using Field=Fld;
      using EditMode=Ed;
    };

    /// @brief focus/blur: Selected (focused/highlighted) defaults to Item's own font.
    /// Sel defaults to It::Body (a leaf Value<v>), not It itself (the whole Item<...> struct)
    /// — same fix as Color<Cor>::Enabled, see colors.h.
    template<typename It, typename Sel=typename It::Body>
    struct Enabled { using Item=It; using Selected=Sel; };

    /// @brief enabled/disabled: Disabled defaults to the same as Enabled.
    template<typename En, typename Dis=En>
    struct Nav { using Enabled=En; using Disabled=Dis; };

    /// @brief the whole table. A single Value<v> as Title cascades everywhere
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

  /// @brief marker component: carries a Font<Fnt>::Table<...> type, zero runtime
  /// behavior. Drop anywhere below a font-aware format (e.g. GfxFmt) in the output
  /// chain to override it; omit it and that format's own built-in default applies.
  template<typename Table>
  struct FontTable {
    using Type=Table;
    template<typename O> using Part=O;
  };

};//namespace oneMenu
