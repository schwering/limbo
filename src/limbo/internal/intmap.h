// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A map from positive integers to some other type. The key integers are assumed
// to be small and sparse, as their size corresponds to the size of the
// underlying array. Unset values are implicitly set to a null value, which by
// default is T(), which amounts to 0 for integers and false for bools; the
// value can be changed by set_null_value().

#ifndef LIMBO_INTERNAL_INTMAP_H_
#define LIMBO_INTERNAL_INTMAP_H_

#include <algorithm>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <limbo/internal/iter.h>
#include <limbo/internal/ints.h>

namespace limbo {
namespace internal {

template<typename Key, typename T>
class IntMap {
 public:
  typedef std::vector<T> Vec;
  typedef typename Vec::value_type value_type;
  typedef typename Vec::reference reference;
  typedef typename Vec::const_reference const_reference;

  void set_null_value(const T& null) { null_ = null; }

  bool operator==(const IntMap& a) const { return null_ == a.null_ && vec_ == a.vec_; }
  bool operator!=(const IntMap& a) const { return !(*this == a); }

  reference operator[](Key key) {
    const typename Vec::size_type key_int = typename Vec::size_type(key);
    if (key_int >= n_keys()) {
      vec_.resize(key_int + 1, null_);
    }
    return vec_[key_int];
  }

  typename Vec::const_reference operator[](Key key) const {
    return const_cast<IntMap&>(*this)[key];
  }

  size_t n_keys() const { return vec_.size(); }

  struct Keys {
    struct Cast { Key operator()(size_t key) const { return Key(key); } };
    typedef internal::int_iterator<size_t> int_iterator;
    typedef internal::transform_iterator<int_iterator, Cast> iterator;
    explicit Keys(const IntMap* owner) : owner(owner) {}
    iterator begin() const { return iterator(int_iterator(0)); }
    iterator end()   const { return iterator(int_iterator(owner->n_keys())); }
   private:
    const IntMap* owner;
  };

  Keys keys() const { return Keys(this); }

  typename Vec::iterator begin() { return vec_.begin(); }
  typename Vec::iterator end()   { return vec_.end(); }

  typename Vec::const_iterator begin() const { return vec_.begin(); }
  typename Vec::const_iterator end()   const { return vec_.end(); }

  struct ConstValues {
    typedef typename Vec::const_iterator iterator;
    explicit ConstValues(const IntMap* owner) : owner(owner) {}
    iterator begin() const { return owner->begin(); }
    iterator end()   const { return owner->end(); }
   private:
    const IntMap* owner;
  };

  ConstValues values() const { return ConstValues(this); }

  template<typename BinaryFunction>
  static IntMap Zip(const IntMap& m1, const IntMap& m2, BinaryFunction f) {
    IntMap m;
    size_t s = std::max(m1.n_keys(), m2.n_keys());
    for (size_t i = 0; i < s; ++i) {
      m[i] = f(m1[i], m2[i]);
    }
    return m;
  }

  template<typename BinaryFunction>
  void Zip(const IntMap& m, BinaryFunction f) {
    size_t s = std::max(n_keys(), m.n_keys());
    for (size_t i = 0; i < s; ++i) {
      Key k = Key(i);
      (*this)[k] = f((*this)[k], m[k]);
    }
  }

 protected:
  T null_ = T();
  Vec vec_;
};

template<typename Key, typename T>
class IntMultiMap {
 public:
  typedef std::unordered_set<T> Bucket;
  typedef IntMap<Key, Bucket> Base;
  typedef T value_type;

  bool operator==(const IntMultiMap& a) const { return map_ == a.map_; }
  bool operator!=(const IntMultiMap& a) const { return !(*this == a); }

        Bucket& operator[](Key key)       { return map_[key]; }
  const Bucket& operator[](Key key) const { return map_[key]; }

  size_t insert(Key key, const T& val) {
    auto p = map_[key].insert(val);
    size_t n = p.second ? 1 : 0;
    size_ += n;
    return n;
  }

