#pragma once

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu  {
  /// @brief XML format: element-per-item, attributes for nav state, CDATA for label text
  struct XmlFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "XmlFmt: printer layers must be placed above XmlFmt");
      return true;
    }
    template<typename O>
    struct Part:Formats::template Part<O> {
      using Base=typename Formats::template Part<O>;

      int  indent    {0};
      int  datasec   {0};
      bool attr      {false};
      bool inItem    {false};
      bool autoCDATA {false}; // auto-started CDATA for plain item body text

      // Close open element tag before any inline content
      void closeAttr() { if(attr) { attr=false; Base::put('>'); } }

      // put() from CRTP outer — plain item body text needs auto CDATA wrap.
      // puts from own handlers use Base::put() directly, bypassing this.
      template<typename T> void put(T v) {
        if(inItem && datasec==0 && !autoCDATA) { closeAttr(); Base::put("<![CDATA["); autoCDATA=true; }
        else closeAttr();
        Base::put(v);
      }
      void put(const char* s, Sz n) {
        if(inItem && datasec==0 && !autoCDATA) { closeAttr(); Base::put("<![CDATA["); autoCDATA=true; }
        else closeAttr();
        Base::put(s,n);
      }
      void nl() { closeAttr(); Base::nl(); }

      template<Fmt tag>
      static constexpr const char* tagName() {
        switch(tag) {
          case Fmt::View:       return "view";
          case Fmt::Title:      return "title";
          case Fmt::Menu:       return "menu";
          case Fmt::Body:       return "body";
          case Fmt::Item:       return "item";
          case Fmt::Index:      return "idx";
          case Fmt::Accel:      return "acc";
          case Fmt::NavCursor:  return "ncur";
          case Fmt::Field:      return "fld";
          case Fmt::Label:      return "lbl";
          case Fmt::EditMode:   return "mode";
          case Fmt::EditCursor: return "ecur";
          case Fmt::Data:       return "data";
          case Fmt::Unit:       return "un";
          default:              return "fmt";
        }
      }

      static constexpr const int attr_tags   = Fmt::NavCursor|Fmt::Index|Fmt::EditCursor|Fmt::EditMode|Fmt::Accel;
      static constexpr const int indent_tags = Fmt::View|Fmt::Menu|Fmt::Body|Fmt::Title|Fmt::Item;
      static constexpr const int block_tags  = Fmt::View|Fmt::Menu|Fmt::Body|Fmt::Title|Fmt::Item;

      void putPath(const Path& p, Depth s, Depth l) {
        Depth end = (s+l < p.len) ? s+l : p.len-1;
        for(int i=s; i<end; i++) { Base::put('/'); Base::put(p.data[i]); }
        Base::put('/');
      }

      // Absolute path for the item currently being rendered:
      // parent path (ctx.at levels) + this item's own index.
      void putItemPath(const Ctx& ctx) {
        putPath(ctx.path, 0, ctx.at);
        Base::put(ctx.idx);
        Base::put('/');
      }

      template<Fmt tag>
      void fmtStart(const Ctx& ctx) {
        if(tag&attr_tags) {
          // attribute: space + name + =" + value (fmtStop closes the quote)
          Base::put(' ');
          Base::put(tagName<tag>());
          Base::put("=\"");
          switch(tag) {
            default: break;
            case Fmt::Index:      Base::put(ctx.idx); break;
            case Fmt::NavCursor:  Base::put(ctx ? (ctx.enabled ? '@' : '-') : ' '); break;
            case Fmt::EditMode:
              if(ctx) switch(ctx.mode) {
                case NavMode::Nav:  Base::put("nav");  break;
                case NavMode::Edit: Base::put("edit"); break;
                case NavMode::Tune: Base::put("tune"); break;
              } else Base::put("none");
              break;
            case Fmt::EditCursor: Base::put(ctx ? '|' : ' '); break;
            case Fmt::Accel:      Base::put(ctx.idx); break;
          }
          return;
        }

        if(tag&(Fmt::Data)) {
          closeAttr();
          if(datasec==0) Base::put("<![CDATA[");
          datasec++;
          return;
        }

        // block/inline element open: close previous open tag with >, nl for block children
        if(attr) {
          attr=false;
          Base::put('>');
          if(tag&block_tags) Base::nl();
        }
        if(tag&(indent_tags))
          for(int n=indent; n>0; n--) Base::put("  ");
        Base::put('<');
        Base::put(tagName<tag>());
        attr=true;
        if(tag&(Fmt::View|Fmt::Menu|Fmt::Body)) {
          if(tag&(Fmt::View|Fmt::Menu)) {
            Base::put(" at=\"");
            putPath(ctx.path, 0, tag==Fmt::Menu
              ? std::min<Depth>(ctx.at, ctx.path.len-1)
              : ctx.pAt);
            Base::put('"');
          }
          indent++;
        }
        if(tag==Fmt::Item) {
          Base::put(" path=\"");
          putItemPath(ctx);
          Base::put('"');
          inItem=true; autoCDATA=false;
        }
        // path on labels and titles enables client-side XSLT string translation
        // without ancestor traversal: <lbl path="/1/3/">Motor Power</lbl>
        if(tag==Fmt::Label) {
          Base::put(" path=\"");
          putItemPath(ctx);
          Base::put('"');
        }
        if(tag==Fmt::Title) {
          Base::put(" path=\"");
          putPath(ctx.path, 0, std::min<Depth>(ctx.at, (Depth)(ctx.path.len-1)));
          Base::put('"');
        }
        Base::template fmtStart<tag>(ctx);
      }

      template<Fmt tag>
      void fmtStop(const Ctx& ctx) {
        Base::template fmtStop<tag>(ctx);
        if(tag&attr_tags) {
          Base::put('"');  // close attribute value quote
          return;
        }
        if(tag&(Fmt::Data)) {
          if(datasec>0) {
            datasec--;
            if(datasec==0) Base::put("]]>");
          }
          return;
        }
        if(tag==Fmt::Item && autoCDATA) { Base::put("]]>"); autoCDATA=false; inItem=false; }
        closeAttr();
        if(tag==Fmt::Item) inItem=false;
        if(tag&(Fmt::View|Fmt::Menu|Fmt::Body)) {
          indent--;
          for(int n=indent; n>0; n--) Base::put("  ");
        }
        Base::put("</");
        Base::put(tagName<tag>());
        Base::put('>');
        if(tag&(Fmt::View|Fmt::Menu|Fmt::Body|Fmt::Item|Fmt::Title)) Base::nl();
      }
    };
  };
};//namespace oneMenu
