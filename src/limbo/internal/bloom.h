// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
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

#ifndef LIMBO_INTERNAL_BLOOM_H_
#define LIMBO_INTERNAL_BLOOM_H_

#include <functional>

#include <limbo/internal/hash.h>
#include <limbo/internal/ints.h>

namespace limbo {
namespace internal {

class BloomFilter {
 public:
  BloomFilter() = default;

  static BloomFilter Union(const BloomFilter a, const BloomFilter b)        { return BloomFilter(a.mask_ | b.mask_); }
  static BloomFilter Intersection(const BloomFilter a, const BloomFilter b) { return BloomFilter(a.mask_ & b.mask_); }

  bool operator==(const BloomFilter b) const { return mask_ == b.mask_; }
  bool operator!=(const BloomFilter b) const { return !(*this == b); }

  void Clear() { mask_ = 0; }

  template<typename HashType>
  void Add(const HashType x) {
    mask_ |= (static_cast<mask_t>(1) << index<0>(x))
          |  (static_cast<mask_t>(1) << index<1>(x))
          |  (static_cast<mask_t>(1) << index<2>(x))
          |  (static_cast<mask_t>(1) << index<3>(x));
  }

  template<typename HashType>
  bool Contains(const HashType x) const {
    return (( (mask_ >> index<0>(x))
            & (mask_ >> index<1>(x))
            & (mask_ >> index<2>(x))
            & (mask_ >> index<3>(x))) & static_cast<mask_t>(1)) != 0;
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

  typedef internal::u64 bit_index_t;
  typedef internal::u64 mask_t;

  explicit BloomFilter(const mask_t& mask) : mask_(mask) {}

  template<size_t I, typename HashType>
  static bit_index_t index(HashType x) {
    // index() should slice the original HashType x into several bit_index_t,
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
    static_assert((~static_cast<HashType>(0) & kMaxIndex) != 0, "HashType does not provide enough bits");
    static_assert((~static_cast<mask_t>(0) >> kMaxIndex) == 1, "mask does not cover mask_t indices");
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
}  // namespace limbo


namespace std {

template<>
struct equal_to<limbo::internal::BloomFilter> {
  bool operator()(const limbo::internal::BloomFilter& a, const limbo::internal::BloomFilter& b) const { return a == b; }
};

template<typename T>
struct equal_to<limbo::internal::BloomSet<T>> {
  bool operator()(const limbo::internal::BloomSet<T>& a, const limbo::internal::BloomSet<T>& b) const { return a == b; }
};

}  // namespace std

#endif  // LIMBO_INTERNAL_BLOOM_H_

