#pragma once
#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"
#include "oneMenu/menu/sys/colors.h"
#include "oneMenu/menu/sys/fonts.h"
#include "oneMenu/menu/fmt/textFmt.h"

namespace oneMenu {

  // GfxCtrlChars: like CtrlChars (out.h), but for pixel-addressed GFX
  // devices — converts a literal '\n' into a real nl() AND repaints the new
  // row's background using whatever color is CURRENTLY active on the device
  // (already set by GfxColorFmt's own fmtStart<Item>/Label/Field/etc before
  // any text of this item printed). Plain CtrlChars only calls nl(): fine
  // for a scrolling terminal (ANSI already colors each new line as it goes),
  // but on a GFX device a multiline item's 2nd+ lines otherwise show
  // whatever was already in that screen area — found on real ST7789
  // hardware: an item's own selection/body color never extended past its
  // first line.
  //
  // Deliberately its own component, not folded into CtrlChars or
  // GfxColorFmt's default per-item behavior: only menus that actually
  // compose multiline GFX items pay for the extra fillRect-per-line, and
  // where/whether to use it is the menu author's own compositional choice
  // (swap in for plain CtrlChars), not a cost every item pays automatically.
  struct GfxCtrlChars {
    template<typename O>
    struct Part:O {
      using Base=O;
      void put(const char o) {
        if(o=='\n') {
          Base::nl();
          Pos p = Base::obj().getPos();
          Base::fillRect(Base::obj().orgX(), p.y, Base::obj().width(), Base::obj().lineHeight());
        } else Base::put(o);
      }
    };
  };

