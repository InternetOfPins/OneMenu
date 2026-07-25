// Overlays a live WebSocket channel on the existing HTTP-only menu UI
// (menu.xslt's own plain <input>/<select> onchange="window.location=...",
// and Toggle's own pill-list <a class="opt" href="/set?...">). Pure
// progressive enhancement: everything here is additive, and if the socket
// never connects (or drops), every original onchange/href is left to fire
// normally, so the page keeps working exactly as it did before this script
// existed.
//
// Covers, over WS instead of a full HTTP round-trip:
//   - Plain single-value fields: TextField/NumField/Power's slider,
//     Choose's own current-value display (<input data-src>). Range/number
//     inputs send on every 'input' tick (drag/up-down-step), not just on
//     the trailing 'change' (release/blur) — lets a server-side OnChange
//     hook (item.h) react in real time, not only to the final value.
//   - Select's dropdown rendering (<select data-src>) — same setAt() call
//     as any other field, this.value is just the option's own index string.
//   - Toggle/Select's own pill-list rendering (.opts a.opt href) — each
//     pill's href already encodes path+val (/set?path=...&val=...&at=...),
//     parsed client-side rather than needing a data-src of its own.
//   - Date's own pad sub-fields (.pad-menu input) — nested one level under
//     a parent item's own "menu" key in the pushed JSON; patchItem() below
//     recurses into it the same way the server nests it.
// jsonFmt.h/item.h/menu.h support all of the above as valid JSON (see
// jsonFmt.h's own header comment).
//
// Navigation links (<a href="/menu?at=...">) are handled a DIFFERENT way
// than field edits — NOT over WS. webNav is one single, server-wide object
// (asyncNav.h) — the whole HTTP path was built deliberately stateless so
// two browser tabs never clobber each other's own nav position (main.cpp's
// own /menu handler comment). Routing navigation through a broadcast WS
// push would reintroduce that same problem: one tab's click dragging every
// OTHER connected tab's cursor along with it. webNav.setAt() (used below,
// over WS, for field edits) has no such problem — it's provably
// side-effect-free on nav state.
//
// Instead, navigate() below re-runs the SAME transform /menu's own XSLT-PI
// route already does, just client-side: fetch the plain /menu?at=... XML
// (same stateless, self-contained request the HTTP fallback always made),
// run it through the browser's own XSLTProcessor with the same menu.xslt
// already cached for this page, and swap the transformed .panel/.result-box
// into the CURRENT document instead of a real navigation. Two real wins
// over a plain <a href> click, beyond skipping the reload flash: the
// WebSocket connection survives across "page changes" (a real navigation
// tears it down and reconnects every time), and the exact same swap
// function can be re-run on a timer to catch changes nobody clicked for —
// see the poll timer at the bottom of this file.
(function () {
  var ws = null;
  var wiredInputs = [];
  var wiredPills = [];
  var wiredLinks = [];
  var xsltDoc = null;
  var currentAt = new URLSearchParams(location.search).get('at') || '/';
  var lastXml = null; // last-applied /menu?at=... response, skips a no-op swap

  function eligible(el) {
    return !el.closest('.opts'); // pills wired separately, see wirePills()
  }

  function loadXslt(cb) {
    if (xsltDoc) { cb(); return; }
    fetch('/menu.xslt').then(function (r) { return r.text(); }).then(function (text) {
      xsltDoc = new DOMParser().parseFromString(text, 'application/xml');
      cb();
    });
  }

  // Maps MultiLangText::current's own raw numeric index (what jsonFmt.h/
  // xmlFmt.h actually emit — see their own comments) to a readable
  // filename code — menu.xslt keeps its own matching table (three
  // separate runtimes: C++/XSLT/JS, no single shared source for this).
  var LANG_CODES = ['en', 'pt', 'zh'];

  // Cache of the current language's {id: text} translation dictionary —
  // see translateFallback() below for why this exists at all.
  var langDict = null;
  var langDictLang = null;

  function loadLangDict(lang, cb) {
    if (langDict && langDictLang === lang) { cb(); return; }
    var code = LANG_CODES[lang] || 'en';
    fetch('/lang/' + code + '.xml').then(function (r) { return r.text(); }).then(function (text) {
      var doc = new DOMParser().parseFromString(text, 'application/xml');
      var dict = {};
      doc.querySelectorAll('t').forEach(function (t) { dict[t.getAttribute('id')] = t.textContent; });
      langDict = dict;
      langDictLang = lang;
      cb();
    }).catch(function (err) {
      console.log('[menu.js] loadLangDict failed:', err);
      cb(); // proceed without a dict — translateFallback then just no-ops
    });
  }

  // menu.xslt's own document()-based translation (renderLbl template) only
  // works for a real top-level page load — confirmed this session that the
  // SAME stylesheet's document() call does not resolve through the JS-
  // exposed XSLTProcessor API used here for a client-side re-transform
  // (transformToFragment hangs/fails on it, a real browser-engine
  // limitation, not a bug in the stylesheet itself). So after a client-side
  // transform, any label/option still showing a "#N" marker means that
  // lookup silently didn't happen — patch those specifically, client-side,
  // using the exact same "id -> text, else leave the marker's own id
  // number" logic menu.xslt's own renderLbl already applies for a real
  // page load. The '#' marker (item.h's IdText, not a bare number) is what
  // lets this scan option VALUES too (Toggle's own pills, Select's own
  // dropdown) without also mistranslating a genuinely-numeric literal
  // option (e.g. a percentage "10") that happens to share a real id.
  function translateFallback(lang) {
    loadLangDict(lang, function () {
      if (!langDict) return;
      var els = document.querySelectorAll(
        '.panel label, .panel .choose-link > span:first-child, ' +
        '.panel a[href^="/menu?at="]:not(.choose-link), ' +
        '.panel .opts a.opt, .panel select[data-src] option, ' +
        '.panel span.disabled'
      );
      els.forEach(function (el) {
        var m = /#(\d+)/.exec(el.textContent);
        if (m && langDict[m[1]] !== undefined) el.textContent = el.textContent.replace(m[0], langDict[m[1]]);
      });
    });
  }

  // Swaps the transformed .panel/.result-box into the live document —
  // everything OUTSIDE .panel (head, masthead, this script itself) is left
  // completely alone, so CSS/JS never gets re-fetched or re-run.
  function renderInto(xmlText) {
    var xmlDoc = new DOMParser().parseFromString(xmlText, 'application/xml');
    var proc = new XSLTProcessor();
    proc.importStylesheet(xsltDoc);
    var frag = proc.transformToFragment(xmlDoc, document);
    var newPanel = frag.querySelector('.panel');
    var newOutput = frag.querySelector('.result-box');
    var oldPanel = document.querySelector('.panel');
    var oldOutput = document.querySelector('.result-box');
    if (oldOutput) oldOutput.remove();
    if (newPanel && oldPanel) oldPanel.replaceWith(newPanel);
    if (newOutput) (newPanel || oldPanel).after(newOutput);
    wire(); // freshly-swapped DOM has none of the old listeners
    var viewEl = xmlDoc.querySelector('view');
    if (viewEl) translateFallback(viewEl.getAttribute('lang'));
  }

  // push=false is used for both popstate (URL already changed, no new entry
  // needed) and the poll timer (background refresh, not a real navigation).
  // fallbackHref: real click-driven navigations fall back to a plain
  // browser navigation if the fetch/transform fails for any reason —
  // background polls just silently skip a failed round, no fallback needed.
  // force: skips the focus guard below even when push===false — for a
  // deliberate user action (e.g. picking a new language from a <select>)
  // rather than a passive background poll. Needed because the <select>
  // itself is still document.activeElement right after the user's own
  // 'change' fires it — the guard existed to protect a text field being
  // typed into from an unrelated poll, not to block the very reload the
  // user's own selection just asked for.
  function navigate(at, push, fallbackHref, force) {
    loadXslt(function () {
      fetch('/menu?at=' + encodeURIComponent(at)).then(function (r) { return r.text(); }).then(function (xml) {
        if (xml === lastXml) return; // nothing changed, skip the swap entirely
        // A background poll must never yank focus out from under an
        // in-progress edit — a real click-driven navigate() (push!==false)
        // still always applies, since the user explicitly asked to leave.
        if (push === false && !force) {
          var panel = document.querySelector('.panel');
          if (panel && document.activeElement && panel.contains(document.activeElement)) return;
        }
        lastXml = xml;
        currentAt = at;
        renderInto(xml);
        if (push !== false) history.pushState({ at: at }, '', '/menu?at=' + encodeURIComponent(at));
      }).catch(function (err) {
        console.log('[menu.js] navigate failed, falling back to real navigation:', err);
        if (fallbackHref) location.href = fallbackHref;
      });
    });
  }

  function navClick(ev) {
    ev.preventDefault();
    var href = this.getAttribute('href');
    var at = new URL(href, location.href).searchParams.get('at') || '/';
    navigate(at, true, href);
  }

  // Choose's own in-submenu option buttons (bare a.opt OUTSIDE .opts,
  // menu.xslt's "Choose's own sub-option button" template) point at /set
  // (set the value, then a real 302 back to the parent — the "close"
  // behavior), not /menu — a genuinely different shape from a plain nav
  // link. fetch() follows redirects by default, so one request already
  // gets us the FINAL /menu?at=parent response body; r.url reflects where
  // it actually landed (at AND the ?sel= highlight param /set's own
  // redirect adds), reused verbatim for the swap and the URL bar.
  function setAndClose(ev) {
    ev.preventDefault();
    var href = this.getAttribute('href');
    loadXslt(function () {
      fetch(href).then(function (r) {
        var landedUrl = new URL(r.url);
        return r.text().then(function (xml) { return { xml: xml, url: landedUrl }; });
      }).then(function (result) {
        if (result.xml === lastXml) return;
        lastXml = result.xml;
        currentAt = result.url.searchParams.get('at') || '/';
        renderInto(result.xml);
        history.pushState({ at: currentAt }, '', result.url.pathname + result.url.search);
      }).catch(function (err) {
        console.log('[menu.js] setAndClose failed, falling back to real navigation:', err);
        location.href = href;
      });
    });
  }

  window.addEventListener('popstate', function (ev) {
    var at = (ev.state && ev.state.at) || new URLSearchParams(location.search).get('at') || '/';
    navigate(at, false);
  });

  // Flow control for high-frequency senders (a slider drag fires many more
  // 'input' events/sec than key-repeat does): at most one message is ever
  // in flight. A tick that arrives while one is still pending doesn't get
  // sent — it just overwrites wsQueued, so only the LATEST value survives —
  // and gets flushed the moment a response arrives (ws.onmessage below).
  // Without this, blasting ws.send() for every drag tick either overflows
  // the browser's own WS send buffer or outpaces the device's single-
  // threaded loop(), and nothing visibly gets through at all. Same idea as
  // MicroTC360's own send-on-every-change-but-gated-on-response approach.
  var wsPending = false;
  var wsQueued = null;

  function wsSendMsg(msg) {
    console.log('[menu.js] WS send:', msg);
    ws.send(msg);
    wsPending = true;
  }

  function wsSend() {
    var msg = 'S|' + this.getAttribute('data-src') + '|' + this.value;
    if (wsPending) { wsQueued = msg; return; }
    wsSendMsg(msg);
  }

  function wire() {
    document.querySelectorAll('input[data-src], select[data-src]').forEach(function (el) {
      if (!eligible(el) || el._wsWired) return;
      el._wsWired = true;
      el._wsOnchange = el.onchange; // keep the original HTTP fallback
      el.onchange = null; // suppressed while WS is live
      el.addEventListener('change', wsSend);
      // Range sliders and number spinners ALSO send live, on every drag
      // tick / up-down step (the 'input' event) — not just once on
      // release/blur ('change') — so a server-side OnChange hook (e.g.
      // driving a motor PWM in real time) sees every intermediate value,
      // matching the slider's own already-live local readout above it.
      if (el.type === 'range' || el.type === 'number') el.addEventListener('input', wsSend);
      wiredInputs.push(el);
    });
    // Pills carry path+val in their own href, not a data-src — parsed at
    // click time instead. href is left untouched (not nulled): when WS
    // isn't open, the click simply falls through to it unprevented.
    //
    // Scoped to ".opts a.opt" specifically, NOT a bare "a.opt" — Choose's
    // OWN in-submenu option buttons (menu.xslt's "Choose's own sub-option
    // button" template) reuse the exact same class="opt" outside any
    // .opts wrapper, but their click is a COMPOUND action (set the value
    // AND navigate back to the parent list — the "close" behavior) which
    // this file deliberately doesn't route over WS (see this file's own
    // header comment on navigation). A bare "a.opt" query wrongly hijacked
    // that click too: preventDefault() blocked the redirect while still
    // sending the WS set, so selecting a Choose option silently applied
    // the value but never returned to the parent list.
    document.querySelectorAll('.opts a.opt').forEach(function (a) {
      if (a._wsWired) return;
      a._wsWired = true;
      a.addEventListener('click', pillClick);
      wiredPills.push(a);
    });
    // Choose's own in-submenu option buttons: bare a.opt OUTSIDE .opts,
    // pointing at /set (not /menu) — see setAndClose's own comment above.
    document.querySelectorAll('a.opt[href^="/set?"]').forEach(function (a) {
      if (a.closest('.opts') || a._navWired) return;
      a._navWired = true;
      a.addEventListener('click', setAndClose);
      wiredLinks.push(a);
    });
    // Plain nav links ("Data fields...", the "‹ Back" title link, etc.) —
    // client-side fetch+XSLT+swap instead of a real navigation (see this
    // file's own header comment).
    document.querySelectorAll('a[href^="/menu?at="]').forEach(function (a) {
      if (a._navWired) return;
      a._navWired = true;
      a.addEventListener('click', navClick);
      wiredLinks.push(a);
    });
    console.log('[menu.js] wired inputs:', wiredInputs.map(function (el) { return el.getAttribute('data-src'); }));
    console.log('[menu.js] wired pills:', wiredPills.length);
  }

  function pillClick(ev) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return; // fall back to href navigation
    ev.preventDefault();
    var url = new URL(this.getAttribute('href'), location.href);
    var msg = 'S|' + url.searchParams.get('path') + '|' + url.searchParams.get('val');
    if (wsPending) { wsQueued = msg; return; }
    wsSendMsg(msg);
  }

  function unwire() {
    wiredInputs.forEach(function (el) {
      el.onchange = el._wsOnchange;
      el.removeEventListener('change', wsSend);
      el.removeEventListener('input', wsSend); // no-op if never added (non range/number)
      el._wsWired = false;
    });
    wiredInputs = [];
    wiredPills.forEach(function (a) {
      a.removeEventListener('click', pillClick);
      a._wsWired = false;
    });
    wiredPills = [];
  }

  // Pushed render is always the CURRENT full item list for whatever level
  // last changed — patches only elements that exist on THIS page (a client
  // sitting on a different level simply finds no matching element, a
  // harmless no-op, since paths are absolute/unique across the whole tree).
  function patchItem(item) {
    // Nav focus (the highlighted row) — menu.xslt's own <li data-path="...">
    // carries the same path regardless of item type (input/select/pill/
    // plain link), so one lookup covers all of them. A full page load/
    // swap bakes @ncur into $curClass via XSLT; a WS-pushed partial update
    // needs this to move the highlight itself.
    var li = document.querySelector('li[data-path="' + item.path + '"]');
    if (li) li.classList.toggle('cur', item.ncur === '@');
    if ('fld' in item) {
      var el = document.querySelector('input[data-src="' + item.path + '"]');
      // Never clobber a TEXT-style field the user is actively typing into
      // right now. A range slider is exempt from this guard: releasing the
      // thumb leaves the browser's own focus on it (unlike text, that's not
      // an in-progress edit), so the value would never visibly update again.
      if (el && (el.type === 'range' || document.activeElement !== el)) el.value = item.fld;
      var out = document.getElementById('row' + item.path.replace(/\//g, '_'));
      if (out) out.textContent = item.fld;
    }
    if (item.opt) {
      // Dropdown <select>: pick whichever option the "opt" array marks
      // sel=="1" — Select items carry no top-level "fld" of their own.
      var sel = document.querySelector('select[data-src="' + item.path + '"]');
      if (sel && document.activeElement !== sel) {
        var selIdx = item.opt.findIndex(function (o) { return o.sel === '1'; });
        if (selIdx >= 0) sel.selectedIndex = selIdx;
      }
      // Pill list: each pill's own href carries this item's path — same
      // array order the server emits "opt" in, so position lines up.
      document.querySelectorAll('a.opt[href*="path=' + item.path + '"]').forEach(function (a, i) {
        if (item.opt[i]) a.classList.toggle('sel', item.opt[i].sel === '1');
      });
    }
    // Date's own pad sub-fields nest one level under this item's own "menu"
    // key (menu.h's own real nested-menu structure, reused for JSON too).
    if (item.menu && item.menu.body) item.menu.body.forEach(patchItem);
  }

  // Tracks the device's own current language index (jsonFmt.h's own new
  // "lang" key on the view root, mirroring xmlFmt.h's <view lang="N">).
  // null until the first push — never treated as "changed" on that one.
  var lastLang = null;

  function applyRender(data) {
    if (!data.view || !data.view.body) return;
    // A language switch changes every label's resolved TEXT, but patchItem()
    // below only ever writes raw id/value fields — it has no client-side
    // translation table (that's deliberately the XSLT no-JS path's job, see
    // menu.xslt's own renderLbl template). Patching in place after a
    // language change would just re-write the same bare ids nothing here
    // can resolve. Instead, treat it like a navigation: re-fetch /menu and
    // re-run the real XSLT transform, which resolves every label fresh
    // against the new language's own /lang/<idx>.xml.
    if (lastLang !== null && data.view.lang !== lastLang) {
      lastLang = data.view.lang;
      navigate(currentAt, false, null, true); // force: bypass the focus guard, see navigate()'s own comment
      return;
    }
    lastLang = data.view.lang;
    data.view.body.forEach(patchItem);
  }

  function connect() {
    ws = new WebSocket('ws://' + location.hostname + ':81/');
    ws.onopen = wire;
    ws.onclose = function () {
      unwire();
      wsPending = false; wsQueued = null; // don't carry a stale in-flight gate across reconnects
      setTimeout(connect, 2000);
    };
    ws.onmessage = function (e) {
      try {
        var data = JSON.parse(e.data);
        console.log('[menu.js] parsed:', data);
        applyRender(data);
      } catch (err) {
        console.log('[menu.js] not JSON we understand:', err);
      }
      // Any incoming push (ours or another client's — the device processes
      // one WS message at a time, so this always means "caught up, ready
      // for more") clears the in-flight gate and flushes whatever tick
      // arrived meanwhile — see wsSend()'s own comment above.
      wsPending = false;
      if (wsQueued !== null) {
        var next = wsQueued; wsQueued = null;
        wsSendMsg(next);
      }
    };
  }

  // Nav-link routing (navClick) doesn't depend on WS at all — wire it up
  // as soon as the DOM exists, not only once ws.onopen eventually fires.
  // This script tag has no defer/async, so it runs before <body> (and
  // every link/input in it) has even been parsed yet.
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', wire);
  else wire();

  // Poll timer: fallback ONLY, skipped entirely whenever WS is actually
  // connected — WS already pushes changes live, so polling on top of it is
  // pure redundancy, and actively harmful here: /menu's own handler calls
  // webNav.enter() on every hit (resetting the submenu's selection to its
  // first child, its own long-standing per-request contract) and also
  // broadcasts. With WS open, that read-only refresh would reset+rebroadcast
  // every few seconds, undoing whatever the user had just set moments
  // earlier. Only actually poll while there's no live push to rely on
  // instead (WS down/reconnecting).
  setInterval(function () {
    if (ws && ws.readyState === WebSocket.OPEN) return;
    navigate(currentAt, false);
  }, 4000);

  connect();
})();
