#pragma once

#include "oneMenu/menu/sys/base.h"
#include "oneMenu/menu/sys/formats.h"

namespace oneMenu {

  // HtmlFmt: streams a complete HTML page per request.
  // Selection indicated by class "s" on the item div.
  // NavCursor suppressed — class handles highlight.
  // CSS and nav buttons embedded in fmtStart/fmtStop<View>.
  /// @brief HTML format: complete page per render, selection via CSS class, nav buttons included
  struct HtmlFmt : aFormat {
    template<typename Before, typename After>
    static constexpr bool rules() {
      static_assert(Excludes<IsPrinter, After>, "HtmlFmt: printer layers must be placed above HtmlFmt");
      return true;
    }
    template<typename O>
    struct Part : Formats::template Part<O> {
      using Base = typename Formats::template Part<O>;

      bool m_sel {false};

      template<Fmt tag> std::enable_if_t<tag&Fmt::NavCursor> fmtStart(const Ctx&) {}
      template<Fmt tag> std::enable_if_t<tag&Fmt::NavCursor> fmtStop(const Ctx&)  {}

      // Emit the absolute path for the item being rendered as a URL query string.
      // Caller wraps with href="/cmd?at=" ... "\"".
      void putItemHref(const Ctx& ctx) {
        Base::put("/cmd?at=");
        for (int i = 0; i < ctx.at; i++) { Base::put('/'); Base::put(ctx.path.data[i]); }
        Base::put('/');
        Base::put(ctx.idx);
        Base::put('/');
      }

      template<Fmt tag>
      std::enable_if_t<tag==Fmt::View>
      fmtStart(const Ctx&) {
        Base::put(
          "<!DOCTYPE html><html><head><title>OneMenu</title>"
          "<meta http-equiv=\"refresh\" content=\"5\"/>"
          "<style>"
          "body{font-family:monospace;background:#111;color:#0f0;padding:10px;margin:0}"
          ".m{border:1px solid #0f0;display:inline-block;min-width:180px}"
          ".t{font-size:1.2em;font-weight:bold;padding:4px 8px;border-bottom:1px solid #0f0}"
          ".i{padding:3px 8px}"
          ".s{background:#0f0;color:#111}"
          ".n{margin-top:8px}"
          ".n a{color:#0f0;border:1px solid #0f0;padding:3px 10px;text-decoration:none;margin-right:3px}"
          ".n a:hover{background:#0f0;color:#111}"
          "</style></head><body><div class=\"m\">"
        );
      }

      template<Fmt tag>
      std::enable_if_t<tag==Fmt::View>
      fmtStop(const Ctx&) {
        Base::template fmtStop<tag>({});
        Base::put(
          "</div>"
          "<div class=\"n\">"
          "<a href=\"/cmd?nav=up\">&#x2191;</a>"
          "<a href=\"/cmd?nav=dn\">&#x2193;</a>"
          "<a href=\"/cmd?nav=en\">Enter</a>"
          "<a href=\"/cmd?nav=esc\">Esc</a>"
          "</div></body></html>"
        );
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStart(const Ctx&) { Base::put("<div class=\"t\">"); }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Title>
      fmtStop(const Ctx&)  { Base::put("</div>"); }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStart(const Ctx& ctx) {
        m_sel = bool(ctx);
        Base::put("<a href=\"");
        putItemHref(ctx);
        Base::put("\">");
        Base::put(m_sel ? "<div class=\"i s\">" : "<div class=\"i\">");
      }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Item>
      fmtStop(const Ctx&) { Base::put("</div></a>"); }

      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Footer>
      fmtStart(const Ctx&) {}
      template<Fmt tag>
      std::enable_if_t<tag&Fmt::Footer>
      fmtStop(const Ctx&)  {}

      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Menu|Fmt::Body)>
      fmtStart(const Ctx& ctx) { Base::template fmtStart<tag>(ctx); }
      template<Fmt tag>
      std::enable_if_t<tag&(Fmt::Menu|Fmt::Body)>
      fmtStop(const Ctx& ctx)  { Base::template fmtStop<tag>(ctx); }
    };
  };

} // namespace oneMenu
