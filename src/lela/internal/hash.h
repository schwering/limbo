// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// 64bit FNV-1a hash implementation.

#ifndef LELA_INTERNAL_HASH_H_
#define LELA_INTERNAL_HASH_H_

#include <cstdint>

namespace lela {
namespace internal {

inline std::uint64_t fnv1a_hash(const std::uint64_t b) {
  constexpr std::uint64_t offset_basis = 0xcbf29ce484222325;
  constexpr std::uint64_t magic_prime = 0x00000100000001b3;
  return
      ((((((((((((((((
        offset_basis
        ^ ((b >>  0) & 0xFF)) * magic_prime)
        ^ ((b >>  8) & 0xFF)) * magic_prime)
        ^ ((b >> 16) & 0xFF)) * magic_prime)
        ^ ((b >> 24) & 0xFF)) * magic_prime)
        ^ ((b >> 32) & 0xFF)) * magic_prime)
        ^ ((b >> 40) & 0xFF)) * magic_prime)
        ^ ((b >> 48) & 0xFF)) * magic_prime)
        ^ ((b >> 56) & 0xFF)) * magic_prime);
}

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_HASH_H_

