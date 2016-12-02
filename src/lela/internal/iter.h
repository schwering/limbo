// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A couple of iterators to immitate Haskell lists with iterators.
//
// Maybe boost provides the same iterators and we should move to boost (this set
// of iterators evolved somewhat).

#ifndef LELA_INTERNAL_ITER_H_
#define LELA_INTERNAL_ITER_H_

#include <iterator>
#include <set>
#include <type_traits>
#include <utility>

#include <lela/internal/traits.h>

namespace lela {
namespace internal {

// Wrapper for operator*() and operator++(int).
template<typename InputIt>
struct iterator_proxy {
  typedef typename InputIt::value_type value_type;
  typedef typename InputIt::reference reference;
  typedef typename InputIt::pointer pointer;

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
  typedef std::bidirectional_iterator_tag iterator_category;
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
template<typename ContainerIt>
struct flatten_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef typename ContainerIt::value_type container_type;
  typedef decltype(std::declval<container_type>().begin()) iterator_type;
  typedef typename iterator_type::value_type value_type;
  typedef typename iterator_type::pointer pointer;
  typedef typename iterator_type::reference reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<flatten_iterator> proxy;

  flatten_iterator() = default;
  explicit flatten_iterator(ContainerIt cont_first, ContainerIt cont_last)
      : cont_first_(cont_first),
        cont_last_(cont_last) {
    if (cont_first_ != cont_last_) {
      iter_ = (*cont_first_).begin();
    }
    Skip();
  }

  bool operator==(flatten_iterator it) const {
    return cont_first_ == it.cont_first_ && (cont_first_ == cont_last_ || iter_ == it.iter_);
  }
  bool operator!=(flatten_iterator it) const { return !(*this == it); }

  reference operator*() const {
    assert(cont_first_ != cont_last_);
    assert((*cont_first_).begin() != (*cont_first_).end());
    return *iter_;
  }

  flatten_iterator& operator++() {
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

  ContainerIt cont_first_;
  const ContainerIt cont_last_;
  iterator_type iter_;
};

template<typename ContainerIt>
struct flatten_range {
  typedef flatten_iterator<ContainerIt> iterator;

  flatten_range(ContainerIt begin, ContainerIt end) : begin_(begin, end), end_(end, end) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename ContainerIt>
inline flatten_range<ContainerIt> nest_range(ContainerIt begin, ContainerIt end) {
  return flatten_range<ContainerIt>(begin, end);
}

// Haskell's map function.
template<typename InputIt, typename UnaryFunction>
struct transform_iterator {
  typedef typename InputIt::difference_type difference_type;
  typedef typename first_type<std::result_of<UnaryFunction(typename InputIt::value_type)>,
                              std::result_of<UnaryFunction(         InputIt            )>>::type value_type;
  typedef const value_type* pointer;
  typedef value_type reference;
  typedef typename InputIt::iterator_category iterator_category;
  typedef iterator_proxy<transform_iterator> proxy;

  transform_iterator() = default;
  explicit transform_iterator(InputIt iter, UnaryFunction func = UnaryFunction()) : iter_(iter), func_(func) {}

  bool operator==(transform_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(transform_iterator it) const { return !(*this == it); }

  template<typename It = InputIt>
  typename std::result_of<UnaryFunction(typename It::value_type)>::type operator*() const { return func_(*iter_); }
  template<typename It = InputIt>
  typename std::result_of<UnaryFunction(         It            )>::type operator*() const { return func_(iter_); }

  transform_iterator& operator++() { ++iter_; return *this; }
  transform_iterator& operator--() { --iter_; return *this; }

  proxy operator->() const { return proxy(operator*()); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }
  proxy operator--(int) { proxy p(operator*()); operator--(); return p; }

  transform_iterator& operator+=(difference_type n) { iter_ += n; }
  transform_iterator& operator-=(difference_type n) { iter_ -= n; }
  friend transform_iterator operator+(transform_iterator it, difference_type n) { it += n; return it; }
  friend transform_iterator operator+(difference_type n, transform_iterator it) { it += n; return it; }
  friend transform_iterator operator-(transform_iterator it, difference_type n) { it -= n; return it; }
  friend difference_type operator-(transform_iterator a, transform_iterator b) { return a.iter_ - b.iter_; }
  reference operator[](difference_type n) const { return *(*this + n); }
  bool operator<(transform_iterator it) const { return it - *this < 0; }
  bool operator>(transform_iterator it) const { return *this < it; }
  bool operator<=(transform_iterator it) const { return !(*this > it); }
  bool operator>=(transform_iterator it) const { return !(*this < it); }

 private:
  InputIt iter_;
  UnaryFunction func_;
};

template<typename InputIt, typename UnaryFunction>
struct transformed_range {
  typedef transform_iterator<InputIt, UnaryFunction> iterator;

