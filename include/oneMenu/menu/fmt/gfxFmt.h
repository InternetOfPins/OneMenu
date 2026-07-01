#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/sys/colors.h"

namespace oneMenu {

  // GfxFmt<Radius, Spacing>
  // Format for GFX-capable devices (HasFillRect). Works in device-native coords
  // (pixels horizontally, pages vertically for SSD1306).
  //
  // Selection indicator: inverted video — fillRect(0x00 → 0xFF via _inv) + XOR'd font bytes.
  // NavCursor: fully suppressed — no space, no '>' character. Inverted video IS the indicator.
  //
  // Per item fmtStart: setInverted(...) from the color table (default: true if selected,
  //   false otherwise — ctx.enabled unused by the built-in default, but reachable via
  //   Color<bool>::Table's Nav::Disabled branch for anyone who wants to customize it),
  //   then fillRect clears/fills the row.
  // Per item fmtStop:  setInverted back to the (possibly customized) non-selected state,
  //   nl(), optional Spacing blank page (cleared).
  // Per title fmtStart: fillRect clears the title row.
  // After view fmtStop: fillRect clears remaining rows below last item.
  //
  // Radius: reserved for future drawRoundRect use (API compat).
  // Spacing: extra blank pages between items (0 = packed).
  //
  // Colors: same Color<Cor>::Table<...> mechanism as ANSIFmt, with Cor=bool — a
  // Colors<f,b> leaf's f is "inverted?" (b unused, kept only for shape symmetry with
  // the color-capable formats). Override via a ColorTable<...> placed below GfxFmt
  // (enforced, see rules()); omit it and the built-in default below applies (identical
  // to the previous hardcoded invert-only-when-selected behavior).
  /// @brief pixel-display format: inverted-video selection, optional big title font, item spacing
  template<Sz Radius=2, Sz Spacing=0, bool BigTitle=false>
  struct GfxFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "GfxFmt: printer layers must be placed above GfxFmt");
      static_assert(Requires<IsCursor,  After>, "GfxFmt: Cursor must be placed below GfxFmt");
      static_assert(Excludes<hapi::IsInstanceOf<ColorTable>, Before>,
        "GfxFmt: ColorTable<> must be placed below GfxFmt (or omitted to use the built-in default)");
      return true;
    }
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::fmtStart;
      using Base::fmtStop;

      Pos m_itemPos{0,0};

      // built-in default: not inverted normally, inverted when selected — reproduces
      // the previous hardcoded behavior exactly (ctx.enabled was never consulted).
      using DefaultPalette = typename Color<bool>::template Table<
        /*Title*/   typename Color<bool>::template Colors<false,false>,
        /*Default*/ typename Color<bool>::template Colors<false,false>,
        /*View*/    typename Color<bool>::template Colors<false,false>,
        /*Nav*/ typename Color<bool>::template Nav<
          /*Enabled*/ typename Color<bool>::template Enabled<
            typename Color<bool>::template Item<typename Color<bool>::template Colors<false,false>>,
            /*Selected*/ typename Color<bool>::template Colors<true,false>
          >
        >
      >;

      using Found = typename hapi::FindFirstOr<hapi::IsInstanceOf<ColorTable>, ColorTable<DefaultPalette>>
                      ::template Check<typename O::Types>;
      using P = typename Found::Type;
      using NavEn  = typename P::Nav::Enabled;
      using NavDis = typename P::Nav::Disabled;

      // unwrap a compile-time Colors<f,b> tag into its "inverted?" bool
      template<bool f,bool b>
      static constexpr bool inverted(typename Color<bool>::template Colors<f,b>) {return f;}

      static bool itemInverted(const Ctx& ctx) {
        return ctx.enabled
          ? (ctx?inverted(typename NavEn::Selected{}):inverted(typename NavEn::Item::Body{}))
          : (ctx?inverted(typename NavDis::Selected{}):inverted(typename NavDis::Item::Body{}));
      }

      // fully suppress NavCursor — no space, no '>' — inverted video is the indicator
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStart(const Ctx&) {}
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStop(const Ctx&)  {}

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStart(const Ctx& ctx) {
        m_itemPos = Base::obj().getPos();
        if constexpr(BigTitle) {
          Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 2);
          Base::setBigFont(true);
        } else {
          Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 1);
        }
        Base::setPos({Base::orgX(), m_itemPos.y});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStart(const Ctx& ctx) {
        m_itemPos = Base::obj().getPos();
        Base::setInverted(itemInverted(ctx));   // set before fillRect — driver XORs the fill byte
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 1);
        Base::setPos({Base::orgX(), m_itemPos.y});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStop(const Ctx& ctx) {
        if constexpr(BigTitle) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if constexpr(BigTitle) Base::obj().nl(); // extra page for 2-page big title
        }
      }

      // Footer handling disabled to match Fmt::Footer being commented out in enums.h
      // template<Fmt tag>
      // std::enable_if_t<tag&Fmt::Footer>
      // fmtStop(const Ctx& ctx) {
      //   if(!ctx.pad) Base::obj().nl();
      // }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStop(const Ctx& ctx) {
        // always reset to the (possibly customized) non-selected state — ctx may
        // evaluate differently in stop vs start, so re-derive rather than hardcode false
        Base::setInverted(ctx.enabled?inverted(typename NavEn::Item::Body{}):inverted(typename NavDis::Item::Body{}));
        if(!ctx.pad) {
          Base::obj().nl();
          if constexpr(Spacing > 0) {
            if(Base::obj().free().y > 0) {  // guard: skip separator if no room (prevents page wrap)
              auto sep = Base::obj().getPos();
              Base::fillRect(Base::orgX(), sep.y, Base::width(), 1, 0x00);
              Base::obj().nl();
            }
          }
        }
      }

      template<Fmt tag>
      std::enable_if_t<tag==Fmt::View>
      fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);
        if(!ctx.pad) {
          auto f = Base::obj().free();
          if(f.y > 0) Base::fillRect(Base::orgX(), Base::obj().getPos().y, Base::width(), f.y);
        }
      }
    };
  };

} // namespace oneMenu
