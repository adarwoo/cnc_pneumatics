#pragma once
#pragma GCC system_header

#include <type_traits> // for std::remove_reference, std::remove_cv, std::remove_cvref, std::is_same_v
#include <utility>     // for std::declval, std::index_sequence, std::make_index_sequence

namespace sml::inline v3_0_0 {
using size_t = decltype(sizeof(int));

namespace type_traits {
struct none {};

// Replace custom traits with standard library equivalents
template<class...> inline constexpr bool is_same_v = false;
template<class T> inline constexpr bool is_same_v<T, T> = true;

template<class T> using remove_reference_t = std::remove_reference_t<T>;
template<class T> using remove_cv_t = std::remove_cv_t<T>;
template<class T> using remove_cvref_t = std::remove_cvref_t<T>;

template<class> struct transition_traits;

template<class T> requires requires { &T::operator(); }
struct transition_traits<T> : transition_traits<decltype(&T::operator())> { };

template<class T> requires requires { &T::template operator()<none>; }
struct transition_traits<T> : transition_traits<decltype(&T::template operator()<none>)> { };

template<class T> requires requires { &T::template operator()<none, none>; }
struct transition_traits<T> : transition_traits<decltype(&T::template operator()<none, none>)> { };

template<class T, class TSrc, class TEvent, class TDst>
struct transition_traits<auto (T::*)(TSrc, TEvent) const -> TDst> {
  using src = std::remove_cvref_t<TSrc>;
  using event = std::remove_cvref_t<TEvent>;
  using dst = std::remove_cvref_t<TDst>;
};

template<class T, class TSrc, class TEvent, class TDst>
struct transition_traits<auto (T::*)(TSrc, TEvent) -> TDst> {
  using src = std::remove_cvref_t<TSrc>;
  using event = std::remove_cvref_t<TEvent>;
  using dst = std::remove_cvref_t<TDst>;
};
} // namespace type_traits

namespace utility {
   // Using std::declval and std::index_sequence
   template<class T>
   using declval = std::declval<T>;

   using std::index_sequence;
   using std::make_index_sequence;

   template<class T> struct wrapper {
      [[no_unique_address]] T t;
      constexpr const auto& operator()() const {
         return t;
      }
   };
} // namespace utility

namespace mp {
   template<class...>
   struct type_list {};

   using info = const size_t*;

   template<info>
   struct meta_type {
      constexpr auto friend get(meta_type);
   };

   template<class T> struct meta_info {
      using value_type = T;
      static constexpr size_t id{};
      constexpr auto friend get(meta_type<&id>) {
         return meta_info{};
      }
   };

   template<class T>
   inline constexpr auto meta = &meta_info<T>::id;

   template<info meta>
   using type_of = typename decltype(get(meta_type<meta>{}))::value_type;

   template<template<class...> class T, const auto& v>
   constexpr auto apply() {
      return []<size_t... Ns>(utility::index_sequence<Ns...>) {
         return T<type_of<v[Ns]>...>{};
      }(utility::make_index_sequence<v.size()>{});
   }

   template<template<class...> class T, const auto& v>
   using apply_t = decltype(apply<T, v>());
} // namespace mp

template<class T, size_t N>
struct static_vector {
  constexpr static_vector() = default;
  constexpr auto push_back(const T& value) { values_[size_++] = value; }
  [[nodiscard]] constexpr const auto& operator[](auto i) const { return values_[i]; }
  [[nodiscard]] constexpr auto begin() const { return &values_[0]; }
  [[nodiscard]] constexpr auto end() const { return &values_[0] + size_; }
  [[nodiscard]] constexpr auto size() const { return size_; }
  [[nodiscard]] constexpr auto empty() const { return not size_; }
  [[nodiscard]] constexpr auto capacity() const { return N; }
  T values_[N]{};
  size_t size_{};
};

template<class... Ts>
  requires (__is_empty(Ts) and ...) and (sizeof...(Ts) < (1u << __CHAR_BIT__))
struct variant {
  template<class T> static constexpr auto id = [] {
    const bool match[]{std::is_same_v<Ts, T>...};
    for (auto i = 0; i < sizeof...(Ts); ++i) if (match[i]) return i;
    return -1;
  }();

