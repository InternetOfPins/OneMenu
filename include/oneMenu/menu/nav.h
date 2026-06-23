/**
 * @file nav.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief menu navigation API
 * @version 5
 * @date 2026-04-28
 * 
 * @copyright Copyright (c) 2026
 * 
*/

#pragma once

namespace oneMenu {

  template<typename N> struct NavAPI:N {};

  template <typename API,typename... NN>
  struct DefinedNav:APIOf<API, NN...> {
    using Base = APIOf<API, NN...>;
    bool up() {return Base::template doCmd<false>(Cmd::Up);}
    bool down() {return Base::template doCmd<false>(Cmd::Down);}
    bool enter() {return Base::template doCmd<false>(Cmd::Enter);}
    bool esc() {return Base::template doCmd<false>(Cmd::Esc);}
  };

  template<typename... II>
  struct NavDef:DefinedNav<NavAPI<hapi::CRTP<NavDef<II...>>>,II...> {};

  struct INav {};

  template<typename... II>
  struct INavDef:INav,DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...> {
    using Base=DefinedNav<NavAPI<hapi::CRTP<INavDef<II...>>>,II...>;
  };

  template<typename T,T& menu>
  struct Root {
    template<typename N>
    struct Part:N {
      using Base=N;
      using Root=std::remove_reference_t<decltype(menu)>;
      static constexpr Root& root() {return menu;}
      static constexpr const Depth depth() {return Root::depth();}
    };
  };

  template<typename M>
  struct StaticRoot {
    template<typename N>
    struct Part:N {
      using Base=N;
      M m_menu;
      inline constexpr M& root() {return m_menu;}
      static constexpr const Depth depth() {return M::depth();}
    };
  };

  struct TreeNav {
    template<typename N>
    struct Part:N {
      using Base=N;
      using Root=typename Base::Root;
      using Base::root;
      using Base::depth;

      Path path() {return m_path;}
      Path focus(Sz i) {return m_path.focusAt(i);}
      Depth level() const {return m_level;}
      Sz sel() const {return m_path[m_level];}

      void navMode(NavMode m) {m_navMode.set(m);}
      const NavMode navMode() const {return m_navMode.get();}

      void sync() {
        m_level.sync();
        m_navMode.sync();
        m_prevSel=sel();
      }

      template<typename Out>
      void sync(Out& out) {
        sync();
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Sync);
        printTo(out);
        out.lockMode(om);
      }

      bool changed() const {
        return m_level.changed()
          ||m_navMode.changed()
          ||m_prevSel!=sel();//however items will check later if the are on focus or just blur (Ctx sel and prev. sel) and signal a draw (on update)
      }

      template<typename Out>
      bool changed(Out& out) {
        if(changed()) return true;
        //changed should not modify the output NEVER, it has to remain faithful to the print  process
        // if(m_level.changed()) {
        //   out.lockMode(LockMode::None);
        //   out.clear();           // resets m_at to {0,0} + hardware clear
        //   return true;
        // }
        // if(m_navMode.changed()||m_prevSel!=sel()) {
        //   // out.resume();          // resets m_at to {orgX,orgY} + device state (invert, cursor)
        //   out.lockMode(hapi::TagIs<PartialDraw>::Check<Out>::value?LockMode::Update:LockMode::None); // resume forces None; set Update after
        //   return true;
        // }
        LockMode om=out.lockMode();
        out.lockMode(LockMode::Changed);
        bool r=printTo(out);    // probe: Gate suppresses all hardware, m_at drift is harmless
        // out.setPos({0,0});//this is clear() job
        // if(r) out.resume();     // data changed: reset for full redraw; resume sets None
        // else 
        out.lockMode(om);
        return r;
      }

      template<typename Out>
      bool printTo(Out& out) {
        ///track scroll top for each level, this is output device specific
        static Sz tops[root().depth()]{0};//TODO: check if ScrollBody is in output part, or store this there with an API call fallback.
        Ctx ctx{focus(m_level+1),m_navMode,m_print_level,true,tops,0,m_prevSel};
        // dout<<xy<0,1><<colors<BLACK,RED><<ctx<<padWith<10><<flush;out.resume();
        // dout<<xy<0,2><<" level:"<<level()<<" path:"<<path()<<padWith<10><<flush;
        bool r=root().printMenu(out,ctx);
        out.flush();
        return r;
      }

      template<bool isKbd>
      bool doCmd(Cmd cmd,Key k=0, bool e=false) {
        if(cmd==Cmd::Esc) return close();//preemptive esc=>close, //TODO: need events to inform target (as a component!)
        CKE cke{cmd,k,e};
        bool r=root().template nav<isKbd>(Base::obj(),cke,focus(m_level+1));
        return r;
      }

      // aux function for items to call and complete a navigation request with default actions
      // Data<Sz&> would dangle: Data constructor takes by-value, binds reference to local.
      // Use Data<Sz> (owned copy) + writeback through the actual sel reference.
      bool doNav(CKE cke,Sz len,bool w) {
        // dout<<colors<GREEN,BLACK><<" len:"<<len<<" wraps:"<<w;
        Sz& sel=m_path.data[(int)level()];
        oneData::DataDef<NumRange<Sz>,oneData::Data<Sz>> at(0,len-1,w,sel);
        switch(cke.cmd) {
          case Cmd::Up:   at.up();   break;
          case Cmd::Down: at.down(); break;
          default: return false;
        }
        sel=at.get();
        return true;
      }

      template<typename In>
      bool in(In& in) {
        CKE cke = in.cmd();
        if (cke.cmd == Cmd::None) return false;
        return cke.kbd ? doCmd<true> (cke.cmd, cke.key, cke.ext)
                       : doCmd<false>(cke.cmd, cke.key, cke.ext);
      }

      void go(Sz i,Depth delta=0) {
        assert(m_level+delta<depth());
        m_path.data[m_level+delta]=i;
      }

      bool padOpen() {
        if(m_level.get()<depth()) {
          m_level.set(m_level+1);
          m_path.data[m_level]=0;
          if(m_level.get()<m_print_level) m_print_level=m_level;
          return true;
        } else return false;
      }
      bool open() {
        if(padOpen()) {
          m_print_level++;//=m_level;
          return true;
        } else return false;
      }
      
      bool close() {
        navMode(NavMode::Nav);
        if(m_level) {
          m_level.set(m_level-1);
          if(m_print_level>m_level) m_print_level=m_level;
          return true;
        } else return false;
      }

    protected: 
      Sz m_prevSel{};
      PathData<depth()+1> m_path{};//TODO: why do we need +1? check depth calc!
      oneData::DataDef<Watch<oneData::Data<Depth>>> m_level{0};
      Depth m_print_level{0};
      oneData::DataDef<Watch<oneData::Data<NavMode>>> m_navMode{NavMode::Nav};
    };
  };

  // Handles Cmd::Go from IdParser: jumps to item N at current level then enters it.
  // Place above TreeNav in the nav chain:  NavDef<IndexGo, TreeNav, Root<...>>
  struct IndexGo {
    template<typename N>
    struct Part : N {
      using Base = N;
      template<bool kbd = false>
      bool doCmd(Cmd c, Key k = 0, bool e = false) {
        if (c == Cmd::Go) {
          Base::go(Sz(k) - 1);  // IdParser emits 1-based; go() is 0-based
          return Base::template doCmd<false>(Cmd::Enter);
        }
        return Base::template doCmd<kbd>(c, k, e);
      }
    };
  };

};// namespace oneMenu