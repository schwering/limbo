// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A map from positive integers to some other type. The key integers are assumed
// to be small and sparse, as their size corresponds to the size of the
// underlying array.

#ifndef SRC_INTMAP_H_
#define SRC_INTMAP_H_

#include <vector>

namespace lela {

template<typename T, T DEFAULT_VALUE = T()>
class IntMap : public std::vector<T> {
 public:
  typedef typename std::vector<T> parent;

  using parent::vector;

  typename parent::reference operator[](typename parent::size_type pos) {
    if (pos >= parent::size()) {
      parent::resize(pos + 1, DEFAULT_VALUE);
    }
    return parent::operator[](pos);
  }

  bool operator[](typename parent::size_type pos) const {
    return pos < parent::size() ? parent::operator[](pos) : DEFAULT_VALUE;
  }
};

}  // namespace lela

#endif  // SRC_INTMAP_H_

