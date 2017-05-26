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
#include <iterator>
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

  reference operator[](Key key) {
    typename Vec::size_type key_int = static_cast<typename Vec::size_type>(key);
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
    typedef internal::int_iterator<Key> iterator;
    explicit Keys(const IntMap* owner) : owner(owner) {}
    iterator begin() const { return iterator(0); }
    iterator end()   const { return iterator(static_cast<Key>(owner->n_keys())); }
   private:
    const IntMap* owner;
  };

  Keys keys() const { return Keys(this); }

  struct Values {
    explicit Values(IntMap* owner) : owner(owner) {}
    typename Vec::iterator begin() const { return owner->vec_.begin(); }
    typename Vec::iterator end() const { return owner->vec_.end(); }
   private:
    IntMap* owner;
  };

  struct ConstValues {
    explicit ConstValues(const IntMap* owner) : owner(owner) {}
    typename Vec::const_iterator begin() const { return owner->vec_.begin(); }
    typename Vec::const_iterator end() const { return owner->vec_.end(); }
   private:
    const IntMap* owner;
  };

  Values values() { return Values(this); }
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
      vec_[i] = f(vec_[i], m[i]);
    }
  }

 protected:
  T null_ = T();
  Vec vec_;
};

template<typename Key, typename T>
class MultiIntMap {
 public:
  typedef IntMap<Key, std::unordered_set<T>> Base;
  typedef T value_type;

  typename Base::reference operator[](Key key) { return map_[key]; }
  typename Base::const_reference operator[](Key key) const { return map_[key]; }

  size_t insert(Key key, const T& val) {
    auto p = map_[key].insert(val);
    return p.second ? 1 : 0;
  }

  void erase(Key key, const T& val) {
    map_[key].erase(val);
  }

  bool contains(Key key, const T& val) const {
    const value_type& s = map_[key];
    return s.find(val) != s.end();
  }

  size_t n_keys() const { return map_.n_keys(); }

  typedef typename Base::Keys Keys;

  Keys keys() const { return Keys(&map_); }

  struct PosValues {
    explicit PosValues(const MultiIntMap* owner, Key key) : owner(owner), key(key) {}
    typename Base::value_type::const_iterator begin() const { return owner->map_[key].begin(); }
    typename Base::value_type::const_iterator end() const { return owner->map_[key].end(); }
   private:
    const MultiIntMap* owner;
    Key key;
  };

  PosValues values(Key key) const { return PosValues(this, key); }

  struct Values {
    typedef flatten_iterator<typename value_type::const_iterator> iterator;
    explicit Values(MultiIntMap* owner) : owner(owner) {}
    iterator begin() const { return owner->map_.values().begin(); }
    iterator end() const { return owner->map_.values().end(); }
   private:
    MultiIntMap* owner;
  };

  Values values() const { return Values(this); }

 private:
  Base map_;
};

template<typename T, typename UnaryFunction, typename Key = typename std::result_of<UnaryFunction(T)>::type>
class MultiIntSet {
 public:
  typedef MultiIntMap<Key, T> Parent;
  typedef typename Parent::Base Base;
  typedef T value_type;

  explicit MultiIntSet(UnaryFunction key = UnaryFunction()) : key_(key) {}

  typename Base::reference operator[](Key key) { return map_[key]; }
  typename Base::const_reference operator[](Key key) const { return map_[key]; }

  size_t insert(const T& val) { return map_.insert(key_(val), val); }

  void erase(const T& val) { map_.erase(key_(val), val); }

  template<typename Collection>
  size_t insert(const Collection& vals) { map_.insert(vals); }

  size_t insert(const MultiIntSet& set) {
    size_t n = 0;
    for (Key key : set.keys()) {
      for (const T& val : set.values(key)) {
        n += insert(val);
      }
    }
    return n;
  }

  bool contains(const T& val) const { return map_.contains(key_(val)); }

  size_t n_keys() const { return map_.n_keys(); }

  typedef typename Parent::Keys Keys;
  typedef typename Parent::PosValues PosValues;
  typedef typename Parent::Values Values;

  Keys keys() const { return map_.keys(); }
  PosValues values(Key key) const { return map_.values(key); }
  PosValues values(const T& val) const { return values(key_(val)); }
  Values values() const { return map_.values(); }

 private:
  UnaryFunction key_;
  Parent map_;
};

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_INTMAP_H_

