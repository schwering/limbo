// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_ITER_H_
#define SRC_ITER_H_

#include <iterator>
#include <type_traits>
#include "./clause.h"
#include "./intmap.h"
#include "./range.h"
#include <iostream>

namespace lela {

template<typename T>
class DecrementingIterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  DecrementingIterator() : index_(-1) {}
  explicit DecrementingIterator(T max_index) : index_(max_index) {}

  bool operator==(DecrementingIterator it) const { return index_ == it.index_; }
  bool operator!=(DecrementingIterator it) const { return !(*this == it); }

  T operator*() const { assert(index_ >= 0); return index_; }

  DecrementingIterator& operator++() { --index_; return *this; }

 private:
  T index_;
};

template<typename Level, typename NextLevelFunction, typename RangeFunction>
class LevelIterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<RangeFunction(const Level*)>::type::iterator_type::value_type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  LevelIterator() : current_(0) {}
  LevelIterator(NextLevelFunction nlf, RangeFunction rf, const Level* level) : nlf_(nlf), rf_(rf), current_(level), range_(rf_(current_)) { Skip(); }

  bool operator==(LevelIterator it) const { return current_ == it.current_ && (!current_ || range_ == it.range_); }
  bool operator!=(LevelIterator it) const { return !(*this == it); }

  value_type operator*() const { assert(!range_.empty()); return *range_.first; }

  LevelIterator& operator++() {
    assert(current_ && !range_.empty());
    ++range_.first;
    Skip();
    return *this;
  }

 private:
  typedef typename std::result_of<RangeFunction(const Level*)>::type::iterator_type iterator_type;

  void Skip() {
    while (range_.empty()) {
      current_ = nlf_(current_);
      if (!current_) {
        break;
      }
      range_ = rf_(current_);
    }
    assert(!current_ || !range_.empty());
  }

  NextLevelFunction nlf_;
  RangeFunction rf_;
  const Level* current_;
  Range<iterator_type> range_;
};

template<typename UnaryPredicate, typename Iter>
class FilterIterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename Iter::value_type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  FilterIterator(UnaryPredicate pred, Iter it, const Iter end = Iter()) : pred_(pred), iter_(it), end_(end) { Skip(); }

  bool operator==(FilterIterator it) const { return iter_ == it.iter_; }
  bool operator!=(FilterIterator it) const { return !(*this == it); }

  value_type operator*() const { return *iter_; }

  FilterIterator& operator++() { ++iter_; Skip(); return *this; }

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

template<typename UnaryFunction, typename Iter>
class TransformIterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<UnaryFunction(typename Iter::value_type)>::type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  TransformIterator() {}
  TransformIterator(UnaryFunction func, Iter iter) : func_(func), iter_(iter) {}

  bool operator==(TransformIterator it) const { return iter_ == it.iter_; }
  bool operator!=(TransformIterator it) const { return !(*this == it); }

  value_type operator*() const { return func_(*iter_); }

  TransformIterator& operator++() { ++iter_; return *this; }

 private:
  UnaryFunction func_;
  Iter iter_;
};

template<typename T>
DecrementingIterator<T> decrementing_iterator(T i) {
  return DecrementingIterator<T>(i);
}

template<typename UnaryPredicate, typename Iter>
  FilterIterator<UnaryPredicate, Iter> filter_iterator(UnaryPredicate pred, Iter it, const Iter end = Iter()) {
  return FilterIterator<UnaryPredicate, Iter>(pred, it, end);
}

template<typename UnaryFunction, typename Iter>
TransformIterator<UnaryFunction, Iter> transform_iterator(UnaryFunction func, Iter iter) {
  return TransformIterator<UnaryFunction, Iter>(func, iter);
}

template<typename Level, typename NextLevelFunction, typename RangeFunction>
LevelIterator<Level, NextLevelFunction, RangeFunction> level_iterator(NextLevelFunction nlf, RangeFunction rf, const Level* level) {
  return LevelIterator<Level, NextLevelFunction, RangeFunction>(nlf, rf, level);
}

template<typename Level, typename NextLevelFunction, typename RangeFunction>
LevelIterator<Level, NextLevelFunction, RangeFunction> level_iterator_end() {
  return LevelIterator<Level, NextLevelFunction, RangeFunction>();
}

}  // namespace lela

#endif  // SRC_ITER_H_