  void insert(const IntMultiMap& m) {
    for (Key key : m.keys()) {
      size_ -= (*this)[key].size();
      (*this)[key].insert(m.map_[key].begin(), m.map_[key].end());
      size_ += (*this)[key].size();
    }
  }

  size_t erase(Key key, const T& val) {
    size_t n = map_[key].erase(val);
    size_ -= n;
    return n;
  }

  bool contains(Key key, const T& val) const {
    const Bucket& s = map_[key];
    return s.find(val) != s.end();
  }

  size_t n_keys() const { return map_.n_keys(); }
  size_t n_values(Key key) const { return map_[key].size(); }

  typedef typename Base::Keys Keys;

  Keys keys() const { return Keys(&map_); }

  typedef typename Bucket::const_iterator value_iterator;

  value_iterator begin(Key key) const { return map_[key].begin(); }
  value_iterator end(Key key) const { return map_[key].end(); }

  struct ValuesForKey {
    explicit ValuesForKey(const IntMultiMap* owner, Key key) : owner(owner), key(key) {}
    value_iterator begin() const { return owner->begin(key); }
    value_iterator end()   const { return owner->end(key); }
   private:
    const IntMultiMap* owner;
    Key key;
  };

  ValuesForKey values(Key key) const { return ValuesForKey(this, key); }

  typedef flatten_iterator<typename Base::Vec::const_iterator> all_values_iterator;

  all_values_iterator begin() const { return all_values_iterator(map_.begin(), map_.end()); }
  all_values_iterator end()   const { return all_values_iterator(map_.end(), map_.end()); }

  struct Values {
    explicit Values(const IntMultiMap* owner) : owner(owner) {}
    all_values_iterator begin() const { return owner->begin(); }
    all_values_iterator end()   const { return owner->end(); }
   private:
    const IntMultiMap* owner;
  };

  Values values() const { return Values(this); }

  bool all_empty() const { return size_ == 0; }
  size_t total_size() const { return size_; }

 private:
  Base map_;
  size_t size_ = 0;
};

template<typename T, typename UnaryFunction, typename Key = typename std::result_of<UnaryFunction(T)>::type>
class IntMultiSet {
 public:
  typedef IntMultiMap<Key, T> Parent;
  typedef typename Parent::Base Base;
  typedef typename Parent::Bucket Bucket;
  typedef T value_type;

  explicit IntMultiSet(UnaryFunction key = UnaryFunction()) : key_(key) {}

  bool operator==(const IntMultiSet& a) const { return map_ == a.map_; }
  bool operator!=(const IntMultiSet& a) const { return !(*this == a); }

        Bucket& operator[](Key key)       { return map_[key]; }
  const Bucket& operator[](Key key) const { return map_[key]; }

  size_t insert(const T& val) { return map_.insert(key_(val), val); }

  void insert(const IntMultiSet& m) { map_.insert(m.map_); }

  template<typename Collection>
  size_t insert(const Collection& vals) { map_.insert(vals); }

  void erase(const T& val) { map_.erase(key_(val), val); }

  bool contains(const T& val) const { return map_.contains(key_(val), val); }

  size_t n_keys() const { return map_.n_keys(); }
  size_t n_values(Key key) const { return map_[key].size(); }

  typedef typename Parent::Keys Keys;
  typedef typename Parent::value_iterator value_iterator;
  typedef typename Parent::ValuesForKey ValuesForKey;
  typedef typename Parent::all_values_iterator all_values_iterator;
  typedef typename Parent::Values Values;

  Keys keys() const { return map_.keys(); }
  value_iterator begin(Key key) const { return map_[key].begin(); }
  value_iterator end(Key key) const { return map_[key].end(); }
  ValuesForKey values(Key key) const { return map_.values(key); }
  ValuesForKey values(const T& val) const { return values(key_(val)); }
  Values values() const { return map_.values(); }

  all_values_iterator begin() const { return values().begin(); }
  all_values_iterator end()   const { return values().end(); }

  bool all_empty() const { return map_.all_empty(); }
  size_t total_size() const { return map_.total_size(); }

 private:
  UnaryFunction key_;
  Parent map_;
};

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_INTMAP_H_