  transformed_range(InputIt begin, InputIt end, UnaryFunction func = UnaryFunction())
      : begin_(begin, func), end_(end, func) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename InputIt, typename UnaryFunction>
inline transformed_range<InputIt, UnaryFunction> transform_range(InputIt begin,
                                                                 InputIt end,
                                                                 UnaryFunction func = UnaryFunction()) {
  return transformed_range<InputIt, UnaryFunction>(begin, end, func);
}

// Haskell's filter function.
template<typename InputIt, typename UnaryPredicate>
struct filter_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef typename InputIt::value_type value_type;
  typedef typename InputIt::pointer pointer;
  typedef typename InputIt::reference reference;
  typedef typename std::conditional<
      std::is_convertible<typename InputIt::iterator_category, std::forward_iterator_tag>::value,
      std::forward_iterator_tag, std::input_iterator_tag>::type iterator_category;
  typedef iterator_proxy<filter_iterator> proxy;

  filter_iterator() = default;
  filter_iterator(InputIt it, const InputIt end, UnaryPredicate pred = UnaryPredicate())
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

  InputIt iter_;
  const InputIt end_;
  UnaryPredicate pred_;
};

template<typename InputIt, typename UnaryPredicate>
struct filtered_range {
  typedef filter_iterator<InputIt, UnaryPredicate> iterator;

  filtered_range(InputIt begin, InputIt end, UnaryPredicate pred = UnaryPredicate())
      : begin_(begin, end, pred), end_(end, end, pred) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename InputIt, typename UnaryPredicate>
inline filtered_range<InputIt, UnaryPredicate> filter_range(InputIt begin,
                                                            InputIt end,
                                                            UnaryPredicate pred = UnaryPredicate()) {
  return filtered_range<InputIt, UnaryPredicate>(begin, end, pred);
}

template<typename T>
struct unique_filter {
  bool operator()(const T& x) const {
    auto it = seen_.upper_bound(x);
    if (it == seen_.end()) {
      seen_.insert(it, x);
      return true;
    } else {
      return false;
    }
  }

 private:
  mutable std::set<T> seen_;
};

template<typename InputIt1, typename InputIt2>
struct joined_ranges {
  struct iterator {
    typedef std::ptrdiff_t difference_type;
    typedef typename InputIt1::value_type value_type;
    typedef typename InputIt1::pointer pointer;
    typedef typename InputIt1::reference reference;
    typedef typename std::conditional<
        std::is_convertible<typename InputIt1::iterator_category, std::forward_iterator_tag>::value &&
        std::is_convertible<typename InputIt2::iterator_category, std::forward_iterator_tag>::value,
        std::forward_iterator_tag, std::input_iterator_tag>::type iterator_category;
    typedef iterator_proxy<InputIt1> proxy;

    iterator() = default;
    iterator(InputIt1 it1, InputIt1 end1, InputIt2 it2) : it1_(it1), end1_(end1), it2_(it2) {}

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
    InputIt1 it1_;
    InputIt1 end1_;
    InputIt2 it2_;
  };

  joined_ranges(InputIt1 begin1, InputIt1 end1, InputIt2 begin2, InputIt2 end2)
      : begin1_(begin1), end1_(end1), begin2_(begin2), end2_(end2) {}

  iterator begin() const { return iterator(begin1_, end1_, begin2_); }
  iterator end()   const { return iterator(end1_, end1_, end2_); }

 private:
  InputIt1 begin1_;
  InputIt1 end1_;
  InputIt2 begin2_;
  InputIt2 end2_;
};

template<typename InputIt1, typename InputIt2>
inline joined_ranges<InputIt1, InputIt2> join_ranges(InputIt1 begin1, InputIt1 end1, InputIt2 begin2, InputIt2 end2) {
  return joined_ranges<InputIt1, InputIt2>(begin1, end1, begin2, end2);
}

}  // namespace internal
}  // namespace lela

#endif  // LELA_INTERNAL_ITER_H_

