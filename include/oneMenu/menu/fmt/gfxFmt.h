#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  // GfxFmt<Radius, Spacing>
  // Format for GFX-capable devices (HasFillRect). Works in device-native coords
  // (pixels horizontally, pages vertically for SSD1306).
  //
  // Selection indicator: inverted video — fillRect(0x00 → 0xFF via _inv) + XOR'd font bytes.
  // NavCursor: fully suppressed — no space, no '>' character. Inverted video IS the indicator.
  //
  // Per item fmtStart: setInverted(true) if selected, then fillRect clears/fills the row.
  // Per item fmtStop:  setInverted(false), nl(), optional Spacing blank page (cleared).
  // Per title fmtStart: fillRect clears the title row.
  // After view fmtStop: fillRect clears remaining rows below last item.
  //
  // Radius: reserved for future drawRoundRect use (API compat).
  // Spacing: extra blank pages between items (0 = packed).
  /// @brief pixel-display format: inverted-video selection, optional big title font, item spacing
  template<Sz Radius=2, Sz Spacing=0, bool BigTitle=false>
  struct GfxFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "GfxFmt: printer layers must be placed above GfxFmt");
      static_assert(Requires<IsCursor,  After>, "GfxFmt: Cursor must be placed below GfxFmt");
      return true;
    }
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::fmtStart;
      using Base::fmtStop;

      Pos m_itemPos{0,0};

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
        if(ctx) Base::setInverted(true);   // set before fillRect — driver XORs the fill byte
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

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Footer>
      fmtStop(const Ctx& ctx) {
        if(!ctx.pad) Base::obj().nl();
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStop(const Ctx& ctx) {
        Base::setInverted(false);  // always reset — ctx may evaluate differently in stop vs start
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
