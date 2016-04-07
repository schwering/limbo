// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A couple of iterators to immitate Haskell lists with iterators.
//
// Maybe boost provides the same iterators and we should move to boost (this set
// of iterators evolved somewhat).

#ifndef SRC_ITER_H_
#define SRC_ITER_H_

#include <iterator>
#include <type_traits>
#include "./clause.h"
#include "./intmap.h"

namespace lela {

// is basically a lazy list whose end is determined while it grows.
template<typename ConstantFunction>
struct incr_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<ConstantFunction()>::type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  incr_iterator() {}
  explicit incr_iterator(ConstantFunction offset) : offset_(offset), index_(0) {}

  bool operator==(incr_iterator it) const { return *(*this) == *it; }
  bool operator!=(incr_iterator it) const { return !(*this == it); }

  value_type operator*() const { assert(index_ >= 0); return offset_() + index_; }

  incr_iterator& operator++() { ++index_; return *this; }

 private:
  ConstantFunction offset_;
  value_type index_;
};

// Expects an iterator pointing to containers and iterates over their elements.
template<typename ContIter>
struct nested_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename ContIter::value_type::value_type iterator_type;
  typedef typename iterator_type::value_type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  nested_iterator() {}
  explicit nested_iterator(ContIter cont_first, ContIter cont_last)
      : cont_first_(cont_first),
        cont_last_(cont_last) {
    if (cont_first_ != cont_last_) {
      iter_ = (*cont_first_).begin();
    }
    Skip();
  }

  bool operator==(nested_iterator it) const { return cont_first_ == it.cont_first_ && (cont_first_ == cont_last_ || iter_ == it.iter_); }
  bool operator!=(nested_iterator it) const { return !(*this == it); }

  value_type operator*() const {
    assert(cont_first_ != cont_last_);
    assert((*cont_first_).begin() != (*cont_first_).end());
    return *iter_;
  }

  nested_iterator& operator++() {
    assert(cont_first_ != cont_last_ && iter_ != (*cont_first_).end());
    ++iter_;
    Skip();
    return *this;
  }

 private:
  void Skip() {
    for (;;) {
      if (cont_first_ == cont_last_) {
        break; // iterator has ended
      }
      if (iter_ != (*cont_first_).end()) {
        break; // found next element
      }
      ++cont_first_;
      if (cont_first_ != cont_last_) {
        iter_ = (*cont_first_).begin();
      }
    }
    assert(cont_first_ == cont_last_ || iter_ != (*cont_first_).end());
  }

  ContIter cont_first_;
  const ContIter cont_last_;
  iterator_type iter_;
};

// Haskell's map function.
template<typename UnaryFunction, typename Iter>
struct transform_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<UnaryFunction(typename Iter::value_type)>::type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  transform_iterator() {}
  transform_iterator(UnaryFunction func, Iter iter) : func_(func), iter_(iter) {}

  bool operator==(transform_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(transform_iterator it) const { return !(*this == it); }

  value_type operator*() const { return func_(*iter_); }

  transform_iterator& operator++() { ++iter_; return *this; }

 private:
  UnaryFunction func_;
  Iter iter_;
};

// Haskell's filter function.
template<typename UnaryPredicate, typename Iter>
struct filter_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename Iter::value_type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  filter_iterator() {}
  filter_iterator(UnaryPredicate pred, Iter it, const Iter end) : pred_(pred), iter_(it), end_(end) { Skip(); }

  bool operator==(filter_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(filter_iterator it) const { return !(*this == it); }

  value_type operator*() const { return *iter_; }

  filter_iterator& operator++() { ++iter_; Skip(); return *this; }

 private:
  void Skip() {
    while (iter_ != end_ && !pred_(*iter_)) {
      ++iter_;
    }
  }

  UnaryPredicate pred_;
  mutable Iter iter_;
  const Iter end_;
};

}  // namespace lela

#endif  // SRC_ITER_H_

