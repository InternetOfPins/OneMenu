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
  struct JsonFmt : aFormat {
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
      // instead of closing it first, producing invalid JSON (confirmed
      // 2026-07-22: dateField's own 2nd sub-item's separator text followed
      // by its own NumField produced `"lbl":".,"fld":"1""` — a real,
      // unescaped quote landing mid-string). Mirrors XmlFmt's closeCDATA().
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
        // corrupts the field's own "fld" string (same root cause as the
        // XmlFmt fix, same day: mode's own value already conveys "this
        // field is being edited"). No-op here.
        if constexpr(tag==Fmt::EditCursor) return;

        // inline attribute-like tags — value emitted here via Base::put (not intercepted)
        if(tag==Fmt::NavCursor||tag==Fmt::Index||tag==Fmt::EditMode||tag==Fmt::Accel) {
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

        if(tag==Fmt::Field||tag==Fmt::Unit) closeAutoLbl();

        switch(tag) {
          case Fmt::View:
            Base::put("{\"view\":{");
            break;
          case Fmt::Menu:
            Base::put("\"at\":\"");
            putPath(ctx.path, 0, ctx.at>0 ? ctx.at : ctx.pAt);
            Base::put("\",");
            break;
          case Fmt::Title:
            Base::put("\"title\":{\"path\":\"");
            putPath(ctx.path, 0, std::min<Depth>(ctx.at,(Depth)(ctx.path.len-1)));
            Base::put("\",\"text\":\"");
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
            break;
          case Fmt::Label:
            if(autoLbl) { Base::put('"'); autoLbl=false; } // close any pending auto-lbl
            Base::put(",\"lbl\":\"");
            inProp=true;
            break;
          case Fmt::Field:
            Base::put(",\"fld\":\"");
            inProp=true;
            break;
          case Fmt::Unit:
            Base::put(",\"unit\":\"");
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
        if(tag==Fmt::Index||tag==Fmt::Accel) {
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

        switch(tag) {
          case Fmt::View:  Base::put("}}"); Base::nl(); break;
          case Fmt::Menu:  break;
          case Fmt::Title: Base::put("\"},"); break;
          case Fmt::Body:  Base::put(']'); break;
          case Fmt::Item:
            if(autoLbl) { Base::put('"'); autoLbl=false; }
            inItem=false; inProp=false; dataOwnsProp=false;
            Base::put('}'); needComma=true;
            break;
          case Fmt::Label: Base::put('"'); inProp=false; break;
          case Fmt::Field: Base::put('"'); inProp=false; break;
          case Fmt::Unit:  Base::put('"'); inProp=false; break;
          default: break;
        }
      }
    };
  };

};//namespace oneMenu
