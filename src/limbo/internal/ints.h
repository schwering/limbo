// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Integer typedefs.

#ifndef LIMBO_INTERNAL_INTS_H_
#define LIMBO_INTERNAL_INTS_H_

#include <cstdint>

namespace limbo {
namespace internal {

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::size_t size_t;
typedef std::uintptr_t uptr_t;
typedef std::intptr_t iptr_t;
typedef unsigned int uint_t;

template<typename T, int = sizeof(T)>
struct Bits {};

template<typename T>
struct Bits<T, sizeof(u16)> {
  static u32 interleave(u16 hi, u16 lo) { return _pdep_u32(hi, 0xAAAAAAAA) | _pdep_u32(lo, 0x55555555); }
  static u16 deinterleave_hi(u32 z) { return _pext_u32(z, 0xAAAAAAAA); }
  static u16 deinterleave_lo(u32 z) { return _pext_u32(z, 0x55555555); }
};

template<typename T>
struct Bits<T, sizeof(u32)> {
  static u64 interleave(u32 hi, u32 lo) { return _pdep_u64(hi, 0xAAAAAAAAAAAAAAAA) | _pdep_u64(lo, 0x5555555555555555); }
  static u32 deinterleave_hi(u64 z) { return _pext_u64(z, 0xAAAAAAAAAAAAAAAA); }
  static u32 deinterleave_lo(u64 z) { return _pext_u64(z, 0x5555555555555555); }
};

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_INTS_H_