  constexpr variant() noexcept = default;
  template<class T> constexpr variant(const T& t) noexcept : index{id<T>} { }

  template<class T> constexpr auto reset(const T&) noexcept requires (std::is_same_v<T, Ts> or ...) {
    index = id<T>;
    return true;
  }

  template<class... TArgs> requires ([]<class T> { return (std::is_same_v<T, TArgs> or ...); }.template operator()<Ts>() or ...)
  constexpr auto reset(const variant<TArgs...>& other) noexcept {
    if (other.index != -1) {
      index = other.index;
      return true;
    }
    return false;
  }

  char index{-1};
};

inline constexpr auto if_else = []<class Fn, template<class...> class T, class... Ts>(Fn&& fn, const T<Ts...>& v) {
  return [&]<size_t... Ns>(utility::index_sequence<Ns...>) {
    return ([&] {
      if (v.index == Ns) return fn(Ts{});
      return false;
    }() or ...);
  }(utility::make_index_sequence<sizeof...(Ts)>{});
};

inline constexpr auto jmp_table = []<class Fn, template<class...> class T, class... Ts>(Fn&& fn, const T<Ts...>& v) {
  static constexpr bool (*dispatch[])(Fn){[](Fn) { return false; }, [](Fn fn) { return fn(Ts{}); }...};
  return dispatch[v.index](fn);
};

template<class... Ts> struct overload : Ts... {
  using value_type = mp::type_list<Ts...>;
  using Ts::operator()...;
};

template<class... Ts> overload(Ts...) -> overload<Ts...>;

template<class T>
  requires (requires (T t) { t(); })
class sm {
  template<class TState>
  static constexpr auto add_state(auto& states, mp::type_list<TState>) {
    const auto new_state = mp::meta<TState>;
    if (new_state == mp::meta<void> or new_state == mp::meta<type_traits::none>) return;
    for (const auto& state : states) if (state == new_state) return;
    states.push_back(new_state);
  }

  template<template<class...> class TList, class... Ts>
  static constexpr auto add_state(auto& states, mp::type_list<TList<Ts...>>) {
    (add_state(states, mp::type_list<Ts>{}), ...);
  }

  static constexpr auto states = []<class... Ts>(mp::type_list<Ts...>) {
    static_vector<mp::info, 2u * sizeof...(Ts)> states{};
    (add_state(states, mp::type_list<typename type_traits::transition_traits<Ts>::src>{}), ...);
    (add_state(states, mp::type_list<typename type_traits::transition_traits<Ts>::dst>{}), ...);
    return states;
  }(typename std::remove_cvref_t<decltype(utility::declval<T>()())>::value_type{});

  template<class TEvent>
  static constexpr auto dispatchable =
    []<class... TStates>(mp::type_list<TStates...>) {
      return (requires(T t, TStates state, TEvent event) { t()(state, event); } or ...);
    }(mp::apply_t<mp::type_list, states>{});

 public:
  constexpr sm(const auto& t) : t_{t} { }

  template<class TEvent, auto dispatch = if_else> requires dispatchable<TEvent>
  constexpr auto process_event(const TEvent& event) -> bool {
    return dispatch([&](const auto& state) {
      if constexpr (requires { states_.reset(t_()(state, event)); }) {
        return states_.reset(t_()(state, event));
      } else if constexpr (requires { t_()(state, event); }) {
        t_()(state, event);
        return true;
      } else {
        return false;
      }
    }, states_);
  }

  template<auto dispatch = if_else> constexpr auto visit(auto&& fn) const {
    return dispatch(fn, states_);
  };

 private:
   T t_{};
  [[no_unique_address]] mp::apply_t<variant, states> states_ = mp::type_of<states[0u]>{};
};

template<class T> requires requires (T t) { t(); } sm(T) -> sm<T>;
template<class T> sm(T) -> sm<utility::wrapper<T>>;
struct X {}; // terminate state
} // namespace sml
