// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A Bloom filter allows for complete check whether an element is in a set.
// That is, it may yield false positives. This implementation is designed for
// small sets and specifically intended for clauses.
//
// Let m = 64 the size of the bitmask.
// Let k be the number of hash functions.
// Let n be the expected number of entries.
//
// The optimal n for given m and k is (m / n) * ln 2. (Says Wikipedia.)
//
// Supposing most clauses don't have more than 10 entries, 4 or 5 hash
// functions should be fine.
//
// We take the byte pairs 1,2 and 3,4 and 5,6 and 7,8 and consider the 16bit
// number formed by each of them as a single hash.

#ifndef LELA_INTERNAL_BLOOM_H_
#define LELA_INTERNAL_BLOOM_H_

#include <cstdint>

#include <functional>

#include <lela/internal/hash.h>

namespace lela {
namespace internal {

class BloomFilter {
 public:
  bool operator==(BloomFilter b) const { return mask_ == b.mask_; }
  bool operator!=(BloomFilter b) const { return !(*this == b); }

  std::uint64_t hash() const { return fnv1a_hash(mask_); }

  void Clear() {
    mask_ = 0;
  }

  void Add(std::uint64_t x) {
    mask_ |= (ONE << (hash<0>(x) % BITS))
          |  (ONE << (hash<1>(x) % BITS))
          |  (ONE << (hash<2>(x) % BITS))
          |  (ONE << (hash<3>(x) % BITS));
  }

  bool Contains(std::uint64_t x) const {
    return ( ((mask_ >> (hash<0>(x) % BITS)) & ONE)
           & ((mask_ >> (hash<1>(x) % BITS)) & ONE)
           & ((mask_ >> (hash<2>(x) % BITS)) & ONE)
           & ((mask_ >> (hash<3>(x) % BITS)) & ONE)) != 0;
  }

  bool SubsetOf(BloomFilter b) const {
    return Subset(*this, b);
  }

  bool DisjointWith(BloomFilter b) const {
    return Disjoint(*this, b);
  }

  static bool Subset(BloomFilter a, BloomFilter b) {
    return ~((~a.mask_) | b.mask_) == 0;
  }

  static bool Disjoint(BloomFilter a, BloomFilter b) {
    return (a.mask_ & b.mask_) == 0;
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(BloomFilterTest, hash);
#endif

  static constexpr std::uint64_t ONE = 1;  // use this constant because 1 is signed
  static constexpr std::uint64_t BITS = 64;

  template<std::uint64_t I>
  static std::uint64_t hash(std::uint64_t x) { return (x >> (I*16)) & 0xFFFF; }

  std::uint64_t mask_ = 0;
};

}  // namespace internal
}  // namespace lela


namespace std {

template<>
struct hash<lela::internal::BloomFilter> {
  size_t operator()(const lela::internal::BloomFilter a) const { return a.hash(); }
};

template<>
struct equal_to<lela::internal::BloomFilter> {
  bool operator()(const lela::internal::BloomFilter a, const lela::internal::BloomFilter b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_INTERNAL_BLOOM_H_

