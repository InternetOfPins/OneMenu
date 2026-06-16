/**
 * @file data.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief OneData overrides for OneMenu
*/

  // static text --
  // template <const char* const* text_ptr>
  // struct StaticText {
  //   using Type = const char*;
  //   template <typename O>
  //   struct PartEnd : O {
  //     using O::O;
  //     template<typename Out> void print(Out& out) const noexcept {}
  //   };
  //   template <typename O>
  //   struct Part : oneData::StaticText<text_ptr>::template Part<PartEnd<O>> {
  //     using Base = typename oneData::StaticText<text_ptr>::template Part<PartEnd<O>>;
  //     using Type = const char*;
  //     using Base::Base;
  //     template<typename Out> void print(Out& out) const noexcept { out.put(get()); O::print(out); }
  //   };
  // };

  // DATA (Owned RAM Storage) ---------------------------------------------------
  template <typename T>
  struct Data:oneData::Data<T> {
    using Type = T;
    template <typename O>
    struct PartEnd : O {
      using O::O;
      template<typename Out> void print(Out& out) const noexcept {}
    };
    template <typename O>
    struct Part : oneData::Data<T>::template Part<PartEnd<O>> {
      using Base = typename oneData::Data<T>::template Part<PartEnd<O>>;
      using Type = T;
      using Base::Base;
      template<typename Out> void print(Out& out) const noexcept { Base::print(out); O::print(out); }
    };
  };

  // DATA REF (Unified External Pointer/Reference - 0 Bytes RAM) ---------------
  template <typename T, T address>
  struct DataRef {
    using Type = std::remove_pointer_t<T>;
    template <typename O>
    struct PartEnd : O {
      using O::O;
      template<typename Out> void print(Out& out) const noexcept {}
    };
    template <typename O>
    struct Part : oneData::DataRef<T,address>::template Part<PartEnd<O>> {
      using Base = typename oneData::DataRef<T,address>::template Part<PartEnd<O>>;
      using Type = T;
      using Base::Base;
      template<typename Out> void print(Out& out) const noexcept { Base::print(out); O::print(out); }
    };
  };

