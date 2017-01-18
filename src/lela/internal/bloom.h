// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
//
// A BloomFilter allows for complete check whether an element is in a set.
// That is, it may yield false positives. The class BloomSet<T> exists to
// make this set interpretation clear.
//
// This implementation is designed for small sets and specifically intended
// for clauses.
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
  BloomFilter() = default;

  static BloomFilter Union(const BloomFilter& a, const BloomFilter& b)        { return BloomFilter(a.mask_ | b.mask_); }
  static BloomFilter Intersection(const BloomFilter& a, const BloomFilter& b) { return BloomFilter(a.mask_ & b.mask_); }

  bool operator==(const BloomFilter& b) const { return mask_ == b.mask_; }
  bool operator!=(const BloomFilter& b) const { return !(*this == b); }

  hash_t hash() const { return fnv1a_hash(mask_); }

  void Clear() { mask_ = 0; }

  void Add(const hash_t& x) {
    mask_ |= (mask_t(1) << index<0>(x))
          |  (mask_t(1) << index<1>(x))
          |  (mask_t(1) << index<2>(x))
          |  (mask_t(1) << index<3>(x));
  }

  bool Contains(const hash_t& x) const {
    return (( (mask_ >> index<0>(x))
            & (mask_ >> index<1>(x))
            & (mask_ >> index<2>(x))
            & (mask_ >> index<3>(x))) & mask_t(1)) != 0;
  }

  void Union(const BloomFilter& b)     { mask_ |= b.mask_; }
  void Intersect(const BloomFilter& b) { mask_ &= b.mask_; }

  bool SubsetOf(const BloomFilter& b) const { return Subset(*this, b); }
  bool Overlaps(const BloomFilter& b) const { return Overlap(*this, b); }

  static bool Subset(const BloomFilter& a, const BloomFilter& b)  { return ~(~a.mask_ | b.mask_) == 0; }
  static bool Overlap(const BloomFilter& a, const BloomFilter& b) { return (a.mask_ & b.mask_) != 0; }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(BloomFilterTest, hash);
#endif

  typedef std::uint64_t bit_index_t;
  typedef std::uint64_t mask_t;

  explicit BloomFilter(const mask_t& mask) : mask_(mask) {}

  template<std::size_t I>
  static bit_index_t index(hash_t x) {
    // index() should slice the original hash_t x into several bit_index_t,
    // whose range shall be [0 ... bits(mask_t) - 1], that is, the indices
    // of the bits in mask_t.
    //
    // When mask_t is a 64 bit integer, we just need log_2(64) = 6 bits for
    // an index. So we can do is take the Ith byte and return its value
    // modulo 64, which then gives a hash in the range [0 ... 63]:
    //
    // return ((x >> (I*8)) & 0xFF) % (sizeof(mask_t) * 8);
    //
    // But since 63 is just binary 111111, we can simply take the six
    // right-most bits of the byte:
    constexpr bit_index_t kMaxIndex = 63;
    static_assert((~mask_t(0) >> kMaxIndex) == 1, "mask does not cover mask_t indices");
    return (x >> (I*8)) & kMaxIndex;
  }

  mask_t mask_ = 0;
};

template<typename T>
class BloomSet {
 public:
  BloomSet() = default;

  static BloomSet Union(const BloomSet& a, const BloomSet& b) {
    return BloomSet(BloomFilter::Union(a.bf_, b.bf_));
  }
  static BloomSet Intersection(const BloomSet& a, const BloomSet& b) {
    return BloomSet(BloomFilter::Intersection(a.bf_, b.bf_));
  }

  bool operator==(const BloomSet& b) const { return bf_ == b.bf_; }
  bool operator!=(const BloomSet& b) const { return !(*this == b); }

  hash_t hash() const { return bf_.hash(); }

  void Clear()                      { bf_.Clear(); }
  void Add(const T& x)              { bf_.Add(x.hash()); }
  void Union(const BloomSet& b)     { bf_.Union(b.bf_); }
  void Intersect(const BloomSet& b) { bf_.Intersect(b.bf_); }

  bool PossiblyContains(const T& x)        const { return bf_.Contains(x.hash()); }
  bool PossiblySubsetOf(const BloomSet& b) const { return bf_.SubsetOf(b.bf_); }
  bool PossiblyOverlaps(const BloomSet& b) const { return bf_.Overlaps(b.bf_); }

 private:
  explicit BloomSet(const BloomFilter& bf) : bf_(bf) {}

  BloomFilter bf_;
};

}  // namespace internal
}  // namespace lela


namespace std {

template<>
struct hash<lela::internal::BloomFilter> {
  std::size_t operator()(const lela::internal::BloomFilter& a) const { return a.hash(); }
};

template<>
struct equal_to<lela::internal::BloomFilter> {
  bool operator()(const lela::internal::BloomFilter& a, const lela::internal::BloomFilter& b) const { return a == b; }
};

template<typename T>
struct hash<lela::internal::BloomSet<T>> {
  std::size_t operator()(const lela::internal::BloomSet<T>& a) const { return a.hash(); }
};

template<typename T>
struct equal_to<lela::internal::BloomSet<T>> {
  bool operator()(const lela::internal::BloomSet<T>& a, const lela::internal::BloomSet<T>& b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_INTERNAL_BLOOM_H_

