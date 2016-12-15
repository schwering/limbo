// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
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

  static BloomFilter Union(BloomFilter a, BloomFilter b) {
    return BloomFilter(a.mask_ | b.mask_);
  }

  static BloomFilter Intersection(BloomFilter a, BloomFilter b) {
    return BloomFilter(a.mask_ & b.mask_);
  }

  bool operator==(BloomFilter b) const { return mask_ == b.mask_; }
  bool operator!=(BloomFilter b) const { return !(*this == b); }

  std::uint64_t hash() const { return fnv1a_hash(mask_); }

  void Clear() {
    mask_ = 0;
  }

  void Add(std::uint64_t x) {
    mask_ |= (ONE << shift<0>(x))
          |  (ONE << shift<1>(x))
          |  (ONE << shift<2>(x))
          |  (ONE << shift<3>(x));
  }

  bool Contains(std::uint64_t x) const {
    return (( (mask_ >> shift<0>(x))
            & (mask_ >> shift<1>(x))
            & (mask_ >> shift<2>(x))
            & (mask_ >> shift<3>(x))) & ONE) != 0;
  }

  void Unite(BloomFilter b) {
    mask_ |= b.mask_;
  }

  void Intersect(BloomFilter b) {
    mask_ &= b.mask_;
  }

  bool Includes(BloomFilter b) const {
    return Subset(b, *this);
  }

  bool Overlaps(BloomFilter b) const {
    return Overlap(*this, b);
  }

  static bool Subset(BloomFilter a, BloomFilter b) {
    return ~(~a.mask_ | b.mask_) == 0;
  }

  static bool Overlap(BloomFilter a, BloomFilter b) {
    return (a.mask_ & b.mask_) != 0;
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(BloomFilterTest, hash);
#endif

  explicit BloomFilter(std::uint64_t mask) : mask_(mask) {}

  template<std::uint64_t I>
  static std::uint64_t hash(std::uint64_t x) { return (x >> (I*16)) & 0xFFFF; }

  template<std::uint64_t I>
  static std::uint64_t shift(std::uint64_t x) { return hash<I>(x) % BITS; }

  static constexpr std::uint64_t ONE = 1;  // use this constant because 1 is signed
  static constexpr std::uint64_t BITS = 64;

  std::uint64_t mask_ = 0;
};

template<typename T>
class BloomSet {
 public:
  BloomSet() = default;

  static BloomSet Union(BloomSet a, BloomSet b) {
    return BloomSet(BloomFilter::Union(a.bf_, b.bf_));
  }

  static BloomSet Intersection(BloomSet a, BloomSet b) {
    return BloomSet(BloomFilter::Intersection(a.bf_, b.bf_));
  }

  bool operator==(BloomSet b) const { return bf_ == b.bf_; }
  bool operator!=(BloomSet b) const { return !(*this == b); }

  std::uint64_t hash() const { return bf_.hash(); }

  void Clear() {
    bf_.Clear();
  }

  void Add(const T& x) {
    const std::uint64_t h = x.hash();
    bf_.Add(h);
  }

  bool PossiblyContains(const T& x) const {
    const std::uint64_t h = x.hash();
    return bf_.Contains(h);
  }

  bool PossiblyIncludes(BloomSet b) const {
    return bf_.Includes(b.bf_);
  }

  bool PossiblyOverlaps(BloomSet b) const {
    return bf_.Overlaps(b.bf_);
  }

  void Unite(BloomSet b) {
    bf_.Unite(b.bf_);
  }

  void Intersect(BloomSet b) {
    bf_.Intersect(b.bf_);
  }

 private:
  explicit BloomSet(BloomFilter bf) : bf_(bf) {}

  BloomFilter bf_;
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

template<typename T>
struct hash<lela::internal::BloomSet<T>> {
  size_t operator()(const lela::internal::BloomSet<T> a) const { return a.hash(); }
};

template<typename T>
struct equal_to<lela::internal::BloomSet<T>> {
  bool operator()(const lela::internal::BloomSet<T> a, const lela::internal::BloomSet<T> b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_INTERNAL_BLOOM_H_

