// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
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
  Maybe()                  : yes(false) {}
  explicit Maybe(T&& val)  : yes(true), val(std::forward<T>(val)) {}
  Maybe(bool yes, T&& val) : yes(yes), val(std::forward<T>(val)) {}

  Maybe(const Maybe&) = default;
  Maybe& operator=(const Maybe&) = default;
  template<typename U> Maybe(const Maybe<U>& m) : yes(m.yes), val(m.val) {}

  Maybe(Maybe&&) = default;
  Maybe& operator=(Maybe&&) = default;
  template<typename U> Maybe(Maybe<U>&& m) : yes(m.yes), val(m.val) {}

  ~Maybe() = default;

  bool operator==(const Maybe& m) const { return yes == m.yes && (!yes || val == m.val); }
  bool operator!=(const Maybe& m) const { return !(*this == m); }

  template<typename U> bool operator==(const Maybe<U>& m) const { return yes == m.yes && (!yes || val == m.val); }
  template<typename U> bool operator!=(const Maybe<U>& m) const { return !(*this == m); }

  explicit operator bool() const { return yes; }

  bool yes;
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

