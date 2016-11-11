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

namespace lela {

// Iterates over numbers offset() + 0, offset() + 1, offset() + 2, ...
template<typename NullaryFunction>
struct incr_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<NullaryFunction()>::type value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::input_iterator_tag iterator_category;

  incr_iterator() = default;
  explicit incr_iterator(NullaryFunction offset) : offset_(offset), index_(0) {}

  bool operator==(incr_iterator it) const { return *(*this) == *it; }
  bool operator!=(incr_iterator it) const { return !(*this == it); }

  value_type operator*() const { assert(index_ >= 0); return offset_() + index_; }

  incr_iterator& operator++() { ++index_; return *this; }

 private:
  NullaryFunction offset_;
  value_type index_;
};

// Expects an iterator pointing to containers and iterates over their elements.
template<typename ContIter>
struct nested_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename ContIter::value_type container_type;
  //typedef typename container_type::const_iterator iterator_type;
  //typedef typename std::result_of<decltype(&container_type::begin)(container_type)>::type iterator_type;
  typedef decltype(std::declval<container_type>().begin()) iterator_type;
  typedef typename iterator_type::value_type value_type;
  typedef typename iterator_type::pointer pointer;
  typedef typename iterator_type::reference reference;
  typedef std::input_iterator_tag iterator_category;

  nested_iterator() = default;
  explicit nested_iterator(ContIter cont_first, ContIter cont_last)
      : cont_first_(cont_first),
        cont_last_(cont_last) {
    if (cont_first_ != cont_last_) {
      iter_ = (*cont_first_).begin();
    }
    Skip();
  }

  bool operator==(nested_iterator it) const {
    return cont_first_ == it.cont_first_ && (cont_first_ == cont_last_ || iter_ == it.iter_);
  }
  bool operator!=(nested_iterator it) const { return !(*this == it); }

  reference operator*() const {
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
        break;  // iterator has ended
      }
      if (iter_ != (*cont_first_).end()) {
        break;  // found next element
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

  transform_iterator() = default;
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

  filter_iterator() = default;
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

template<typename Iter1, typename Iter2>
struct joined {
 public:
  struct iter {
    typedef std::ptrdiff_t difference_type;
    typedef typename Iter1::value_type value_type;
    typedef typename Iter1::pointer pointer;
    typedef typename Iter1::reference reference;
    typedef std::input_iterator_tag iterator_category;

    iter() = default;
    iter(Iter1 it1, Iter1 end1, Iter2 it2) : it1_(it1), end1_(end1), it2_(it2) {}

    bool operator==(const iter& it) const { return it1_ == it.it1_ && it2_ == it.it2_; }
    bool operator!=(const iter& it) const { return !(*this == it); }

    reference operator*() const { return it1_ != end1_ ? *it1_ : *it2_; }

    iter& operator++() {
      if (it1_ != end1_)  ++it1_;
      else                ++it2_;
      return *this;
    }

   private:
    Iter1 it1_;
    Iter1 end1_;
    Iter2 it2_;
  };

  joined(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2)
      : begin1_(begin1), end1_(end1), begin2_(begin2), end2_(end2) {}

  iter begin() const { return iter(begin1_, end1_, begin2_); }
  iter end() const { return iter(end1_, end1_, end2_); }

 private:
  Iter1 begin1_;
  Iter1 end1_;
  Iter2 begin2_;
  Iter2 end2_;
};

template<typename Iter1, typename Iter2>
inline joined<Iter1, Iter2> join(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2) {
  return joined<Iter1, Iter2>(begin1, end1, begin2, end2);
}

}  // namespace lela

#endif  // SRC_ITER_H_

