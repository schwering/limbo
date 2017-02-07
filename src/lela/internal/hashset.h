// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
//
// Hash set.

#ifndef LELA_INTERNAL_HASHSET_H_
#define LELA_INTERNAL_HASHSET_H_

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <utility>
#include <vector>

#include <lela/internal/hash.h>
#include <lela/internal/ints.h>
#include <lela/internal/iter.h>

namespace lela {
namespace internal {

template<typename T, typename Hash = std::hash<T>, typename Equal = std::equal_to<T>>
class HashSet {
 public:
  typedef typename std::result_of<Hash(T)>::type hash_t;

  struct Cell {
    Cell() = default;
    Cell(const T& val, hash_t hash) : val(val), hash(hash) {}
    Cell(T&& val, hash_t hash) : val(std::forward<T>(val)), hash(hash) {}

    static hash_t mask(const hash_t h) { return h & ~(kRemoved | kFresh); }

    void MarkRemoved() { hash = kRemoved; }

    bool occupied() const { return (hash & (kRemoved | kFresh)) == 0; }
    bool removed() const { return (hash & kRemoved) != 0; }
    bool fresh() const { return (hash & kFresh) != 0; }

    T val;
    hash_t hash = kFresh;

   private:
    static constexpr hash_t kRemoved  = static_cast<hash_t>(1) << (sizeof(hash_t) * 8 - 2);
    static constexpr hash_t kFresh    = static_cast<hash_t>(1) << (sizeof(hash_t) * 8 - 1);
  };

  template<typename U, typename InputIt>
  struct Iterator {
    typedef std::ptrdiff_t difference_type;
    typedef U value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::forward_iterator_tag iterator_category;
    typedef iterator_proxy<Iterator> proxy;

    Iterator() = default;
    explicit Iterator(InputIt end) : now_(end) { }
    Iterator(InputIt now, InputIt end) : now_(now), end_(end) { Next(); }

    bool operator==(Iterator it) const { return now_ == it.now_; }
    bool operator!=(Iterator it) const { return !(*this == it); }

    reference operator*() const { return now_->val; }
    Iterator& operator++() {
      ++now_;
      Next();
      return *this;
    }

    pointer operator->() const { return &now_->val; }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

    Cell* cell() const { return const_cast<Cell*>(now_.operator->()); }

   private:
    void Next() {
      while (now_ != end_ && !now_->occupied()) {
        ++now_;
      }
    }

    InputIt now_;
    InputIt end_;
  };

  template<typename U, typename InputIt>
  struct BucketIterator {
    typedef std::ptrdiff_t difference_type;
    typedef U value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::forward_iterator_tag iterator_category;
    typedef iterator_proxy<BucketIterator> proxy;

    BucketIterator() = default;
    explicit BucketIterator(InputIt end) : now_(end) {}
    BucketIterator(hash_t h, InputIt now, InputIt begin, InputIt end) :
        hash_(h), now_(now), now_bak_(now), begin_(begin), end_(end) { Next(true); }

    bool operator==(BucketIterator it) const { return now_ == it.now_; }
    bool operator!=(BucketIterator it) const { return !(*this == it); }

    reference operator*() const { return now_->val; }
    BucketIterator& operator++() {
      ++now_;
      Next();
      return *this;
    }

    pointer operator->() const { return &now_->val; }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

    Cell* cell() const { return const_cast<Cell*>(now_.operator->()); }

   private:
    void Next(bool first = false) {
      while ((first || now_ != now_bak_) && !cell()->fresh() && (cell()->removed() || cell()->hash != hash_)) {
        if (now_ == end_) {
          now_ = begin_;
        } else {
          ++now_;
        }
        first = false;
      }
      if ((!first && now_ == now_bak_) || cell()->fresh()) {
        now_ = end_;
      }
    }

    hash_t hash_;
    InputIt now_;
    InputIt now_bak_;
    InputIt begin_;
    InputIt end_;
  };

  typedef Iterator<T,       typename std::vector<Cell>::iterator>       iterator;
  typedef Iterator<const T, typename std::vector<Cell>::const_iterator> const_iterator;

  typedef BucketIterator<T,       typename std::vector<Cell>::iterator>       bucket_iterator;
  typedef BucketIterator<const T, typename std::vector<Cell>::const_iterator> const_bucket_iterator;

  explicit HashSet(size_t capacity = 0, Hash hash = Hash(), Equal equal = Equal()) :
      hash_(hash),
      equal_(equal),
      vec_(Capacity(capacity)) {}

  template<typename ForwardIt>
  HashSet(ForwardIt begin, ForwardIt end, Hash hash = Hash(), Equal equal = Equal()) :
      HashSet(std::distance(begin, end), hash, equal) {
    for (; begin != end; ++begin) {
      Add(*begin);
    }
  }

  template<typename U>
  HashSet(std::initializer_list<U> xs, Hash hash = Hash(), Equal equal = Equal()) :
      HashSet(xs.size(), hash, equal) {
    for (auto& x : xs) {
      Add(x);
    }
  }

