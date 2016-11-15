// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A couple of iterators to immitate Haskell lists with iterators.
//
// Maybe boost provides the same iterators and we should move to boost (this set
// of iterators evolved somewhat).

#ifndef LELA_INTERNAL_ITER_H_
#define LELA_INTERNAL_ITER_H_

#include <iterator>
#include <type_traits>
#include <utility>

namespace lela {
namespace internal {

// Wrapper for operator*() and operator++(int).
template<typename Iter>
struct iterator_proxy {
  typedef typename Iter::value_type value_type;
  typedef typename Iter::reference reference;
  typedef typename Iter::pointer pointer;

  explicit iterator_proxy(reference v) : v_(v) {}
  reference operator*() const { return v_; }
  pointer operator->() const { return &v_; }

 private:
  mutable value_type v_;
};

struct Identity {
  template<typename T>
  constexpr auto operator()(T&& x) const noexcept -> decltype(std::forward<T>(x)) {
    return std::forward<T>(x);
  }
};

// Encapsulates an integer type.
template<typename T, typename UnaryFunction = Identity>
struct int_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef const value_type* pointer;
  typedef value_type reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<int_iterator> proxy;

  int_iterator() = default;
  explicit int_iterator(value_type index, UnaryFunction func = UnaryFunction()) : index_(index), func_(func) {}

  bool operator==(int_iterator it) const { return *(*this) == *it; }
  bool operator!=(int_iterator it) const { return !(*this == it); }

  reference operator*() const { return func_(index_); }
  int_iterator& operator++() { ++index_; return *this; }
  int_iterator& operator--() { --index_; return *this; }

  proxy operator->() const { return proxy(operator*()); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }
  proxy operator--(int) { proxy p(operator*()); operator--(); return p; }

 private:
  value_type index_;
  UnaryFunction func_;
};

// Expects an iterator pointing to containers and iterates over their elements.
template<typename ContIter>
struct nested_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef typename ContIter::value_type container_type;
  typedef decltype(std::declval<container_type>().begin()) iterator_type;
  typedef typename iterator_type::value_type value_type;
  typedef typename iterator_type::pointer pointer;
  typedef typename iterator_type::reference reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<nested_iterator> proxy;

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

  pointer operator->() const { return iter_.operator->(); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

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
template<typename Iter, typename UnaryFunction>
struct transform_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef typename std::result_of<UnaryFunction(typename Iter::value_type)>::type value_type;
  typedef const value_type* pointer;
  typedef value_type reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<transform_iterator> proxy;

  transform_iterator() = default;
  explicit transform_iterator(Iter iter, UnaryFunction func = UnaryFunction()) : iter_(iter), func_(func) {}

  bool operator==(transform_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(transform_iterator it) const { return !(*this == it); }

  reference operator*() const { return func_(*iter_); }
  transform_iterator& operator++() { ++iter_; return *this; }

  proxy operator->() const { return proxy(operator*()); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

 private:
  Iter iter_;
  UnaryFunction func_;
};

template<typename Iter, typename UnaryFunction>
struct transformed_range {
  typedef transform_iterator<Iter, UnaryFunction> iterator;

  transformed_range(Iter begin, Iter end, UnaryFunction func = UnaryFunction())
      : begin_(begin, func), end_(end, func) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename Iter, typename UnaryFunction>
inline transformed_range<Iter, UnaryFunction> transform_range(Iter begin,
                                                              Iter end,
                                                              UnaryFunction func = UnaryFunction()) {
  return transformed_range<Iter, UnaryFunction>(begin, end, func);
}

// Haskell's filter function.
template<typename Iter, typename UnaryPredicate>
struct filter_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef typename Iter::value_type value_type;
  typedef typename Iter::pointer pointer;
  typedef typename Iter::reference reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<filter_iterator> proxy;

  filter_iterator() = default;
  filter_iterator(Iter it, const Iter end, UnaryPredicate pred = UnaryPredicate())
      : iter_(it), end_(end), pred_(pred) { Skip(); }

  bool operator==(filter_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(filter_iterator it) const { return !(*this == it); }

  reference operator*() const { return *iter_; }
  filter_iterator& operator++() { ++iter_; Skip(); return *this; }

  pointer operator->() const { return iter_.operator->(); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

 private:
  void Skip() {
    while (iter_ != end_ && !pred_(*iter_)) {
      ++iter_;
    }
  }

  Iter iter_;
  const Iter end_;
  UnaryPredicate pred_;
};

template<typename Iter, typename UnaryPredicate>
struct filtered_range {
  typedef filter_iterator<Iter, UnaryPredicate> iterator;

  filtered_range(Iter begin, Iter end, UnaryPredicate pred = UnaryPredicate())
      : begin_(begin, end, pred), end_(end, end, pred) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename Iter, typename UnaryPredicate>
inline filtered_range<Iter, UnaryPredicate> filter_range(Iter begin, Iter end, UnaryPredicate pred = UnaryPredicate()) {
  return filtered_range<Iter, UnaryPredicate>(begin, end, pred);
}

template<typename Iter1, typename Iter2>
struct joined_ranges {
  struct iterator {
    typedef std::ptrdiff_t difference_type;
    typedef typename Iter1::value_type value_type;
    typedef typename Iter1::pointer pointer;
    typedef typename Iter1::reference reference;
    typedef std::input_iterator_tag iterator_category;
    typedef iterator_proxy<Iter1> proxy;

    iterator() = default;
    iterator(Iter1 it1, Iter1 end1, Iter2 it2) : it1_(it1), end1_(end1), it2_(it2) {}

    bool operator==(const iterator& it) const { return it1_ == it.it1_ && it2_ == it.it2_; }
    bool operator!=(const iterator& it) const { return !(*this == it); }

    reference operator*() const { return it1_ != end1_ ? *it1_ : *it2_; }
    iterator& operator++() {
      if (it1_ != end1_)  ++it1_;
      else                ++it2_;
      return *this;
    }

    pointer operator->() const { return it1_ != end1_ ? it1_.operator->() : it2_.operator->(); }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

   private:
    Iter1 it1_;
    Iter1 end1_;
    Iter2 it2_;
  };

  joined_ranges(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2)
      : begin1_(begin1), end1_(end1), begin2_(begin2), end2_(end2) {}

  iterator begin() const { return iterator(begin1_, end1_, begin2_); }
  iterator end()   const { return iterator(end1_, end1_, end2_); }

 private:
  Iter1 begin1_;
  Iter1 end1_;
  Iter2 begin2_;
  Iter2 end2_;
};

template<typename Iter1, typename Iter2>
inline joined_ranges<Iter1, Iter2> join_ranges(Iter1 begin1, Iter1 end1, Iter2 begin2, Iter2 end2) {
  return joined_ranges<Iter1, Iter2>(begin1, end1, begin2, end2);
}

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_ITER_H_

