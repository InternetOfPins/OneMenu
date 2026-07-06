#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/sys/colors.h"
#include "oneMenu/menu/sys/fonts.h"

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
  //
  // Fonts: same Font<Fnt>::Table<...> mechanism, with Fnt=bool (big/normal — all the
  // driver currently supports via setBigFont(bool)). BigTitle still works exactly as
  // before (it's just the default table's Title value now); override per-role/state
  // via a FontTable<...> placed below GfxFmt (enforced, see rules()).
  /// @brief pixel-display format: inverted-video selection, optional big title font, item spacing
  template<Sz Radius=2, Sz Spacing=0, bool BigTitle=false>
  struct GfxFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "GfxFmt: printer layers must be placed above GfxFmt");
      static_assert(Requires<IsCursor,  After>, "GfxFmt: Cursor must be placed below GfxFmt");
      static_assert(Excludes<hapi::IsInstanceOf<ColorTable>, Before>,
        "GfxFmt: ColorTable<> must be placed below GfxFmt (or omitted to use the built-in default)");
      static_assert(Excludes<hapi::IsInstanceOf<FontTable>, Before>,
        "GfxFmt: FontTable<> must be placed below GfxFmt (or omitted to use the built-in default)");
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

      // built-in default: Title uses BigTitle (preserves the old template-arg behavior
      // exactly), everything else normal — cascades from Default below.
      using DefaultFonts = typename Font<bool>::template Table<
        /*Title*/   typename Font<bool>::template Value<BigTitle>,
        /*Default*/ typename Font<bool>::template Value<false>
      >;

      using FoundFont = typename hapi::FindFirstOr<hapi::IsInstanceOf<FontTable>, FontTable<DefaultFonts>>
                          ::template Check<typename O::Types>;
      using PF = typename FoundFont::Type;

      // unwrap a compile-time Value<v> tag into its bool — constexpr so callers can
      // still use it in `if constexpr` (the table is fully resolved at compile time).
      template<bool v>
      static constexpr bool big(typename Font<bool>::template Value<v>) {return v;}

      using PFNavEn  = typename PF::Nav::Enabled;
      using PFNavDis = typename PF::Nav::Disabled;

      // Item content's own big/normal choice — same enabled x selected shape as
      // itemInverted()/Color<Cor>, read from the font table instead. Runtime (not
      // `if constexpr` like Title's, which is a single fixed BigTitle value) because it can
      // legitimately differ per item once a FontTable<> distinguishes Selected from Item::Body.
      static bool itemBig(const Ctx& ctx) {
        return ctx.enabled
          ? (ctx?big(typename PFNavEn::Selected{}):big(typename PFNavEn::Item::Body{}))
          : (ctx?big(typename PFNavDis::Selected{}):big(typename PFNavDis::Item::Body{}));
      }

      // Per-role big/normal choice — Font<Fnt>::Item<Body,Field,EditMode> (fonts.h) already
      // has separate slots for this; nothing previously read Field/EditMode specifically
      // (every fmtStart/fmtStop below was Fmt::Item/Fmt::Title-granular, one font decision
      // per whole item). Deliberately bypasses the Selected/focus branch itemBig() uses —
      // "is this item currently focused" is orthogonal to "which role within it is this
      // text" (a FullScreen item is always the focused one when it's shown at all, so
      // routing Label/Field through Selected would make them indistinguishable). Unit has
      // no dedicated slot — it inherits Field's (same row, same size), and unlike
      // Label/Field/EditMode below, Unit gets no fmtStart override at all: it's always a
      // continuation of whatever Field just established, never a fresh-row start.
      template<Fmt tag>
      static bool roleBig(const Ctx& ctx) {
        if constexpr(tag&Fmt::Label)
          return ctx.enabled ? big(typename PFNavEn::Item::Body{}) : big(typename PFNavDis::Item::Body{});
        else if constexpr(tag&Fmt::EditMode)
          return ctx.enabled ? big(typename PFNavEn::Item::EditMode{}) : big(typename PFNavDis::Item::EditMode{});
        else // Field
          return ctx.enabled ? big(typename PFNavEn::Item::Field{}) : big(typename PFNavDis::Item::Field{});
      }

      // Unconditional — deterministic solely from ctx/tag, matching Fmt::Item's own
      // fillRect+setBigFont pattern below, NOT comparing against the driver's current
      // _big_font state (lineHeight()>1). That comparison was the actual bug (found via
      // real hardware + Rui's own diagnosis, 2026-07-06): setBigFont() isn't Gate-guarded
      // (Gate's Part only wraps put/fillRect/setPos/etc — see out.h — never setBigFont),
      // so it takes real, permanent effect even during a Measure/Changed probe pass.
      // StaticBody::changed() (staticBody.h) checks *every* item's changed(), regardless
      // of which one is actually selected/visible — so a probe pass touching an unrelated
      // Rows-based item's Field/Unit could flip the *real* driver font state, and nothing
      // guaranteed it flipped back before the next real draw of a *different*, currently-
      // visible item. Comparing to live state made the decision depend on that leaked
      // history instead of being a pure function of this call's own tag/ctx — always
      // fillRect+setBigFont+setPos here (same as Fmt::Item) removes the dependency
      // entirely: same tag+ctx always does the same thing, no matter what ran before.
      // Restores position to `p` (where it actually was), NOT orgX — two distinct real
      // bugs found on real hardware chasing this, in order:
      // 1. First version reset to {orgX,p.y} unconditionally — clobbered Row's own
      //    centering (Row::rowPrint's out.setPos({start.x+leftW+centerOffset,...}) before
      //    calling centerFn()), always yanking content back to the left edge.
      // 2. Removing the setPos entirely (to fix #1) broke it a different way:
      //    Ssd1306::fillRect (OneIO) updates the *driver's own* internal cursor
      //    (_col=col_pix+w_pix, _page=...) to the far edge of whatever it just filled —
      //    a real device-level side effect, separate from the logical Cursor's m_at
      //    (fillRect doesn't touch that). With no setPos after fillRect, the physical
      //    device cursor is left at the fill's far edge while m_at still (correctly)
      //    says `p` — next character print goes to the *device's* stale position, not
      //    where the logical cursor claims to be. Same class of physical/logical resync
      //    gap as ItemPrinter::printItem's own `setPos(getPos())` idiom (printers.h) for
      //    a force-unlocked row. Fix: restore to `p` specifically (not orgX) — re-syncs
      //    the device to the position Row/Rows actually established, wherever that is.
      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Label|Fmt::Field|Fmt::EditMode)>
      fmtStart(const Ctx& ctx) {
        bool wantBig = roleBig<tag>(ctx);
        Pos p = Base::obj().getPos();
        Base::fillRect(Base::orgX(), p.y, Base::width(), wantBig?2:1);
        Base::setBigFont(wantBig);
        Base::setPos(p);
        Base::template fmtStart<tag>(ctx);
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
        // Unconditional setBigFont (both branches), not just "if big, turn on" — same
        // determinism fix as Fmt::Item/roleBig below: setBigFont() isn't Gate-guarded, so
        // leaving the false-branch implicit ("just don't touch it") let a *different*
        // item's leftover true state (from an unrelated probe pass — see roleBig's own
        // comment) silently leak into this title's rendering.
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
        Base::setInverted(itemInverted(ctx));   // set before fillRect — driver XORs the fill byte
        bool bigItem = itemBig(ctx);
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), bigItem?2:1);
        // Unconditional (both true/false), not "if(bigItem) setBigFont(true)" — see roleBig's
        // comment above for why comparing/skipping based on current state is unsafe here.
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
          if constexpr(big(typename PF::Title{})) Base::obj().nl(); // extra page for 2-page big title
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
        // Live state, not itemBig(ctx)'s compile-time Item::Body guess: a per-role
        // fmtStart (Fmt::Field/Fmt::Unit above) may have left big-font on for a reason
        // itemBig() (Body-only) never sees — e.g. a small-label item whose Field/Unit
        // row was big. lineHeight()>1 already reflects the driver's real _big_font state.
        bool bigItem = Base::obj().lineHeight()>1;
        if(bigItem) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if(bigItem) Base::obj().nl();  // extra page for 2-page big item, same as big title
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
