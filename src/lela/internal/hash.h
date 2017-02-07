// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// Some fast hash functions.

#ifndef LELA_INTERNAL_HASH_H_
#define LELA_INTERNAL_HASH_H_

#include <lela/internal/ints.h>

namespace lela {
namespace internal {

typedef u32 hash32_t;
typedef u64 hash64_t;

hash32_t jenkins_hash(u32 x) {
   x = (x+0x7ed55d16) + (x<<12);
   x = (x^0xc761c23c) ^ (x>>19);
   x = (x+0x165667b1) + (x<<5);
   x = (x+0xd3a2646c) ^ (x<<9);
   x = (x+0xfd7046c5) + (x<<3);
   x = (x^0xb55a4f09) ^ (x>>16);
   return x;
}

template<typename T>
hash64_t fnv1a_hash(const T& x, hash64_t seed = 0) {
  constexpr hash64_t kOffsetBasis = 0xcbf29ce484222325;
  constexpr hash64_t kMagicPrime = 0x00000100000001b3;
  hash64_t h = seed ^ kOffsetBasis;
  for (size_t i = 0; i < sizeof(x); ++i) {
    const std::uint8_t b = reinterpret_cast<const std::uint8_t*>(&x)[i];
    h ^= b;
    h *= kMagicPrime;
  }
  return h;
}

template<typename T>
u32 MurmurHash2(const T& x, u32 seed ) {
  // MurmurHash (32bit hash) by Austin Appleby (public domain).
  const u32 m = 0x5bd1e995;
  const int r = 24;
  u32 h = seed ^ sizeof(x);

  const u64* data = (const u64 *)reinterpret_cast<const u64*>(&x);
  size_t len = sizeof(x);

  while (sizeof(x) >= 4) {
    u32 k = *(u32*) data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  switch (len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
  }

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

template<typename T>
hash64_t murmur64a_hash(const T& x, hash64_t seed = 0) {
  // MurmurHash (64bit hash for 64bit CPU) by Austin Appleby (public domain).
  const hash64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  const u64* data = (const u64 *)reinterpret_cast<const u64*>(&x);
  const size_t len = sizeof(x);
  const u64* end = data + (len / 8);

  hash64_t h = seed ^ (len * m);

  while (data != end) {
    u64 k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  switch (len & 7) {
    case 7: h ^= static_cast<u64>(data[6]) << 48;
    case 6: h ^= static_cast<u64>(data[5]) << 40;
    case 5: h ^= static_cast<u64>(data[4]) << 32;
    case 4: h ^= static_cast<u64>(data[3]) << 24;
    case 3: h ^= static_cast<u64>(data[2]) << 16;
    case 2: h ^= static_cast<u64>(data[1]) << 8;
    case 1: h ^= static_cast<u64>(data[0]);
            h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_HASH_H_

