#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/sys/colors.h"
#include "oneMenu/menu/sys/fonts.h"

namespace oneMenu {

  // GfxColorFmt<Radius, Spacing, BigTitle>
  // Real-color sibling of GfxFmt, for pixel-addressed vendor GFX devices whose
  // driver has a genuine setColors(fg,bg) (AdaGfxVendor/AdaGfxBufferedVendor —
  // see oledOut.h's HasSetColors gate). Same overall shape/plumbing as GfxFmt
  // (fillRect/setBigFont/setPos, Row/Field/EditMode handling, Spacing
  // separators, Font<bool> table for big/normal font) — deliberately NOT
  // merged into GfxFmt itself, same "structurally different case gets its own
  // path" convention ANSIFmt/JsonFmt/XmlFmt already follow as siblings rather
  // than one type forking on a color-model NTTP.
  //
  // Colors: same Color<Cor>::Table<...> cascading mechanism as ANSIFmt, with
  // Cor=uint16_t (RGB565) instead of ANSI's int codes or GfxFmt's bool
  // (inverted?). A Colors<f,b> leaf is a real {fg,bg} pair applied via
  // setColors(fg,bg) — no invert trick needed, the selected/body/field/
  // edit-mode palette entries differ directly. Override via a ColorTable<...>
  // placed below GfxColorFmt (enforced, see rules()); omit it and the
  // built-in DefaultPalette below applies (neutral white-on-black, inverted
  // for selected — not tuned to any particular example's hardware).
  //
  // Fonts: identical Font<Fnt>::Table<...> mechanism to GfxFmt, copied
  // verbatim — orthogonal to color, unchanged here.
  /// @brief pixel-display format with real per-role/state RGB565 colors (see GfxFmt for the invert-only sibling)
  template<Sz Radius=2, Sz Spacing=0, bool BigTitle=false>
  struct GfxColorFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "GfxColorFmt: printer layers must be placed above GfxColorFmt");
      static_assert(Requires<IsCursor,  After>, "GfxColorFmt: Cursor must be placed below GfxColorFmt");
      static_assert(Excludes<hapi::IsInstanceOf<ColorTable>, Before>,
        "GfxColorFmt: ColorTable<> must be placed below GfxColorFmt (or omitted to use the built-in default)");
      static_assert(Excludes<hapi::IsInstanceOf<FontTable>, Before>,
        "GfxColorFmt: FontTable<> must be placed below GfxColorFmt (or omitted to use the built-in default)");
      return true;
    }
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::fmtStart;
      using Base::fmtStop;

      Pos m_itemPos{0,0};

      // built-in default: neutral white-on-black, inverted for selected —
      // not tuned to any specific hardware/example, override via ColorTable<>.
      using DefaultPalette = typename Color<uint16_t>::template Table<
        /*Title*/   typename Color<uint16_t>::template Colors<0xFFFF,0x0000>,
        /*Default*/ typename Color<uint16_t>::template Colors<0xFFFF,0x0000>,
        /*View*/    typename Color<uint16_t>::template Colors<0xFFFF,0x0000>,
        /*Nav*/ typename Color<uint16_t>::template Nav<
          /*Enabled*/ typename Color<uint16_t>::template Enabled<
            typename Color<uint16_t>::template Item<typename Color<uint16_t>::template Colors<0xFFFF,0x0000>>,
            /*Selected*/ typename Color<uint16_t>::template Colors<0x0000,0xFFFF>
          >
        >
      >;

      using Found = typename hapi::FindFirstOr<hapi::IsInstanceOf<ColorTable>, ColorTable<DefaultPalette>>
                      ::template Check<typename O::Types>;
      using P = typename Found::Type;
      using NavEn  = typename P::Nav::Enabled;
      using NavDis = typename P::Nav::Disabled;

      // unwrap a compile-time Colors<f,b> tag into the runtime {fg,bg} pair setColors() needs
      template<uint16_t f,uint16_t b>
      static constexpr Colors<uint16_t> unwrap(typename Color<uint16_t>::template Colors<f,b>) {return {f,b};}

      // same enabled x selected branching as GfxFmt::itemInverted, resolving a real
      // {fg,bg} pair instead of a bool.
      static Colors<uint16_t> fb(const Ctx& ctx) {
        return ctx.enabled
          ? (ctx?unwrap(typename NavEn::Selected{}):unwrap(typename NavEn::Item::Body{}))
          : (ctx?unwrap(typename NavDis::Selected{}):unwrap(typename NavDis::Item::Body{}));
      }

      // built-in default: Title uses BigTitle (preserves GfxFmt's own template-arg
      // behavior exactly), everything else normal — cascades from Default below.
      using DefaultFonts = typename Font<bool>::template Table<
        /*Title*/   typename Font<bool>::template Value<BigTitle>,
        /*Default*/ typename Font<bool>::template Value<false>
      >;

      using FoundFont = typename hapi::FindFirstOr<hapi::IsInstanceOf<FontTable>, FontTable<DefaultFonts>>
                          ::template Check<typename O::Types>;
      using PF = typename FoundFont::Type;

      template<bool v>
      static constexpr bool big(typename Font<bool>::template Value<v>) {return v;}

      using PFNavEn  = typename PF::Nav::Enabled;
      using PFNavDis = typename PF::Nav::Disabled;

      static bool itemBig(const Ctx& ctx) {
        return ctx.enabled
          ? (ctx?big(typename PFNavEn::Selected{}):big(typename PFNavEn::Item::Body{}))
          : (ctx?big(typename PFNavDis::Selected{}):big(typename PFNavDis::Item::Body{}));
      }

      // per-role big/normal choice — copied verbatim from GfxFmt, unrelated to color.
      template<Fmt tag>
      static bool roleBig(const Ctx& ctx) {
        if constexpr(tag&Fmt::Label)
          return ctx.enabled ? big(typename PFNavEn::Item::Body{}) : big(typename PFNavDis::Item::Body{});
        else if constexpr(tag&Fmt::EditMode)
          return ctx.enabled ? big(typename PFNavEn::Item::EditMode{}) : big(typename PFNavDis::Item::EditMode{});
        else // Field
          return ctx.enabled ? big(typename PFNavEn::Item::Field{}) : big(typename PFNavDis::Item::Field{});
      }

      // per-role color choice — same Body/Field/EditMode granularity as roleBig above,
      // deliberately bypassing the Selected branch for the same reason roleBig does
      // ("is this item focused" is orthogonal to "which role within it is this text").
      template<Fmt tag>
      static Colors<uint16_t> roleColors(const Ctx& ctx) {
        if constexpr(tag&Fmt::Label)
          return ctx.enabled ? unwrap(typename NavEn::Item::Body{}) : unwrap(typename NavDis::Item::Body{});
        else if constexpr(tag&Fmt::EditMode)
          return ctx.enabled ? unwrap(typename NavEn::Item::EditMode{}) : unwrap(typename NavDis::Item::EditMode{});
        else // Field
          return ctx.enabled ? unwrap(typename NavEn::Item::Field{}) : unwrap(typename NavDis::Item::Field{});
      }

      // Set colors before fillRect — driver reads current color state at fill time
      // (same ordering rule GfxFmt documents for setInverted). Otherwise identical
      // to GfxFmt's own fmtStart<Label|Field|EditMode> — see its comments for the
      // "clear from current position, restore to p not orgX" rationale.
      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Label|Fmt::Field|Fmt::EditMode)>
      fmtStart(const Ctx& ctx) {
        auto c = roleColors<tag>(ctx);
        Base::setColors(c.fg, c.bg);
        bool wantBig = roleBig<tag>(ctx);
        Pos p = Base::obj().getPos();
        Base::fillRect(p.x, p.y, Base::width()-p.x, wantBig?2:1);
        Base::setBigFont(wantBig);
        Base::setPos(p);
        Base::template fmtStart<tag>(ctx);
      }

      // fully suppress NavCursor — no space, no '>' — real color is the indicator
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStart(const Ctx&) {}
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStop(const Ctx&)  {}

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStart(const Ctx& ctx) {
        { auto c=unwrap(typename P::Title{}); Base::setColors(c.fg,c.bg); }
        m_itemPos = Base::obj().getPos();
        if constexpr(big(typename PF::Title{})) {
          Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 2);
        } else {
          Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), 1);
        }
        Base::setBigFont(big(typename PF::Title{}));
        Base::setPos({Base::orgX(), m_itemPos.y});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStart(const Ctx& ctx) {
        m_itemPos = Base::obj().getPos();
        auto c = fb(ctx);
        Base::setColors(c.fg, c.bg);   // set before fillRect — driver reads state at fill time
        bool bigItem = itemBig(ctx);
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), bigItem?2:1);
        Base::setBigFont(bigItem);
        Base::setPos({Base::orgX(), m_itemPos.y});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStop(const Ctx& ctx) {
        if constexpr(big(typename PF::Title{})) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if constexpr(big(typename PF::Title{})) Base::obj().nl();
        }
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStop(const Ctx& ctx) {
        // always reset to the (possibly customized) non-selected color — ctx may
        // evaluate differently in stop vs start, so re-derive rather than hardcode
        auto c = ctx.enabled ? unwrap(typename NavEn::Item::Body{}) : unwrap(typename NavDis::Item::Body{});
        Base::setColors(c.fg, c.bg);
        bool bigItem = Base::obj().lineHeight()>1;
        if(bigItem) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if(bigItem) Base::obj().nl();
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
          auto c = unwrap(typename P::View{});
          Base::setColors(c.fg, c.bg);
          auto f = Base::obj().free();
          if(f.y > 0) Base::fillRect(Base::orgX(), Base::obj().getPos().y, Base::width(), f.y);
        }
      }
    };
  };

} // namespace oneMenu
