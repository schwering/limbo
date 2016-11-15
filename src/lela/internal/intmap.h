// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A map from positive integers to some other type. The key integers are assumed
// to be small and sparse, as their size corresponds to the size of the
// underlying array. Unset values are implicitly set to a null value, which by
// default is T(), which amounts to 0 for integers and false for bools; the
// value can be changed by set_null_value().

#ifndef LELA_INTERNAL_INTMAP_H_
#define LELA_INTERNAL_INTMAP_H_

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include <lela/internal/iter.h>

namespace lela {
namespace internal {

template<typename Key, typename T>
class IntMap : public std::vector<T> {
 public:
  typedef std::vector<T> parent;

  template<typename Iter>
  struct gen_iterator {
   public:
    typedef Iter iterator;
    typedef std::ptrdiff_t difference_type;
    typedef std::pair<const Key, typename Iter::pointer> value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef std::input_iterator_tag iterator_category;

    gen_iterator() {}
    explicit gen_iterator(Iter begin, Iter offset) : iter_(offset) {
      index_ = static_cast<Key>(std::distance(begin, offset));
    }

    bool operator==(gen_iterator it) const { return index_ == it.index_ && iter_ == it.iter_; }
    bool operator!=(gen_iterator it) const { return !(*this == it); }

    value_type operator*() const { return std::make_pair(index_, &*iter_); }

    gen_iterator& operator++() { ++index_; ++iter_; return *this; }

   private:
    Iter iter_;
    Key index_;
  };

  typedef gen_iterator<typename parent::iterator> iterator;
  typedef gen_iterator<typename parent::const_iterator> const_iterator;

  using std::vector<T>::vector;

  void set_null_value(const T& null) { null_ = null; }

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

  iterator begin() { return iterator(parent::begin(), parent::begin()); }
  iterator end()   { return iterator(parent::end(), parent::end()); }

  const_iterator cbegin() const { return const_iterator(parent::cbegin(), parent::cbegin()); }
  const_iterator cend()   const { return const_iterator(parent::cend(), parent::cend()); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end()   const { return cend(); }

  struct Keys {
    typedef internal::int_iterator<Key> iterator;

    explicit Keys(const IntMap* owner) : owner_(owner) {}

    iterator begin() const { return iterator(0); }
    iterator end()   const { return iterator(static_cast<Key>(owner_->size())); }

   private:
    const IntMap* owner_;
  };

  Keys keys() const { return Keys(this); }

  struct Values {
    explicit Values(IntMap* owner) : owner_(owner) {}

    typename parent::iterator begin() const { return owner_->parent::begin(); }
    typename parent::iterator end() const { return owner_->parent::end(); }

   private:
    IntMap* owner_;
  };

  Values values() { return Values(this); }

  struct ConstValues {
    explicit ConstValues(const IntMap* owner) : owner_(owner) {}

    typename parent::const_iterator begin() const { return owner_->parent::begin(); }
    typename parent::const_iterator end() const { return owner_->parent::end(); }

   private:
    const IntMap* owner_;
  };

  ConstValues cvalues() const { return ConstValues(this); }
  ConstValues values() const { return cvalues(); }

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

 protected:
  T null_ = T();
};

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_INTMAP_H_

