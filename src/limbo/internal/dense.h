// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// DenseMap, DenseSet, MinHeap classes, which are all based on representing
// keys or entries, respectively, as non-negative integers close to zero.

#ifndef LIMBO_DENSE_H_
#define LIMBO_DENSE_H_

#include <algorithm>
#include <vector>

namespace limbo {
namespace internal {

template<typename Key, typename Index>
struct KeyToIndex {
  Index operator()(const Key k) const { return int(k); }
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
         typename KeyToIndex = KeyToIndex<Key, Index>>
class DenseMap {
 public:
  using Vec             = std::vector<Val>;
  using value_type      = typename Vec::value_type;
  using reference       = typename Vec::reference;
  using const_reference = typename Vec::const_reference;
  using iterator        = typename Vec::iterator;
  using const_iterator  = typename Vec::const_iterator;

  DenseMap() = default;

  DenseMap(const DenseMap&)              = default;
  DenseMap& operator=(const DenseMap& c) = default;
  DenseMap(DenseMap&&)                   = default;
  DenseMap& operator=(DenseMap&& c)      = default;

  void Capacitate(Key k) { Capacitate(k2i_(k)); }
  void Capacitate(Index i) { if (i >= vec_.size()) { vec_.resize(i + 1); } }

  void Clear() { vec_.clear(); }
  bool Cleared() { return vec_.empty(); }

  Index upper_bound() const { return vec_.size(); }

        reference operator[](Index i)       { check_bound_(this, i); return vec_[i]; }
  const_reference operator[](Index i) const { check_bound_(const_cast<DenseMap*>(this), i); return vec_[i]; }

        reference operator[](Key key)       { return operator[](k2i_(key)); }
  const_reference operator[](Key key) const { return operator[](k2i_(key)); }

  iterator begin() { return vec_.begin(); }
  iterator end()   { return vec_.end(); }

  const_iterator begin() const { return vec_.begin(); }
  const_iterator end()   const { return vec_.end(); }

 private:
  CheckBound check_bound_;
  KeyToIndex k2i_;
  Vec vec_;
};

template<typename T,
         typename CheckBound = NoBoundCheck,
         typename Index = int,
         typename KeyToIndex = KeyToIndex<T, Index>>
class DenseSet {
 public:
  using Map             = DenseMap<T, T, CheckBound, Index, KeyToIndex>;
  using value_type      = typename Map::value_type;
  using reference       = typename Map::reference;
  using const_reference = typename Map::const_reference;
  using iterator        = typename Map::iterator;
  using const_iterator  = typename Map::const_iterator;

  DenseSet() = default;

  DenseSet(const DenseSet&)              = default;
  DenseSet& operator=(const DenseSet& c) = default;
  DenseSet(DenseSet&&)                   = default;
  DenseSet& operator=(DenseSet&& c)      = default;

  void Capacitate(Index i) { map_.Capacitate(i); }
  void Capacitate(const T& x) { map_.Capacitate(x); }

  void Clear() { map_.Clear(); }
  bool Cleared() { return map_.empty(); }

  Index upper_bound() const { return map_.upper_bound(); }

  bool Contains(const T& x) const { return !x.null() && map_[x] == x; }

  void Insert(const T& x) { assert(!x.null()); map_[x] = x; }
  void Remove(const T& x) { assert(!x.null()); map_[x] = T(); }

        reference operator[](Index i)       { return map_[i]; }
  const_reference operator[](Index i) const { return map_[i]; }

  iterator begin() { return map_.begin(); }
  iterator end()   { return map_.end(); }

  const_iterator begin() const { return map_.begin(); }
  const_iterator end()   const { return map_.end(); }

 private:
  Map map_;
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

  bool Contains(const T& x) const { return index_[x] != 0; }

  T top() const { return heap_[bool(size())]; }

  void Increase(const T& x) {
    assert(Contains(x));
    SiftUp(index_[x]);
    assert(Contains(x));
    assert(std::min_element(heap_.begin() + 1, heap_.end(), less_) == heap_.begin() + 1);
  }

  void Decrease(const T& x) {
    Remove(x);
    Insert(x);
  }

  void Insert(const T& x) {
    assert(!Contains(x));
    const int i = heap_.size();
    heap_.push_back(x);
    index_[x] = i;
    SiftUp(i);
    assert(std::min_element(heap_.begin() + 1, heap_.end(), less_) == heap_.begin() + 1);
  }

  void Remove(const T& x) {
    assert(Contains(x));
    const int i = index_[x];
    heap_[i] = heap_.back();
    index_[heap_[i]] = i;
    heap_.pop_back();
    index_[x] = 0;
    if (heap_.size() > i) {
      SiftDown(i);
      SiftUp(i);
    }
    assert(!Contains(x));
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

#endif  // LIMBO_DENSE_H_

