// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_ITER_H_
#define SRC_ITER_H_

#include <iterator>
#include <type_traits>
#include "./clause.h"
#include "./intmap.h"
#include "./range.h"

namespace lela {

template<typename T>
class DecrementingIterator {
 public:
  typedef T value_type;

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
  typedef typename std::result_of<RangeFunction(const Level*)>::type::iterator_type iterator_type;
  typedef typename std::result_of<RangeFunction(const Level*)>::type::iterator_type::value_type value_type;

  LevelIterator() : current_(0) {}
  LevelIterator(NextLevelFunction nlf, RangeFunction rf, const Level* level)
      : nlf_(nlf), rf_(rf), current_(level) {
    operator++();
  }

  bool operator==(LevelIterator it) const { return current_ == it.current_ && range_ == it.range_; }
  bool operator!=(LevelIterator it) const { return !(*this == it); }

  value_type operator*() const { return *range_.first; }

  LevelIterator& operator++() {
    if (!range_.empty()) {
      ++range_.first;
    } else {
      while (current_ && (range_ = rf_(current_)).empty()) {
        current_ = nlf_();
      }
    }
    return *this;
  }

 private:
  NextLevelFunction nlf_;
  RangeFunction rf_;
  const Level* current_;
  Range<iterator_type> range_;
};

template<typename UnaryPredicate, typename Iter>
class FilterIterator {
 public:
  typedef typename Iter::value_type value_type;

  explicit FilterIterator(UnaryPredicate pred, Iter it) : pred_(pred), iter_(it) {}

  bool operator==(FilterIterator it) const { return iter_ == it.iter_; }
  bool operator!=(FilterIterator it) const { return !(*this == it); }

  value_type operator*() const { Skip(); return *iter_; }

  FilterIterator& operator++() {
    ++iter_;
    return *this;
  }

 private:
  void Skip() const {
    while (!pred_(*iter_)) {
      assert(*iter_ >= 0);
      ++iter_;
    }
  }

  UnaryPredicate pred_;
  mutable Iter iter_;
};

template<typename UnaryFunction, typename Iter>
class TransformIterator {
 public:
  typedef typename std::result_of<UnaryFunction(typename Iter::value_type)>::type value_type;

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
  FilterIterator<UnaryPredicate, Iter> filter_iterator(UnaryPredicate pred, Iter it) {
  return FilterIterator<UnaryPredicate, Iter>(pred, it);
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

