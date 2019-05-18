// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// DenseMap, DenseSet, MinHeap classes, which are all based on representing
// keys or entries, respectively, as non-negative integers close to zero.

#ifndef LIMBO_INTERNAL_DENSE_H_
#define LIMBO_INTERNAL_DENSE_H_

#include <algorithm>
#include <vector>

namespace limbo {
namespace internal {

template<typename Key, typename Index>
struct KeyToIndex {
  Index operator()(const Key k) const { return int(k); }
};

template<typename Key, typename Index>
struct IndexToKey {
  Key operator()(const Index i) const { return Key::FromId(i); }
};

struct NoBoundCheck {
  template<typename T, typename Index>
  void operator()(T*, const Index&) const {}
};

struct SlowAdjustBoundCheck {
  template<typename T, typename Index>
  void operator()(T* c, const Index& i) const {
    c->Capacitate(i);
  }
};

struct FastAdjustBoundCheck {
  template<typename T, typename Index>
  void operator()(T* c, const Index& i) const {
    c->Capacitate(next_power_of_two(i));
  }
};

template<typename Key,
         typename Val,
         typename CheckBound = NoBoundCheck,
         typename Index = int,
         typename KeyToIndex = KeyToIndex<Key, Index>,
         typename IndexToKey = IndexToKey<Key, Index>>
class DenseMap {
 public:
  using Vec             = std::vector<Val>;
  using value_type      = typename Vec::value_type;
  using reference       = typename Vec::reference;
  using const_reference = typename Vec::const_reference;
  using iterator        = typename Vec::iterator;
  using const_iterator  = typename Vec::const_iterator;

  // Wrapper for operator*() and operator++(int).
  template<typename InputIt>
  class IteratorProxy {
   public:
    typedef typename InputIt::value_type value_type;
    typedef typename InputIt::reference reference;
    typedef typename InputIt::pointer pointer;

    explicit IteratorProxy(reference v) : v_(v) {}
    reference operator*() const { return v_; }
    pointer operator->() const { return &v_; }

   private:
    mutable typename std::remove_const<value_type>::type v_;
  };

  struct KeyIterator {
    using difference_type = Index;
    using value_type = Key;
    using pointer = const value_type*;
    using reference = value_type;
    using iterator_category = std::random_access_iterator_tag;

    typedef IteratorProxy<KeyIterator> proxy;

    explicit KeyIterator(Index i, KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey())
        : k2i(k2i), i2k(i2k), index(i) {}

    bool operator==(KeyIterator it) const { return index == it.index; }
    bool operator!=(KeyIterator it) const { return !(*this == it); }

    reference operator*() const { return i2k(index); }
    KeyIterator& operator++() { ++index; return *this; }
    KeyIterator& operator--() { --index; return *this; }

    proxy operator->() const { return proxy(operator*()); }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }
    proxy operator--(int) { proxy p(operator*()); operator--(); return p; }

    KeyIterator& operator+=(difference_type n) { index += n; return *this; }
    KeyIterator& operator-=(difference_type n) { index -= n; return *this; }
    friend KeyIterator operator+(KeyIterator it, difference_type n) { it += n; return it; }
    friend KeyIterator operator+(difference_type n, KeyIterator it) { it += n; return it; }
    friend KeyIterator operator-(KeyIterator it, difference_type n) { it -= n; return it; }
    friend difference_type operator-(KeyIterator a, KeyIterator b) { return a.index - b.index; }
    reference operator[](difference_type n) const { return *(*this + n); }
    bool operator<(KeyIterator it) const { return index < it.index; }
    bool operator>(KeyIterator it) const { return index > it.index; }
    bool operator<=(KeyIterator it) const { return !(*this > it); }
    bool operator>=(KeyIterator it) const { return !(*this < it); }

   private:
    KeyToIndex k2i;
    IndexToKey i2k;
    Index index;
  };

  template<typename T>
  struct Range {
    Range(T begin, T end) : begin_(begin), end_(end) {}
    T begin() const { return begin_; }
    T end()   const { return end_; }
   private:
    T begin_;
    T end_;
  };

  explicit DenseMap(KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey()) : k2i_(k2i), i2k_(i2k) {}

  explicit DenseMap(Index max, KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey())
      : DenseMap(k2i, i2k) { Capacitate(max); }

  explicit DenseMap(Index max, Val init, KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey())
      : DenseMap(k2i, i2k) { Capacitate(max, init); }

  explicit DenseMap(Key max, KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey())
      : DenseMap(k2i(max), k2i, i2k) {}

  explicit DenseMap(Key max, Val init, KeyToIndex k2i = KeyToIndex(), IndexToKey i2k = IndexToKey())
      : DenseMap(k2i(max), init, k2i, i2k) {}

  DenseMap(const DenseMap&)              = default;
  DenseMap& operator=(const DenseMap& c) = default;
  DenseMap(DenseMap&&)                   = default;
  DenseMap& operator=(DenseMap&& c)      = default;

