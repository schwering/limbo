// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_INTMAP_H_
#define SRC_INTMAP_H_

#include <vector>

namespace lela {

#include <vector>

template<typename T>
class IntMap : public std::vector<T> {
 public:
  typedef typename std::vector<T> parent;

  using std::vector<T>::vector;

  IntMap(const T& default_elem) : default_elem_(default_elem) {}

  typename parent::reference operator[](typename parent::size_type pos) {
    if (pos >= std::vector<T>::size()) {
      parent::resize(pos + 1, default_elem_);
    }
    return parent::operator[](pos);
  }

  bool operator[](typename parent::size_type pos) const {
    return pos < parent::size() ? parent::operator[](pos) : false;
  }

 private:
  const T default_elem_;
};

}  // namespace lela

#endif  // SRC_INTMAP_H_

