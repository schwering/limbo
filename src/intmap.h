// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A map from positive integers to some other type. The key integers are assumed
// to be small and sparse, as their size corresponds to the size of the
// underlying array.

#ifndef SRC_INTMAP_H_
#define SRC_INTMAP_H_

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include "./iter.h"

namespace lela {

template<typename Key, typename T, T NULL_VALUE = T()>
class IntMap : public std::vector<T> {
 public:
  typedef std::vector<T> parent;

  template<typename Iter>
  struct gen_iterator {
   public:
    typedef Iter iterator;
    typedef std::ptrdiff_t difference_type;
    typedef std::pair<const Key, typename Iter::value_type> value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::input_iterator_tag iterator_category;

    gen_iterator() {}
    explicit gen_iterator(const IntMap<Key, T>& owner, Iter it) : owner(owner), iter_(it) {
      Iter first = owner.parent::begin();
      index_ = static_cast<Key>(std::distance(first, it));
    }

    bool operator==(gen_iterator it) const { return index_ == it.index_ && iter_ == it.iter_; }
    bool operator!=(gen_iterator it) const { return !(*this == it); }

    value_type operator*() const { return std::make_pair(index_, *iter_); }

    gen_iterator& operator++() { ++index_; ++iter_; return *this; }

   private:
    const IntMap<Key, T>& owner;
    Iter iter_;
    Key index_;
  };

  typedef gen_iterator<typename parent::iterator> iterator;
  typedef gen_iterator<typename parent::const_iterator> const_iterator;

  using std::vector<T>::vector;

  typename parent::reference operator[](Key pos) {
    typename parent::size_type pos_int = static_cast<typename parent::size_type>(pos);
    if (pos_int >= parent::size()) {
      parent::resize(pos_int + 1, null_);
    }
    return parent::operator[](pos_int);
  }

  typename parent::const_reference operator[](Key pos) const {
    typename parent::size_type pos_int = static_cast<typename parent::size_type>(pos);
    return pos_int < parent::size() ? parent::operator[](pos_int) : null_;
  }

  iterator begin() { typename iterator::iterator it = parent::begin(); return iterator(*this, it); }
  iterator end()   { typename iterator::iterator it = parent::end(); return iterator(*this, it); }

  const_iterator cbegin() const { typename const_iterator::iterator it = parent::cbegin(); return const_iterator(*this, it); }
  const_iterator cend()   const { typename const_iterator::iterator it = parent::cend(); return const_iterator(*this, it); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end()   const { return cend(); }

  template<typename BinaryFunction>
  static IntMap Zip(const IntMap& m1, const IntMap& m2, BinaryFunction f) {
    IntMap m;
    size_t s = std::max(m1.size(), m2.size());
    for (size_t i = 0; i < s; ++i) {
      m[i] = f(m1[i], m2[i]);
    }
    return m;
  }

  template<typename BinaryFunction>
  void Zip(const IntMap& m, BinaryFunction f) {
    size_t s = std::max(parent::size(), m.size());
    for (size_t i = 0; i < s; ++i) {
      (*this)[i] = f((*this)[i], m[i]);
    }
  }

 private:
  T null_ = NULL_VALUE;
};

}  // namespace lela

#endif  // SRC_INTMAP_H_

