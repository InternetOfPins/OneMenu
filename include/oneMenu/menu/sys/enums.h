/**
 * @file enums.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief 
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

#include <oneInput/inputEvent.h>

namespace oneMenu {

  // enum {no=false,yes=true};
  enum class Scroll {no=false,yes=true};
  enum Edge {start=true,stop=false};
  enum class Clear {no,yes};
  enum class Pad {no,yes};
  enum class NavMode {Nav,Edit,Tune};
  enum class AlignMethod {Left,Center,Right};

  // Up/Down are index-paired (Down decrements, Up increments the selection), not visually
  // paired to any physical key — a key mapped "up" on the device (e.g. an ANSI up-arrow) can
  // legitimately map to Cmd::Down if that's how it's wired to move the index.
  // Cmd moved to oneInput/inputEvent.h (unified input layer); re-exported here for backwards compat
  using Cmd = oneInput::Cmd;

  // AM4-parity semantic event bitmask — ported from AM4's eventMask (see
  // oneMenu/compat/am4.h). v1 scope: only Enter/Exit/Focus/Blur are ever raised
  // (by EventDispatch, nav.h). selFocus/selBlur/update/activate are real AM4
  // features too but not implemented yet — deliberately left out rather than
  // declared-and-unused.
  // Registration is a mask (e.g. Focus|Blur, or Any=~0); dispatch is "any bit overlaps"
  // (raised & registeredMask), and the handler receives the single raised bit, not the
  // mask — matches AM4's navNode::event() exactly. Gets |/& for free via the generic
  // operator templates above (same pattern as Fmt).
  enum class EventMask {None=0,Enter=1<<0,Exit=1<<1,Focus=1<<2,Blur=1<<3,Any=~0};

  template<typename T> inline constexpr int operator|(T   a,T   b){return (int)a|(int)b;}
  template<typename T> inline constexpr int operator|(int a,T   b){return (int)a|(int)b;}
  template<typename T> inline constexpr int operator|(T   a,int b){return (int)a|(int)b;}
  template<typename T> inline constexpr int operator&(T   a,T   b){return (int)a&(int)b;}
  template<typename T> inline constexpr int operator&(int a,T   b){return (int)a&(int)b;}
  template<typename T> inline constexpr int operator&(T   a,int b){return (int)a&(int)b;}
  template<typename T> inline constexpr int operator^(T   a,T   b){return (int)a^(int)b;}
  template<typename T> inline constexpr int operator^(int a,T   b){return (int)a^(int)b;}
  template<typename T> inline constexpr int operator^(T   a,int b){return (int)a^(int)b;}

  enum class Fmt:int {
    None=0<<0,View=1<<0,Title=1<<1,Menu=1<<2,Body=1<<3,Item=1<<4,
    Index=1<<5,Accel=1<<6,NavCursor=1<<7,
    Field=1<<8,Label=1<<9,EditMode=1<<10,EditCursor=1<<11,Data=1<<12,Unit=1<<13,
    // Low/High: a numeric field's own range (StaticRange's low<T>()/high<T>()) —
    // real child tags (not attribute-only), consumed today only by XmlFmt (a
    // <fld>'s own min/max, e.g. for a web client's slider widget); every
    // other format's own base fmtStart/fmtStop default (out.h) is already a
    // universal no-op for any tag it doesn't specifically handle, so this is
    // safe to add without touching ANSI/text/gfx rendering at all.
    Low=1<<14,High=1<<15,
    // Option/Selected: a Toggle/Select field's own full option list — each
    // sibling in the field's body, wrapped individually, plus which one is
    // currently selected (a plain 0/1 child, not ctx-derived like NavCursor —
    // there is no real navigation focus on non-selected options, only a
    // stored index) — consumed today only by XmlFmt (radio-group/dropdown
    // rendering); every other format's own base fmtStart/fmtStop default is
    // already a universal no-op for tags it doesn't handle, so this is safe
    // to add without touching ANSI/text/gfx rendering at all.
    Option=1<<16,Selected=1<<17,
    // Choice: marks a <menu> as a Choose field's own inner body (as opposed
    // to an ordinary nested submenu) — a constant attribute, no ctx-derived
    // value, emitted once right after <menu>'s own tag opens (MenuPrinter,
    // printers.h), gated on the Menu<> instance composing the IsChoiceBody
    // marker (menu.h). Lets a web client (XmlFmt only) tell "these sibling
    // items are selectable options, click one to choose it" apart from "this
    // is a normal submenu with independently-editable fields" — same shape
    // as Option/Selected above, consumed today only by XmlFmt.
    Choice=1<<18/*,
    Footer=1<<19*/
  };

  /// @brief lock/unlock print output
  enum class LockMode {
    None=0<<0,//normal output
    Update=1<<0,//draw only changed
    Sync=1<<1,//just sync the visible values
    Measure=1<<2,//normal lock, no output, just cursor movement calculation (no real cursor moved)
    Changed=1<<3//check if any visible items changed
  };

  #ifdef MENU_DEBUG

    template<typename Out>
    Out& operator<<(Out& out, Pad o) {return out<<(o==Pad::yes?"yes":"no");}

    template<typename Out>
    Out& operator<<(Out& out,const NavMode& mode) {
      switch(mode) {
        case NavMode::Nav: return out<<"NavMode::Nav";break;
        case NavMode::Edit: return out<<"NavMode::Edit";break;
        case NavMode::Tune: return out<<"NavMode::Tune";break;
        default: return out<<"NavMode::?";break;
      }
    };

    template<typename Out>
    Out& operator<<(Out& out,const Cmd& cmd) {
      switch(cmd) {
        case Cmd::Enter:return out<<"Cmd::Enter";
        case Cmd::Esc:return out<<"Cmd::Esc";
        case Cmd::Up:return out<<"Cmd::Up";
        case Cmd::Down:return out<<"Cmd::Down";
        case Cmd::Left:return out<<"Cmd::Left";
        case Cmd::Right:return out<<"Cmd::Right";
        case Cmd::Key:return out<<"Cmd::Key";
        case Cmd::Go:return out<<"Cmd::Go";
        default: return out<<"Cmd?";
      }
    };

    template<typename Out>
    Out& operator<<(Out& out,const Fmt& tag) {
      switch(tag) {
        case Fmt::View: return out<<"View";
        case Fmt::Title: return out<<"Title";
        case Fmt::Menu: return out<<"Menu";
        case Fmt::Body: return out<<"Body";
        case Fmt::Item: return out<<"Item";
        case Fmt::Index: return out<<"Index";
        case Fmt::Accel: return out<<"Accel";
        case Fmt::NavCursor: return out<<"NavCursor";
        case Fmt::Field: return out<<"Field";
        case Fmt::Label: return out<<"Label";
        case Fmt::EditMode: return out<<"EditMode";
        case Fmt::EditCursor: return out<<"EditCursor";
        case Fmt::Data: return out<<"Data";
        case Fmt::Unit: return out<<"Unit";
        case Fmt::Low: return out<<"Low";
        case Fmt::High: return out<<"High";
        case Fmt::Option: return out<<"Option";
        case Fmt::Selected: return out<<"Selected";
        case Fmt::Choice: return out<<"Choice";
        default: return out<<"Fmt::?";
      }
    }

    template<typename Out>
    Out& operator<<(Out& out, LockMode m) {
      switch(m){
        case LockMode::None:return out<<"None";
        case LockMode::Update:return out<<"Update";
        case LockMode::Sync:return out<<"Sync";
        case LockMode::Measure:return out<<"Measure";
        case LockMode::Changed:return out<<"Changed";
        default: return out<<"LockMode::?";
      }
    }

    #endif
  };