// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017--2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Integer utilities.

#ifndef LIMBO_INTERNAL_INTS_H_
#define LIMBO_INTERNAL_INTS_H_

#include <cstdint>

namespace limbo {
namespace internal {

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using uint  = unsigned int;
using ulong = unsigned long;
using size_t = std::size_t;
using uptr_t = std::uintptr_t;
using iptr_t = std::intptr_t;

template<typename T, int = sizeof(T)>
struct Bits {};

template<typename T>
struct Bits<T, sizeof(u16)> {
  static constexpr u32 kHi = 0xAAAAAAAA;
  static constexpr u32 kLo = 0x55555555;
  static u32 interleave(u16 hi, u16 lo) { return _pdep_u32(hi, kHi) | _pdep_u32(lo, kLo); }
  static u16 deinterleave_hi(u32 z) { return _pext_u32(z, kHi); }
  static u16 deinterleave_lo(u32 z) { return _pext_u32(z, kLo); }
};

template<typename T>
struct Bits<T, sizeof(u32)> {
  static constexpr u64 kHi = 0xAAAAAAAAAAAAAAAA;
  static constexpr u64 kLo = 0x5555555555555555;
  static u64 interleave(u32 hi, u32 lo) { return _pdep_u64(hi, kHi) | _pdep_u64(lo, kLo); }
  static u32 deinterleave_hi(u64 z) { return _pext_u64(z, kHi); }
  static u32 deinterleave_lo(u64 z) { return _pext_u64(z, kLo); }
};

ulong next_power_of_two(ulong n) {
  n += !n;
  return static_cast<ulong>(1) << (sizeof(ulong) * 8 - 1 - __builtin_clzl(n+n-1));
}

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_INTS_H_

