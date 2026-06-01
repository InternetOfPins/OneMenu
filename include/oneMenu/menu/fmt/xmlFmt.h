/**
 * @file xmlFmt.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * @version 5
 * @date 2026-05-04
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

struct XmlFmt {
  template<typename O>
  struct Part:Formats::template Part<O> {
    using Base=typename Formats::template Part<O>;
    using Base::nl;
    using Base::put;

    int indent{0};
    int datasec{0};
    bool attr{false};

    template<Fmt tag>
    void name() {
      switch(tag) {
        case Fmt::View: Base::put("view");break;
        case Fmt::Title: Base::put("title");break;
        case Fmt::Menu: Base::put("menu");break;
        case Fmt::Body: Base::put("body");break;
        case Fmt::Item: Base::put("item");break;
        case Fmt::Index: Base::put("idx");break;
        case Fmt::Accel: Base::put("acc");break;
        case Fmt::NavCursor: Base::put("ncur");break;
        case Fmt::Field: Base::put("fld");break;
        case Fmt::Label: Base::put("lbl");break;
        case Fmt::EditMode: Base::put("mode");break;
        case Fmt::EditCursor: Base::put("ecur");break;
        case Fmt::Data: Base::put("data");break;
        case Fmt::Unit: Base::put("un");break;
        default: put("fmt");break;
      }
    }

    template<Fmt tag>
    void attrStart(const Ctx& ctx) {
      put(' ');
      name<tag>();
      put('=');
      put('"');
    }

    template<Fmt tag>
    void attrStop(const Ctx& ctx) {put('"');}

    static constexpr const int attr_tags=Fmt::NavCursor|Fmt::Index|Fmt::EditCursor|Fmt::EditMode|Fmt::Accel;
    static constexpr const int indent_tags=Fmt::View|Fmt::Menu|Fmt::Body|Fmt::Title|Fmt::Item;

    void putPath(const Path& p,Depth s,Depth l) {
      assert(s+l<p.len);
      for(int i=s;i<s+l;i++) {
        put('/');
        put(p.data[i]);
      }
      put('/');
    }

    template<Fmt tag>
    void fmtStart(const Ctx& ctx) {
      if(attr&&!(tag&attr_tags)) {
        attr=false;
        put('>');
        if(tag&indent_tags) nl();
      }
      if(tag&attr_tags) {
        attr=true;
        attrStart<tag>(ctx);
        switch(tag) {
          default:break;
          // case Fmt::View: 
          case Fmt::Index: put(ctx.idx);break;
          case Fmt::NavCursor: put(ctx?(ctx.enabled?'@':'-'):' ');break;
        }
        return;
      }

      if(tag&(Fmt::Data)) {
        if(datasec==0) put("<![CDATA[");
        datasec++;
        return;
      }

      if(tag&(indent_tags))
        for(int n=indent;n>0;n--) put("  ");
      put('<');
      name<tag>();
      attr=true;
      if(tag&(Fmt::View|Fmt::Menu|Fmt::Body)) {
        if(tag&(Fmt::View|Fmt::Menu)) {
          put(" at=\"");
          putPath(ctx.path,0,tag==Fmt::Menu?std::template min<Depth>(ctx.at,ctx.path.len-1):ctx.pAt);
          put("\"");
        }
        indent++;
        // nl();
      }
      // if(tag&(Fmt::Item|Fmt::Title)) {
      //   put('$');
      //   nl();
      // }
      Base::template fmtStart<tag>(ctx);
    }
    template<Fmt tag>
    void fmtStop(const Ctx& ctx) {
      Base::template fmtStop<tag>(ctx);
      if(tag&(Fmt::NavCursor|Fmt::Index|Fmt::EditCursor|Fmt::EditMode|Fmt::Accel)) {
        attrStop<tag>(ctx);
        return;
      }
      if(tag&(Fmt::Data)) {
        if(datasec>0) {
          datasec--;
          if(datasec==0) put("]]>");
          return;
        }
      }
      if(tag&(Fmt::View|Fmt::Menu|Fmt::Body)) indent--;
      if(tag&(Fmt::View|Fmt::Menu|Fmt::Body))
        for(int n=indent;n>0;n--) put("  ");
      put('<');
      put('/');
      name<tag>();
      put('>');
      if(tag&(Fmt::View|Fmt::Menu|Fmt::Body|Fmt::Item|Fmt::Title)) nl();
    }

  };
};