  HashSet(const HashSet&) = default;
  HashSet& operator=(const HashSet& s) = default;

  HashSet(HashSet&&) = default;
  HashSet& operator=(HashSet&& s) = default;

  iterator begin() { return iterator(vec_.begin(), vec_.end()); }
  iterator end()   { return iterator(vec_.end()); }

  const_iterator begin() const { return const_iterator(vec_.begin(), vec_.end()); }
  const_iterator end()   const { return const_iterator(vec_.end()); }

  bucket_iterator bucket(hash_t h) {
    h = Cell::mask(h);
    return bucket_iterator(h, vec_.begin() + (h % capacity()), vec_.begin(), vec_.end());
  }

  bucket_iterator bucket_begin(const T& val) { return bucket_begin(hash(val)); }
  bucket_iterator bucket_begin(hash_t h) {
    h = Cell::mask(h);
    return bucket_iterator(h, vec_.begin() + (h % capacity()), vec_.begin(), vec_.end());
  }
  bucket_iterator bucket_end() { return bucket_iterator(vec_.end()); }

  const_bucket_iterator bucket_begin(const T& val) const { return bucket_begin(hash(val)); }
  const_bucket_iterator bucket_begin(hash_t h) const {
    h = Cell::mask(h);
    return const_bucket_iterator(h, vec_.begin() + (h % capacity()), vec_.begin(), vec_.end());
  }
  const_bucket_iterator bucket_end() const { return const_bucket_iterator(vec_.end()); }

  size_t capacity() const { return vec_.size(); }
  size_t size()     const { return size_; }
  bool   empty()    const { return size_ == 0; }

  bool Add(const T& val) {
    Rehash(size_ + 1);
    const hash_t h = hash(val);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash && equal_(vec_[i].val, val)) {
        return false;
      }
      ++i;
      i %= capacity();
    }
    vec_[i] = Cell(val, h);
    ++size_;
    return true;
  }

  bool Add(T&& val) {
    Rehash(size_ + 1);
    const hash_t h = hash(val);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash && equal_(vec_[i].val, val)) {
        return false;
      }
      ++i;
      i %= capacity();
    }
    vec_[i] = Cell(std::forward<T>(val), h);
    ++size_;
    return true;
  }

  bool Remove(const T& val) {
    const hash_t h = hash(val);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash && equal_(vec_[i].val, val)) {
        vec_[i].MarkRemoved();
        --size_;
        return true;
      }
      ++i;
      i %= capacity();
    }
    return false;
  }

  void Remove(iterator it)              { it.cell()->MarkRemoved(); --size_; }
  void Remove(const_iterator it)        { it.cell()->MarkRemoved(); --size_; }
  void Remove(bucket_iterator it)       { it.cell()->MarkRemoved(); --size_; }
  void Remove(const_bucket_iterator it) { it.cell()->MarkRemoved(); --size_; }

  bool RemoveHash(hash_t h) {
    h = Cell::mask(h);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash) {
        vec_[i].MarkRemoved();
        --size_;
        return true;
      }
      ++i;
      i %= capacity();
    }
    return false;
  }

  void RemoveAllHashes(hash_t h) {
    h = Cell::mask(h);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash) {
        vec_[i].MarkRemoved();
        --size_;
      }
      ++i;
      i %= capacity();
    }
  }

  bool Contains(const T& val) const {
    const hash_t h = hash(val);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash && equal_(val, vec_[i].val)) {
        return true;
      }
      ++i;
      i %= capacity();
    }
    return false;
  }

  bool ContainsHash(hash_t h) const {
    h = Cell::mask(h);
    hash_t i = h % capacity();
    size_t c = capacity();
    while (!vec_[i].fresh() && c-- > 0) {
      if (h == vec_[i].hash) {
        return true;
      }
      ++i;
      i %= capacity();
    }
    return false;
  }

 private:
  hash_t hash(const T& val) const { return Cell::mask(hash_(val)); }

  static size_t Capacity(size_t cap) {
    static const size_t primes[] = { 3, 7, 11, 23, 31, 73, 151, 313, 643, 1291, 2593, 5233, 10501, 21013, 42073,
      84181, 168451, 337219, 674701, 1349473, 2699299, 5398891, 10798093, 21596719, 43193641, 86387383, 172775299,
      345550609, 691101253 };
    static const size_t n_primes = sizeof(primes) / sizeof(primes[0]);
    for (size_t i = 1; i < n_primes; ++i) {
      if (cap + cap / 2 <= primes[i]) {
        return primes[i];
      }
    }
    return primes[n_primes - 1];
  }

  void Rehash(size_t cap) {
    cap = Capacity(cap);
    if (cap > capacity()) {
      std::vector<Cell> old(cap);
      std::swap(vec_, old);
      for (size_t i = 0; i < old.size(); ++i) {
        if (old[i].occupied()) {
          Add(std::forward<T>(old[i].val));
        }
      }
    }
  }

  Hash hash_;
  Equal equal_;
  size_t size_ = 0;
  std::vector<Cell> vec_;
};

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_HASHSET_H_

