// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// 64bit FNV-1a hash implementation.

#ifndef LELA_INTERNAL_HASH_H_
#define LELA_INTERNAL_HASH_H_

#include <cstdint>

namespace lela {
namespace internal {

typedef std::uint64_t hash_t;

template<typename T>
hash_t fnv1a_hash(const T& x) {
  constexpr hash_t kOffsetBasis = 0xcbf29ce484222325;
  constexpr hash_t kMagicPrime = 0x00000100000001b3;
  hash_t h = kOffsetBasis;
  for (std::size_t i = 0; i < sizeof(x); ++i) {
    const std::uint8_t b = reinterpret_cast<const std::uint8_t*>(&x)[i];
    h ^= b;
    h *= kMagicPrime;
  }
  return h;
}

#if 0
template<>
hash_t fnv1a_hash(const std::uint64_t& x) {
  constexpr hash_t kOffsetBasis = 0xcbf29ce484222325;
  constexpr hash_t kMagicPrime = 0x00000100000001b3;
  return
      ((((((((((((((((
        kOffsetBasis
        ^ ((x >>  0) & 0xFF)) * kMagicPrime)
        ^ ((x >>  8) & 0xFF)) * kMagicPrime)
        ^ ((x >> 16) & 0xFF)) * kMagicPrime)
        ^ ((x >> 24) & 0xFF)) * kMagicPrime)
        ^ ((x >> 32) & 0xFF)) * kMagicPrime)
        ^ ((x >> 40) & 0xFF)) * kMagicPrime)
        ^ ((x >> 48) & 0xFF)) * kMagicPrime)
        ^ ((x >> 56) & 0xFF)) * kMagicPrime);
}
#endif

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_HASH_H_