  void Capacitate(Key k)          { Capacitate(k2i_(k)); }
  void Capacitate(Key k, Val v)   { Capacitate(k2i_(k), v); }
  void Capacitate(Index i)        { if (i >= vec_.size()) { vec_.resize(i + 1); } }
  void Capacitate(Index i, Val v) { if (i >= vec_.size()) { vec_.resize(i + 1, v); } }

  void Clear() { vec_.clear(); }
  bool Cleared() { return vec_.empty(); }

  Index upper_bound() const { return Index(vec_.size()) - 1; }

        reference operator[](Index i)       { check_bound_(this, i); return vec_[i]; }
  const_reference operator[](Index i) const { check_bound_(const_cast<DenseMap*>(this), i); return vec_[i]; }

        reference operator[](Key key)       { return operator[](k2i_(key)); }
  const_reference operator[](Key key) const { return operator[](k2i_(key)); }

  Range<KeyIterator> keys() const {
    return Range<KeyIterator>(KeyIterator(0, k2i_, i2k_), KeyIterator(vec_.size(), k2i_, i2k_));
  }
  Range<const_iterator> values() const { return Range<const_iterator>(vec_.begin(), vec_.end()); }
  Range<      iterator> values()       { return Range<      iterator>(vec_.begin(), vec_.end()); }

 private:
  CheckBound check_bound_;
  KeyToIndex k2i_;
  IndexToKey i2k_;
  Vec vec_;
};

template<typename T,
         typename Less,
         typename CheckBound = NoBoundCheck,
         typename Index = int,
         typename KeyToIndex = KeyToIndex<T, Index>>
class MinHeap {
 public:
  using Map = DenseMap<T, int, CheckBound, Index, KeyToIndex>;

  explicit MinHeap(Less less = Less()) : less_(less) { heap_.emplace_back(); }

  MinHeap(const MinHeap&)            = default;
  MinHeap& operator=(const MinHeap&) = default;
  MinHeap(MinHeap&&)                 = default;
  MinHeap& operator=(MinHeap&&)      = default;

  void set_less(Less less) { less_ = less; }

  void Capacitate(const T x) { index_.Capacitate(x); }
  void Capacitate(const int i) { index_.Capacitate(i); }

  void Clear() { heap_.clear(); index_.Clear(); heap_.emplace_back(); }

  int size()  const { return heap_.size() - 1; }
  bool empty() const { return heap_.size() == 1; }
  const T& operator[](int i) const { return heap_[i + 1]; }

  bool contains(const T& x) const { return index_[x] != 0; }

  T top() const { return heap_[bool(size())]; }

  void Increase(const T& x) {
    assert(contains(x));
    SiftUp(index_[x]);
    assert(contains(x));
    assert(std::min_element(heap_.begin() + 1, heap_.end(), less_) == heap_.begin() + 1);
  }

  void Decrease(const T& x) {
    Remove(x);
    Insert(x);
  }

  void Insert(const T& x) {
    assert(!contains(x));
    const int i = heap_.size();
    heap_.push_back(x);
    index_[x] = i;
    SiftUp(i);
    assert(std::min_element(heap_.begin() + 1, heap_.end(), less_) == heap_.begin() + 1);
  }

  void Remove(const T& x) {
    assert(contains(x));
    const int i = index_[x];
    heap_[i] = heap_.back();
    index_[heap_[i]] = i;
    heap_.pop_back();
    index_[x] = 0;
    if (heap_.size() > i) {
      SiftDown(i);
      SiftUp(i);
    }
    assert(!contains(x));
    assert(std::min_element(heap_.begin() + 1, heap_.end(), less_) == heap_.begin() + 1);
  }

  typename std::vector<T>::const_iterator begin() const { return heap_.begin() + 1; }
  typename std::vector<T>::const_iterator end()   const { return heap_.end(); }

 private:
  static int left(const int i)   { return 2 * i; }
  static int right(const int i)  { return 2 * i + 1; }
  static int parent(const int i) { return i / 2; }

  void SiftUp(int i) {
    assert(i > 0 && i < heap_.size());
    const T x = heap_[i];
    int p;
    while ((p = parent(i)) != 0 && less_(x, heap_[p])) {
      heap_[i] = heap_[p];
      index_[heap_[i]] = i;
      i = p;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  void SiftDown(int i) {
    assert(i > 0 && i < heap_.size());
    const T x = heap_[i];
    while (left(i) < heap_.size()) {
      const int min_child = right(i) < heap_.size() && less_(heap_[right(i)], heap_[left(i)]) ? right(i) : left(i);
      if (!less_(heap_[min_child], x)) {
        break;
      }
      heap_[i] = heap_[min_child];
      index_[heap_[i]] = i;
      i = min_child;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  Less less_;
  std::vector<T> heap_;
  Map index_;
};

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_DENSE_H_