  // GfxTextWrap: like TextWrap (out.h), but for pixel-addressed GFX devices
  // — same fix idea as GfxCtrlChars above, same reason: plain TextWrap's
  // wrap-triggered nl() has no repaint, so a long line that wraps mid-item
  // shows the same stale-background symptom on its wrapped continuation
  // rows as an unpatched embedded '\n' did. Same "opt-in, pay only if
  // composed" reasoning as GfxCtrlChars — swap in for plain TextWrap.
  struct GfxTextWrap : aParser {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsDataParser,       After>, "GfxTextWrap: DataParser<sz> must be placed above GfxTextWrap");
      static_assert(Excludes<hapi::SameAs<UTF8>, After>, "GfxTextWrap: UTF8 must be placed above GfxTextWrap — wrapping must count characters after surrogate filtering");
      return true;
    }
    template<typename O>
    struct Part:O {
      using IsParser=std::true_type;
      using Base=O;
      inline void put(const char o) {
        if(Base::obj().free().x<=0) {
          Base::nl();
          Pos p = Base::obj().getPos();
          Base::fillRect(Base::obj().orgX(), p.y, Base::obj().width(), Base::obj().lineHeight());
        }
        Base::put(o);
      }
    };
  };

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
      // but NOT bypassing Selected the way roleBig does: roleBig's bypass is correct
      // for FONT SIZE (focus is orthogonal to which role you're in), but wrong for
      // COLOR — selection has to visually highlight the whole row, sub-roles included,
      // same as fb() already does for plain single-Body items. Found on real hardware:
      // without this, a composite item's (NumFieldDef: Label+Field+Unit) Selected
      // highlight never showed at all, since every sub-role kept rendering its own
      // normal Body/Field/EditMode color regardless of focus.
      template<Fmt tag>
      static Colors<uint16_t> roleColors(const Ctx& ctx) {
        if(ctx) return ctx.enabled ? unwrap(typename NavEn::Selected{}) : unwrap(typename NavDis::Selected{});
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
      // EditMode is NOT part of this mask (see its own fmtStart below) — it needs a
      // narrow one-glyph fill, not "cover the rest of the row".
      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Label|Fmt::Field)>
      fmtStart(const Ctx& ctx) {
        auto c = roleColors<tag>(ctx);
        Base::setColors(c.fg, c.bg);
        bool wantBig = roleBig<tag>(ctx);
        Pos p = Base::obj().getPos();
        // height in device coordinates (lineHeight(), not a literal page count) — a
        // pixel-addressed device (VendorGfxOut) needs the real per-row pixel span, not
        // the "1 page = 8px" unit GfxFmt's own literal 1/2 assumes for page-addressed
        // OledOut. See fmtStart<Item> below for the full rationale.
        Base::fillRect(p.x, p.y, Base::width()-p.x, wantBig?Base::lineHeight()*2:Base::lineHeight());
        Base::setBigFont(wantBig);
        Base::setPos(p);
        Base::template fmtStart<tag>(ctx);
      }

      // Real edit-mode separator glyph — same MenuChars ANSIFmt already uses
      // (':'/'='/'.'/' ' for Nav/Edit/Tune/blur), fired wherever the item composes
      // AsEditMode<> (previously color-only here, no visible marker at all on GFX).
      // Narrow one-glyph fillRect, not the full-row-width fill Label/Field use:
      // this device's print() is always transparent-background (AdaGfxVendor never
      // calls the opaque 2-arg setTextColor), so the glyph needs ITS OWN small
      // background patch first or it blends invisibly into whatever Label/Field
      // already painted behind it — but painting the WHOLE remaining row width
      // here would just get immediately overpainted by Field's own fillRect right
      // after anyway.
      template<Fmt tag>
      std::enable_if_t<tag==Fmt::EditMode>
      fmtStart(const Ctx& ctx) {
        auto c = roleColors<tag>(ctx);
        Base::setColors(c.fg, c.bg);
        Pos p = Base::obj().getPos();
        Base::fillRect(p.x, p.y, Base::obj().charWidth(), Base::lineHeight());
        if(ctx && (!ctx.pad||(ctx.sel(ctx.pAt)==ctx.pIdx))) {
          switch(ctx.mode) {
            case NavMode::Nav:  Base::put(MenuChars::sepNav);  break;
            case NavMode::Edit: Base::put(MenuChars::sepEdit); break;
            case NavMode::Tune: Base::put(MenuChars::sepTune); break;
            default: break;
          }
        } else if(!ctx.pad) Base::put(MenuChars::blur);
        Base::template fmtStart<tag>(ctx);
      }

      // fully suppress NavCursor — no space, no '>' — real color is the indicator
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStart(const Ctx&) {}
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::NavCursor>
      fmtStop(const Ctx&)  {}

      // HasItemSpacing<T>: detects an optional LineSpacing<gut,spc> composed
      // somewhere in the chain (out.h) — same gated-optional idiom as
      // HasSetColors/HasSetFont (oledOut.h), falls back to 0 if absent. Checked
      // against the fully-assembled object's type (Base::obj()), not O, since
      // LineSpacing<> is typically composed dead-last (innermost), below where
      // a plain O::-qualified lookup from here would reach.
      //
      // Checks itemSpacing(), NOT lineSpacing() — VendorGfxOut/OledOut
      // already define their OWN, unrelated lineSpacing() (forwards
      // Oled::lineSpacing(), the device's CharH), and since those sit
      // earlier (more derived) in a real chain than LineSpacing<> (composed
      // dead-last), name-hiding would silently resolve to THAT one instead
      // — found on real ST7789 hardware: the selected item's highlight box
      // was padded by 16px, not the intended 1px.
      //
      // itemSpacing() is applied as an explicit top margin (fmtStart, once,
      // before any content prints) and bottom margin (fmtStop, once, after
      // ALL content — including any internal '\n'/wrapped lines — has
      // finished printing) — deliberately NOT baked into the cached
      // lineHeight() itself, which an earlier version of this code did:
      // lineHeight() is what EVERY nl() call reads, including the internal
      // line breaks GfxCtrlChars/GfxTextWrap trigger mid-item for multiline
      // content, so baking the margin in there multiplied it by however many
      // internal lines an item happened to have — found on real hardware, a
      // 3-line item showed 3x the intended margin. Applying it once at each
      // boundary instead keeps the total margin constant regardless of an
      // item's internal line count.
      template<typename T, typename = void>
      struct HasItemSpacing : std::false_type {};
      template<typename T>
      struct HasItemSpacing<T, std::void_t<decltype(std::declval<T&>().itemSpacing())>>
        : std::true_type {};
      Sz lineSpc() {
        using ObjType = std::remove_reference_t<decltype(Base::obj())>;
        if constexpr(HasItemSpacing<ObjType>::value) return Base::obj().itemSpacing();
        else return (Sz)0;
      }

      // Bottom margin: fixed, once, regardless of internal line count — call
      // AFTER whatever nl() advances already happened for this tag's content.
      void bottomMargin() {
        Sz spc = lineSpc();
        if(spc>0) {
          Pos p = Base::obj().getPos();
          Base::fillRect(Base::orgX(), p.y, Base::width(), spc);
          Base::obj().setPos({p.x, p.y+spc});
        }
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStart(const Ctx& ctx) {
        { auto c=unwrap(typename P::Title{}); Base::setColors(c.fg,c.bg); }
        m_itemPos = Base::obj().getPos();
        Sz spc = lineSpc();
        Sz h = big(typename PF::Title{}) ? Base::lineHeight()*2 : Base::lineHeight();
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), spc+h);
        Base::setBigFont(big(typename PF::Title{}));
        Base::setPos({Base::orgX(), m_itemPos.y+spc});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStart(const Ctx& ctx) {
        m_itemPos = Base::obj().getPos();
        auto c = fb(ctx);
        Base::setColors(c.fg, c.bg);   // set before fillRect — driver reads state at fill time
        bool bigItem = itemBig(ctx);
        // height in device coordinates: lineHeight() is whatever Cursor<CharW,LineH>
        // says a row actually spans in THIS device's own y-units — 1 page (8px) for
        // page-addressed OledOut, or the real pixel row height for pixel-addressed
        // VendorGfxOut (e.g. 20px here). A literal "1"/"2" (GfxFmt's own convention,
        // tuned only for OledOut's page semantics) silently fills just 1-2 raw pixels
        // on a pixel-addressed device instead of the whole row — found on real ST7789
        // hardware, the selection highlight was a single barely-visible pixel line.
        Sz h = bigItem?Base::lineHeight()*2:Base::lineHeight();
        Sz spc = lineSpc();
        Base::fillRect(Base::orgX(), m_itemPos.y, Base::width(), spc+h);
        Base::setBigFont(bigItem);
        Base::setPos({Base::orgX(), m_itemPos.y+spc});
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStop(const Ctx& ctx) {
        if constexpr(big(typename PF::Title{})) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if constexpr(big(typename PF::Title{})) Base::obj().nl();
          bottomMargin();
        }
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStop(const Ctx& ctx) {
        // always reset to the (possibly customized) non-selected color — ctx may
        // evaluate differently in stop vs start, so re-derive rather than hardcode
        auto c = ctx.enabled ? unwrap(typename NavEn::Item::Body{}) : unwrap(typename NavDis::Item::Body{});
        Base::setColors(c.fg, c.bg);
        // itemBig(ctx), NOT Base::obj().lineHeight()>1 (GfxFmt's own check, copied
        // verbatim here): that check assumes lineHeight() is a small page count
        // (1 or 2, page-addressed OledOut), where ">1" means "this row's real
        // driver state is big". On a pixel-addressed device lineHeight() is a
        // real pixel height (16, 20+) — ALWAYS >1 — so this fired on every single
        // item, double-nl()'ing every row: fillRect painted 1×lineHeight() but
        // the cursor advanced 2×lineHeight(), leaving an unpainted gap of exactly
        // one row's height before the next item — found on real hardware as a
        // band of raw/leftover GRAM content (striping) between every item, that
        // only a full clear() (level change) happens to paint over rather than
        // actually fix. itemBig(ctx) is the same ctx-only decision fmtStart<Item>
        // already used to size the fillRect in the first place, so it's
        // guaranteed consistent with what was actually painted — narrower than
        // GfxFmt's original live-state check (which also caught a sub-role
        // Field/Unit going big independent of itemBig's Body-only guess), but
        // AdaGfxVendor's setBigFont() is a no-op today so no sub-role can
        // actually diverge from itemBig() here regardless.
        bool bigItem = itemBig(ctx);
        if(bigItem) Base::setBigFont(false);
        if(!ctx.pad) {
          Base::obj().nl();
          if(bigItem) Base::obj().nl();
          if constexpr(Spacing > 0) {
            if(Base::obj().free().y > 0) {  // guard: skip separator if no room (prevents page wrap)
              auto sep = Base::obj().getPos();
              Base::fillRect(Base::orgX(), sep.y, Base::width(), Base::lineHeight(), 0x00);
              Base::obj().nl();
            }
          }
          bottomMargin();
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
