#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  // GfxFmt<Radius, Spacing>
  // Format for GFX-capable devices (HasFillRect + HasDrawRoundRect). No TextFmt needed.
  //
  // Per item fmtStart: fillRect clears the row; if selected, drawRoundRect drawn BEFORE text
  //   (text overwrites the left portion of the border — right side beyond text remains visible)
  // Per item fmtStop:  nl() + Spacing extra nl() for line gap
  // Per title fmtStart: fillRect clears the title row
  // After view fmtStop: fillRect clears remaining rows below last item
  // NavCursor: suppressed (rounded rect IS the selection indicator)
  //
  // Radius: corner radius for selection border (0 = square rect)
  // Spacing: extra blank rows between items (0 = packed)
  template<Sz Radius=2, Sz Spacing=0>
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

      // suppress text NavCursor — rounded rect is the selection indicator
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStart(const Ctx&) { Base::put(' '); }
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStop(const Ctx&)  {}

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStart(const Ctx& ctx) {
        m_itemPos = Base::obj().getPos();
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 1);
        Base::setPos({Base::orgX(), m_itemPos.y});  // fillRect leaves _col=w*6, reset to row start
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
      std::enable_if_t<tag&(Fmt::Title|Fmt::Footer)>
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
