// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A simple Maybe type as in Haskell: it is either Nothing or Just some object.
// Thanks to moving constructors and all that weird stuff, it's faster than
// using pairs with a boolean for the same purpose -- let alone much clearer.
//
// To handle Maybe<std::unique_ptr<T>> we use std::forward(). Is that correct?

#ifndef LELA_INTERNAL_MAYBE_H_
#define LELA_INTERNAL_MAYBE_H_

#include <cassert>

#include <memory>
#include <utility>

namespace lela {
namespace internal {

template<typename T>
struct Maybe {
  Maybe()                                           : succ(false) {}
  explicit Maybe(T&& val)                           : succ(true), val(std::forward<T>(val)) {}  // NOLINT
  template<typename U> explicit Maybe(const U& val) : succ(true), val(val) {}
  Maybe(bool succ, T&& val)                         : succ(succ), val(std::forward<T>(val)) {}  // NOLINT

  Maybe(const Maybe&) = default;
  Maybe(Maybe&&) = default;
  Maybe& operator=(Maybe&) = default;
  Maybe& operator=(Maybe&&) = default;
  ~Maybe() = default;

  template<typename U> Maybe(const Maybe<U>& m)            : succ(m.succ), val(m.val) {}  // NOLINT
  template<typename U> Maybe(Maybe<U>&& m)                 : succ(m.succ), val(m.val) {}  // NOLINT
  template<typename U> Maybe& operator=(const Maybe<U>& m) { succ = m.succ; val = m.val; return *this; }  // NOLINT
  template<typename U> Maybe& operator=(Maybe<U>&& m)      { succ = m.succ; val = m.val; return *this; }  // NOLINT

  bool operator==(const Maybe& m) const { return succ == m.succ && (!succ || val == m.val); }
  bool operator!=(const Maybe& m) const { return !(*this == m); }

  template<typename U> bool operator==(const Maybe<U>& m) const { return succ == m.succ && (!succ || val == m.val); }
  template<typename U> bool operator!=(const Maybe<U>& m) const { return !(*this == m); }

  operator bool() const { return succ; }

  bool succ;
  T val;
};

struct NothingType {
  template<typename T>
  operator Maybe<T>() const {
    return Maybe<T>();
  }
};

constexpr NothingType Nothing = NothingType();

template<typename T>
Maybe<typename std::remove_cv<typename std::remove_reference<T>::type>::type> Just(T&& val) {  // NOLINT
  return Maybe<T>(std::forward<T>(val));  // NOLINT
}

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_MAYBE_H_

