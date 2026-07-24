#pragma once

/**
 * @file jsonFmt.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief JSON format output for OneMenu
 * @version 1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026
 */

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  /// @brief JSON format: object-per-item, all nav attributes as JSON properties
  struct JsonFmt : aFormat, aJsonFmt {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "JsonFmt: printer layers must be placed above JsonFmt");
      return true;
    }
    template<typename O>
    struct Part:Formats::template Part<O> {
      using Base=typename Formats::template Part<O>;

      bool needComma   {false}; // comma before next item in body array
      int  datasec     {0};     // nesting depth inside a data section
      bool inItem      {false}; // inside fmtStart/fmtStop<Item>
      bool inProp      {false}; // inside an open JSON string property
      bool autoLbl     {false}; // auto-started "lbl" for plain item body text
      bool dataOwnsProp{false}; // Data itself opened inProp (vs nested inside Label/Field)
      // Fmt::Menu assumes it's always immediately preceded by View's own
      // `{"view":{` — true for the real, top-level menu, but pad-mode
      // composite fields (menu.h, Menu::Part::printItem's own isPad()
      // branch) re-enter the WHOLE printMenu chain from an ALREADY-OPEN
      // outer Item — that nested Menu needs its own named `,"menu":{...}`
      // wrapper key instead, or its "at"/title/body properties spill
      // directly into the outer item's own object. `inItem` alone can't
      // tell fmtStart from fmtStop apart reliably — by the time a NESTED
      // Menu's own fmtStop fires, the last nested sub-item's own fmtStop
      // already flipped inItem back to false — so the "is this nested"
      // decision is made once, at fmtStart (the one point inItem is
      // trustworthy), and stashed per depth for fmtStop to read back later.
      static constexpr int MaxMenuDepth = 4;
      bool menuWrapStack[MaxMenuDepth]{};
      int  menuDepth{0};
      // Fmt::Option (Toggle/Select's own per-choice <opt><sel>/<fld>/<un></opt>
      // siblings, xmlFmt.h) has no collective wrapper of its own in XML — each
      // fires independently, sibling elements just concatenate. JSON has no
      // such thing (repeated bare "opt" keys in one object is invalid), so
      // this format synthesizes the wrapping array itself: inOptArray tracks
      // "have we opened \"opt\":[ for the CURRENT item yet", inOpt tracks
      // "are we inside one option's own {...} right now", optFirstProp tracks
      // "has this specific option object written a property yet" (so its
      // first property — sel, fld, or un, whichever a given field type prints
      // first — skips the leading comma Field/Unit/Selected otherwise always
      // emit for the plain-item-level case).
      bool inOptArray  {false}; // "opt":[ ... ] opened for the current item
      bool inOpt       {false}; // inside one <opt>'s own {...}
      bool optFirstProp{true};  // next Field/Unit/Selected inside inOpt needs no leading comma

      // put() from CRTP outer — item body text needs auto "lbl" wrap.
      // puts from our own handlers use Base::put() directly, bypassing this.
      template<typename T>
      void put(T v) {
        if(inItem && !inProp) { Base::put(",\"lbl\":\""); inProp=true; autoLbl=true; }
        Base::put(v);
      }
      void put(const char* s, Sz n) {
        if(inItem && !inProp) { Base::put(",\"lbl\":\""); inProp=true; autoLbl=true; }
        Base::put(s,n);
      }

      // Close a pending auto-opened "lbl" string before starting any new
      // tag — bare (non-AsLabel-wrapped) item text (e.g. dateField's own
      // "." separator, StaticText with no Label wrapper) auto-opens "lbl"
      // via put()'s own lazy-open (above), same as XmlFmt's autoCDATA.
      // Without this, a following Field/Unit/attribute-like tag's own
      // fmtStart appends its content INTO the still-open "lbl" string
      // instead of closing it first. Mirrors XmlFmt's closeCDATA().
      void closeAutoLbl() { if(autoLbl) { Base::put('"'); autoLbl=false; inProp=false; } }

      void putPath(const Path& p, Depth s, Depth l) {
        Depth end = (s+l < p.len) ? s+l : p.len-1;
        for(int i=s; i<end; i++) { Base::put('/'); Base::put(p.data[i]); }
        Base::put('/');
      }

      void putItemPath(const Ctx& ctx) {
        putPath(ctx.path, 0, ctx.at);
        Base::put(ctx.idx);
        Base::put('/');
      }

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        // EditCursor tracks a physical device cursor position (out.h's own
        // Cursor<> component) — meaningless for a static JSON snapshot, and
        // TextField::printItem (fields.h) fires it INLINE, mid-string, not
        // at a real property boundary — treating it as a property there
        // would corrupt the field's own "fld" string (mode's own value
        // already conveys "this field is being edited"). No-op here.
        if constexpr(tag==Fmt::EditCursor) return;

        // inline attribute-like tags — value emitted here via Base::put (not intercepted)
        if(tag==Fmt::NavCursor||tag==Fmt::Index||tag==Fmt::EditMode||tag==Fmt::Accel
         ||tag==Fmt::Enabled||tag==Fmt::Choice||tag==Fmt::Dropdown) {
          closeAutoLbl();
          switch(tag) {
            case Fmt::NavCursor:
              Base::put(",\"ncur\":\"");
              Base::put(ctx ? (ctx.enabled ? '@' : '-') : ' ');
              break;
            case Fmt::Index:
              Base::put(",\"idx\":");
              Base::put(ctx.idx); // number — no quotes, fmtStop emits nothing
              break;
            case Fmt::EditMode:
              Base::put(",\"mode\":\"");
              if(ctx&&(!ctx.pad||(ctx.sel(ctx.pAt)==ctx.pIdx))) switch(ctx.mode) {
                case NavMode::Nav:  Base::put("nav");  break;
                case NavMode::Edit: Base::put("edit"); break;
                case NavMode::Tune: Base::put("tune"); break;
              } else Base::put("none");
              break;
            case Fmt::Accel:
              Base::put(",\"acc\":");
              Base::put(ctx.idx);
              break;
            case Fmt::Enabled: // number, not quoted — same convention as idx/acc above
              Base::put(",\"en\":");
              Base::put(ctx.enabled ? 1 : 0);
              break;
            case Fmt::Choice:  // constant marker, no ctx-derived value — see xmlFmt.h's own comment
              Base::put(",\"choice\":1");
              break;
            case Fmt::Dropdown:
              Base::put(",\"dropdown\":1");
              break;
            default: break;
          }
          return;
        }

        if(tag==Fmt::Data) {
          closeAutoLbl();
          if(datasec==0) {
            if(!inProp) {
              // standalone data section — open "data" property
              Base::put(",\"data\":\"");
              inProp=true;
              dataOwnsProp=true;
            }
            // else: nested inside Label/Field — content flows into the enclosing string
          }
          datasec++;
          return;
        }

        if(tag==Fmt::Field||tag==Fmt::Unit||tag==Fmt::Low||tag==Fmt::High||tag==Fmt::Selected) closeAutoLbl();

        // Toggle/Select's own repeated <opt> siblings (xmlFmt.h) — see the
        // inOptArray/inOpt/optFirstProp members' own comment above. Handled
        // separately from the plain switch below since it needs to
        // synthesize a JSON array wrapper XML never needed.
        if(tag==Fmt::Option) {
          closeAutoLbl();
          if(!inOptArray) { Base::put(",\"opt\":["); inOptArray=true; }
          else Base::put(',');
          Base::put('{');
          inOpt=true; optFirstProp=true;
          Base::template fmtStart<tag>(ctx);
          return;
        }

        switch(tag) {
          case Fmt::View:
            Base::put("{\"view\":{");
            break;
          case Fmt::Menu: {
            // inItem is reliable HERE (fmtStart), before any nested item of
            // THIS menu's own body has had a chance to open and flip it —
            // see the menuWrapStack/menuDepth members' own comment above.
            bool nested = inItem;
            if(menuDepth < MaxMenuDepth) menuWrapStack[menuDepth] = nested;
            menuDepth++;
            if(nested) { closeAutoLbl(); Base::put(",\"menu\":{"); }
            Base::put("\"at\":\"");
            putPath(ctx.path, 0, ctx.at>0 ? ctx.at : ctx.pAt);
            Base::put("\",");
            break;
          }
          case Fmt::Title:
            Base::put("\"title\":{\"path\":\"");
            putPath(ctx.path, 0, std::min<Depth>(ctx.at,(Depth)(ctx.path.len-1)));
            Base::put("\",\"text\":\"");
            // Unlike Label/Field/Unit, this never set inProp — harmless for
            // a top-level title (fires before any Item opens, inItem is
            // already false), but a NESTED title (pad-mode fields) fires
            // WHILE inItem is still true from the outer item, so the
            // generic put() override's own `inItem && !inProp` auto-lbl
            // check would wrongly re-fire mid-string without this.
            inProp=true;
            break;
          case Fmt::Body:
            needComma=false;
            Base::put("\"body\":[");
            break;
          case Fmt::Item:
            if(needComma) Base::put(',');
            Base::nl();
            Base::put("{\"path\":\"");
            putItemPath(ctx);
            Base::put('"');
            inItem=true; inProp=false; autoLbl=false; dataOwnsProp=false;
            inOptArray=false; inOpt=false; optFirstProp=true;
            break;
          case Fmt::Label:
            if(autoLbl) { Base::put('"'); autoLbl=false; } // close any pending auto-lbl
            Base::put(",\"lbl\":\"");
            inProp=true;
            break;
          // Field/Unit/Selected can each fire either at plain item level
          // (always preceded by at least "path", so the leading comma is
          // always correct) or nested inside one Fmt::Option's own {...}
          // (where whichever of sel/fld/un prints FIRST must skip it, or
          // the option object starts with a stray leading comma — invalid
          // JSON). optFirstProp (cleared by whichever fires first) makes
          // this order-independent rather than assuming sel always leads.
          case Fmt::Field:
            if(inOpt && optFirstProp) { Base::put("\"fld\":\""); optFirstProp=false; }
            else Base::put(",\"fld\":\"");
            inProp=true;
            break;
          case Fmt::Unit:
            if(inOpt && optFirstProp) { Base::put("\"unit\":\""); optFirstProp=false; }
            else Base::put(",\"unit\":\"");
            inProp=true;
            break;
          case Fmt::Selected:
            if(inOpt && optFirstProp) { Base::put("\"sel\":\""); optFirstProp=false; }
            else Base::put(",\"sel\":\"");
            inProp=true;
            break;
          // Low/High (NumField's own range bounds, e.g. Power's slider,
          // dateField's own year/month/day sub-items) — plain item-level
          // siblings only in every case seen so far (never nested inside
          // an Option), so no inOpt handling needed here, unlike Field/
          // Unit/Selected above.
          case Fmt::Low:
            Base::put(",\"lo\":\"");
            inProp=true;
            break;
          case Fmt::High:
            Base::put(",\"hi\":\"");
            inProp=true;
            break;
          default: break;
        }
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);

        if constexpr(tag==Fmt::EditCursor) return;  // see fmtStart's own comment

        if(tag==Fmt::NavCursor||tag==Fmt::EditMode) {
          Base::put('"');
          return;
        }
        if(tag==Fmt::Index||tag==Fmt::Accel||tag==Fmt::Enabled||tag==Fmt::Choice||tag==Fmt::Dropdown) {
          return; // number — no closing
        }

        if(tag==Fmt::Data) {
          if(datasec>0) {
            datasec--;
            if(datasec==0 && dataOwnsProp) {
              Base::put('"');
              inProp=false;
              dataOwnsProp=false;
            }
          }
          return;
        }

        if(tag==Fmt::Option) { Base::put('}'); inOpt=false; return; }

        switch(tag) {
          case Fmt::View:  Base::put("}}"); Base::nl(); break;
          case Fmt::Menu:
            menuDepth--;
            if(menuDepth>=0 && menuDepth<MaxMenuDepth && menuWrapStack[menuDepth]) Base::put('}');
            break;
          case Fmt::Title: Base::put("\"},"); inProp=false; break;
          case Fmt::Body:  Base::put(']'); break;
          case Fmt::Item:
            if(autoLbl) { Base::put('"'); autoLbl=false; }
            if(inOptArray) { Base::put(']'); inOptArray=false; }
            inItem=false; inProp=false; dataOwnsProp=false;
            Base::put('}'); needComma=true;
            break;
          case Fmt::Label:    Base::put('"'); inProp=false; break;
          case Fmt::Field:    Base::put('"'); inProp=false; break;
          case Fmt::Unit:     Base::put('"'); inProp=false; break;
          case Fmt::Selected: Base::put('"'); inProp=false; break;
          case Fmt::Low:      Base::put('"'); inProp=false; break;
          case Fmt::High:     Base::put('"'); inProp=false; break;
          default: break;
        }
      }
    };
  };

};//namespace oneMenu
