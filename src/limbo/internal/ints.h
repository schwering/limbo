// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Integer typedefs.

#ifndef LIMBO_INTERNAL_INTS_H_
#define LIMBO_INTERNAL_INTS_H_

#include <cstdint>

#include <functional>
#include <utility>

namespace limbo {
namespace internal {

typedef std::int8_t i8;
typedef std::int32_t i32;
typedef std::int64_t i64;
typedef std::uint8_t u8;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::size_t size_t;
typedef std::uintptr_t uptr_t;
typedef std::intptr_t iptr_t;
typedef unsigned int uint_t;

template<typename T>
struct Integer {
  Integer() = default;
  explicit Integer(size_t i) : i(i) {}
  explicit operator T() const { return i; }

  Integer& operator++() { ++i; return *this; }
  Integer& operator--() { --i; return *this; }

  Integer operator++(int) { size_t j = i; ++i; return Integer(j); }
  Integer operator--(int) { size_t j = i; --i; return Integer(j); }

  Integer& operator+=(Integer j) { i += j.i; return *this; }
  Integer& operator-=(Integer j) { i -= j.i; return *this; }

  Integer operator+(Integer j) const { return Integer(i + j.i); }
  Integer operator-(Integer j) const { return Integer(i - j.i); }
  Integer operator*(Integer j) const { return Integer(i * j.i); }
  Integer operator/(Integer j) const { return Integer(i / j.i); }

  bool operator< (Integer j) const { return i < j.i; }
  bool operator<=(Integer j) const { return i <= j.i; }
  bool operator!=(Integer j) const { return i != j.i; }
  bool operator==(Integer j) const { return i == j.i; }
  bool operator> (Integer j) const { return i > j.i; }
  bool operator>=(Integer j) const { return i >= j.i; }

 private:
  T i;
};

}  // namespace internal
}  // namespace limbo


namespace std {

template<typename T>
struct equal_to<limbo::internal::Integer<T>> {
  bool operator()(limbo::internal::Integer<T> i, limbo::internal::Integer<T> j) const { return i == j; }
};

template<typename T>
struct hash<limbo::internal::Integer<T>> {
  size_t operator()(limbo::internal::Integer<T> i) const { return size_t(T(i)); }
};

}  // namespace std


#endif  // LIMBO_INTERNAL_INTS_H_

