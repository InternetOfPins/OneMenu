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
  enum class Cmd {None=0,Enter=1<<0,Esc=1<<1,Up=1<<2,Down=1<<3,Left=1<<4,Right=1<<5,Key=1<<6,Go=1<<7};

  // AM4-parity semantic event bitmask — ported from AM4's eventMask (see
  // oneMenu/compat/am4.h and notes.md "AM4 compat layer" for the full plan). v1 scope:
  // only Enter/Exit/Focus/Blur are ever raised (by EventDispatch, nav.h).
  // selFocus/selBlur/update/activate are real AM4 features too but not implemented yet —
  // deliberately left out rather than declared-and-unused.
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
    Field=1<<8,Label=1<<9,EditMode=1<<10,EditCursor=1<<11,Data=1<<12,Unit=1<<13/*,
    Footer=1<<14*/
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