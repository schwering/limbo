// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Traits, mostly to extract and identify the arguments of functors.

#ifndef LELA_INTERNAL_TRAITS_H_
#define LELA_INTERNAL_TRAITS_H_

#include <tuple>
#include <type_traits>

namespace lela {
namespace internal {

// first_type<T, U>::type is T::type if it exists, and U::type else.
template<typename>
struct first_type_helper {
  typedef void type;
};

template<typename T, typename U, typename = void>
struct first_type {
  typedef typename U::type type;
};

template<typename T, typename U>
struct first_type<T, U, typename first_type_helper<typename T::type>::type> {
  typedef typename T::type type;
};

// arg<Function>::type<N> is the type of the Nth argument of Function().
template<typename Function>
struct arg : public arg<decltype(&Function::operator())> {};

template<typename Function, typename Return, typename... Args>
struct arg<Return (Function::*)(Args...) const> {
  template<int N = 0>
  using type = typename std::tuple_element<N, typename std::tuple<Args...>>::type;
};

// is_arg<Function, ExpType, N>::value is true iff the Nth argument of Function is
// convertible to ExpType, and false otherwise.
template<typename Function, typename ExpType, int N = 0>
struct is_arg {
  static constexpr bool value = std::is_convertible<typename arg<Function>::template type<N>, ExpType>::value;
};

// if_arg<Function, ExpType, Type, N>::type is defined as Type iff the Nth
// argument of Function is convertible to ExpType.
template<typename Function, typename ExpType, typename Type = void, int N = 0,
  bool = is_arg<Function, ExpType, N>::value>
struct if_arg {};

template<typename Function, typename ExpType, typename Type, int N>
struct if_arg<Function, ExpType, Type, N, true> {
  typedef Type type;
};

// remove_const_ref<T>::type removes the const and reference from T.
template<typename T>
struct remove_const_ref {
  typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
};

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_TRAITS_H_


