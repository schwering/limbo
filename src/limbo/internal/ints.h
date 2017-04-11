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
typedef std::int32_t i32;
typedef std::int64_t i64;
typedef std::uint8_t u8;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef std::size_t size_t;
typedef std::uintptr_t uptr_t;
typedef std::intptr_t iptr_t;

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_INTS_H_

