// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A couple of iterators to immitate Haskell lists with iterators.
//
// Maybe boost provides the same iterators and we should move to boost (this set
// of iterators evolved somewhat).

#ifndef LIMBO_INTERNAL_ITER_H_
#define LIMBO_INTERNAL_ITER_H_

#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <limbo/internal/ints.h>
#include <limbo/internal/maybe.h>
#include <limbo/internal/traits.h>

namespace limbo {
namespace internal {

// Wrapper for operator*() and operator++(int).
template<typename InputIt>
class iterator_proxy {
 public:
  typedef typename InputIt::value_type value_type;
  typedef typename InputIt::reference reference;
  typedef typename InputIt::pointer pointer;

  explicit iterator_proxy(reference v) : v_(v) {}
  reference operator*() const { return v_; }
  pointer operator->() const { return &v_; }

 private:
  mutable typename std::remove_const<value_type>::type v_;
};

struct Identity {
  template<typename T>
  constexpr auto operator()(T&& x) const noexcept -> decltype(std::forward<T>(x)) { return std::forward<T>(x); }
};

// Encapsulates an integer type.
template<typename T, typename UnaryFunction = Identity>
class int_iterator {
 public:
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

template<typename T, typename UnaryFunction = Identity>
class int_iterators {
 public:
  typedef int_iterator<T, UnaryFunction> iterator;

  explicit int_iterators(T begin, T end, UnaryFunction func1 = UnaryFunction(), UnaryFunction func2 = UnaryFunction()) :
      begin_(begin, func1), end_(end, func2) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename T, typename UnaryFunction = Identity>
inline int_iterators<T, UnaryFunction> int_range(T begin,
                                                 T end,
                                                 UnaryFunction func1 = UnaryFunction(),
                                                 UnaryFunction func2 = UnaryFunction()) {
  return int_iterators<T, UnaryFunction>(begin, end, func1, func2);
}

template<typename T, typename U>
class array_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef U value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::random_access_iterator_tag iterator_category;
  typedef internal::iterator_proxy<array_iterator> proxy;

  array_iterator() = default;
  explicit array_iterator(T* array, size_t i) : array_(array), index_(i) {}

  bool operator==(array_iterator it) const { assert(array_ == it.array_); return index_ == it.index_; }
  bool operator!=(array_iterator it) const { return !(*this == it); }

  reference operator*() const { return (*array_)[index_]; }
  array_iterator& operator++() { ++index_; return *this; }
  array_iterator& operator--() { --index_; return *this; }

  proxy operator->() const { return proxy(operator*()); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }
  proxy operator--(int) { proxy p(operator*()); operator--(); return p; }

  array_iterator& operator+=(difference_type n) { index_ += n; return *this; }
  array_iterator& operator-=(difference_type n) { index_ -= n; return *this; }
  friend array_iterator operator+(array_iterator it, difference_type n) { it += n; return it; }
  friend array_iterator operator+(difference_type n, array_iterator it) { it += n; return it; }
  friend array_iterator operator-(array_iterator it, difference_type n) { it -= n; return it; }
  friend difference_type operator-(array_iterator a, array_iterator b) { return a.index_ - b.index_; }
  reference operator[](difference_type n) const { return *(*this + n); }
  bool operator<(array_iterator it) const { return index_ < it.index_; }
  bool operator>(array_iterator it) const { return index_ > it.index_; }
  bool operator<=(array_iterator it) const { return !(*this > it); }
  bool operator>=(array_iterator it) const { return !(*this < it); }

 private:
  T* array_;
  size_t index_;
};

// Expects an iterator over containers, and iterates over the containers' elements.
template<typename OuterInputIt,
         typename InnerInputIt = decltype(std::declval<typename OuterInputIt::value_type const>().begin()),
         InnerInputIt (OuterInputIt::value_type::*begin)() const = &OuterInputIt::value_type::begin,
         InnerInputIt (OuterInputIt::value_type::*end)() const = &OuterInputIt::value_type::end>
class flatten_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename InnerInputIt::value_type value_type;
  typedef typename InnerInputIt::pointer pointer;
  typedef typename InnerInputIt::reference reference;
  typedef std::input_iterator_tag iterator_category;
  typedef iterator_proxy<flatten_iterator> proxy;

  flatten_iterator() = default;
  explicit flatten_iterator(OuterInputIt cont_first, OuterInputIt cont_last) :
      cont_first_(cont_first),
        cont_last_(cont_last) {
    if (cont_first_ != cont_last_) {
      iter_ = ((*cont_first_).*begin)();
    }
    Skip();
  }

  bool operator==(flatten_iterator it) const {
    return cont_first_ == it.cont_first_ && (cont_first_ == cont_last_ || iter_ == it.iter_);
  }
  bool operator!=(flatten_iterator it) const { return !(*this == it); }

  reference operator*() const {
    assert(cont_first_ != cont_last_);
    assert(((*cont_first_).*begin)() != ((*cont_first_).*end)());
    return *iter_;
  }

  flatten_iterator& operator++() {
    assert(cont_first_ != cont_last_ && iter_ != ((*cont_first_).*end)());
    ++iter_;
    Skip();
    return *this;
  }

  pointer operator->() const { return iter_.operator->(); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

 private:
  static_assert(std::is_convertible<typename OuterInputIt::iterator_category, std::input_iterator_tag>::value,
                "OuterInputIt has wrong iterator category");
  static_assert(std::is_convertible<typename InnerInputIt::iterator_category, std::input_iterator_tag>::value,
                "InnerInputIt has wrong iterator category");

  void Skip() {
    for (;;) {
      if (cont_first_ == cont_last_) {
        break;  // iterator has ended
      }
      if (iter_ != ((*cont_first_).*end)()) {
        break;  // found next element
      }
      ++cont_first_;
      if (cont_first_ != cont_last_) {
        iter_ = ((*cont_first_).*begin)();
      }
    }
    assert(cont_first_ == cont_last_ || iter_ != ((*cont_first_).*end)());
  }

  OuterInputIt cont_first_;
  OuterInputIt cont_last_;
  InnerInputIt iter_;
};

// Haskell's map function.
template<typename InputIt, typename UnaryFunction>
class transform_iterator {
 public:
  typedef typename InputIt::difference_type difference_type;
  typedef typename std::remove_reference<
      typename first_type<std::result_of<UnaryFunction(typename InputIt::value_type)>,
                          std::result_of<UnaryFunction(         InputIt            )>>::type
    >::type value_type;
  typedef const typename std::remove_reference<value_type>::type* pointer;
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

  transform_iterator& operator+=(difference_type n) { iter_ += n; return *this; }
  transform_iterator& operator-=(difference_type n) { iter_ -= n; return *this; }
  friend transform_iterator operator+(transform_iterator it, difference_type n) { it += n; return it; }
  friend transform_iterator operator+(difference_type n, transform_iterator it) { it += n; return it; }
  friend transform_iterator operator-(transform_iterator it, difference_type n) { it -= n; return it; }
  friend difference_type operator-(transform_iterator a, transform_iterator b) { return a.iter_ - b.iter_; }
  reference operator[](difference_type n) const { return *(*this + n); }
  bool operator<(transform_iterator it) const { return *this - it < 0; }
  bool operator>(transform_iterator it) const { return *this < it; }
  bool operator<=(transform_iterator it) const { return !(*this > it); }
  bool operator>=(transform_iterator it) const { return !(*this < it); }

 private:
  static_assert(std::is_convertible<typename InputIt::iterator_category, std::input_iterator_tag>::value,
                "InputIt has wrong iterator category");

  InputIt iter_;
  UnaryFunction func_;
};

template<typename InputIt, typename UnaryFunction>
class transform_iterators {
 public:
  typedef transform_iterator<InputIt, UnaryFunction> iterator;

  transform_iterators(InputIt begin, InputIt end, UnaryFunction func = UnaryFunction()) :
      begin_(begin, func), end_(end, func) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  static_assert(std::is_convertible<typename InputIt::iterator_category, std::input_iterator_tag>::value,
                "InputIt has wrong iterator category");

  iterator begin_;
  iterator end_;
};

template<typename InputIt, typename UnaryFunction>
inline transform_iterators<InputIt, UnaryFunction> transform_range(InputIt begin,
                                                                   InputIt end,
                                                                   UnaryFunction func) {
  return transform_iterators<InputIt, UnaryFunction>(begin, end, func);
}

template<typename Range, typename InputIt, typename UnaryFunction>
inline transform_iterators<typename Range::iterator, UnaryFunction>
transform_range(Range r, UnaryFunction func) {
  return transform_range(r.begin(), r.end(), func);
}

// Haskell's filter function.
template<typename InputIt, typename UnaryPredicate>
class filter_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename InputIt::value_type value_type;
  typedef typename InputIt::pointer pointer;
  typedef typename InputIt::reference reference;
  typedef typename std::conditional<
      std::is_convertible<typename InputIt::iterator_category, std::forward_iterator_tag>::value,
      std::forward_iterator_tag, std::input_iterator_tag>::type iterator_category;
  typedef iterator_proxy<filter_iterator> proxy;

  filter_iterator() = default;
  filter_iterator(InputIt it, const InputIt end, UnaryPredicate pred = UnaryPredicate()) :
      iter_(it), end_(end), pred_(pred) { Skip(); }

  bool operator==(filter_iterator it) const { return iter_ == it.iter_; }
  bool operator!=(filter_iterator it) const { return !(*this == it); }

  reference operator*() const { return *iter_; }
  filter_iterator& operator++() { ++iter_; Skip(); return *this; }

  pointer operator->() const { return iter_.operator->(); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

 private:
  static_assert(std::is_convertible<typename InputIt::iterator_category, std::input_iterator_tag>::value,
                "InputIt has wrong iterator category");

  void Skip() {
    while (iter_ != end_ && !pred_(*iter_)) {
      ++iter_;
    }
  }

  InputIt iter_;
  InputIt end_;
  UnaryPredicate pred_;
};

template<typename InputIt, typename UnaryPredicate>
class filter_iterators {
 public:
  typedef filter_iterator<InputIt, UnaryPredicate> iterator;

  filter_iterators(InputIt begin, InputIt end, UnaryPredicate pred = UnaryPredicate()) :
      begin_(begin, end, pred), end_(end, end, pred) {}

  iterator begin() const { return begin_; }
  iterator end()   const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template<typename InputIt, typename UnaryPredicate>
inline filter_iterators<InputIt, UnaryPredicate> filter_range(InputIt begin,
                                                              InputIt end,
                                                              UnaryPredicate pred) {
  return filter_iterators<InputIt, UnaryPredicate>(begin, end, pred);
}

template<typename Range, typename UnaryPredicate>
inline filter_iterators<typename Range::iterator, UnaryPredicate>
filter_range(Range r, UnaryPredicate pred) {
  return filter_range(r.begin(), r.end(), pred);
}

template<typename DomainType, typename CodomainInputIt>
class mapping_iterator {
 public:
  typedef DomainType domain_type;
  typedef CodomainInputIt codomain_iterator;
  typedef typename CodomainInputIt::value_type codomain_type;

  struct value_type {
    value_type(const mapping_iterator* owner) : owner(owner) {}

    internal::Maybe<codomain_type> operator()(domain_type x) const {
      auto it = owner->dcd_.find(x);
      if (it != owner->dcd_.end()) {
        auto& cd = it->second;
        assert(cd.current != cd.end);
        const codomain_type y = *cd.current;
        return internal::Just(y);
      } else {
        return internal::Nothing;
      }
    }

    bool operator==(const value_type& a) const { return *owner == *a->owner; }
    bool operator!=(const value_type& a) const { return !(*this == a); }

   private:
    const mapping_iterator* owner;
  };

  typedef std::ptrdiff_t difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef typename std::conditional<
      std::is_convertible<typename codomain_iterator::iterator_category, std::forward_iterator_tag>::value,
      std::forward_iterator_tag, std::input_iterator_tag>::type iterator_category;
  typedef iterator_proxy<mapping_iterator> proxy;

  mapping_iterator() {}

  template<typename InputIt>
  explicit mapping_iterator(InputIt begin, InputIt end) {
    for (; begin != end; ++begin) {
      domain_type x = begin->first;
      codomain_iterator y1 = begin->second.begin();
      codomain_iterator y2 = begin->second.end();
      dcd_.insert(std::make_pair(x, CodomainState(y1, y2)));
    }
    iter_ = Just(dcd_.end());
  }

  // These iterators are really heavy-weight, especially comparison is
  // unusually expensive. To boost the usual comparison with end(), we
  // hence reset the iter_ Maybe to Nothing once the end is reached.
  bool operator==(const mapping_iterator& it) const { return iter_ == it.iter_ && (!iter_ || dcd_ == it.dcd_); }
  bool operator!=(const mapping_iterator& it) const { return !(*this == it); }

  value_type operator*() const { return value_type(this); }

  mapping_iterator& operator++() {
    for (iter_ = Just(dcd_.begin()); iter_.val != dcd_.end(); ++iter_.val) {
      CodomainState& cd = iter_.val->second;
      assert(cd.current != cd.end);
      ++cd.current;
      if (cd.current != cd.end) {
        break;
      } else {
        cd.current = cd.begin;
        assert(cd.current != cd.end);
      }
    }
    assert(iter_);
    if (iter_.val == dcd_.end()) {
      iter_ = Nothing;
      assert(*this == mapping_iterator());
    }
    return *this;
  }

  pointer operator->() const { return iter_.operator->(); }
  proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

 private:
  friend struct value_type;

  struct CodomainState {
    CodomainState(codomain_iterator begin, codomain_iterator end) : begin(begin), current(begin), end(end) {}

    bool operator==(const CodomainState& a) const { return begin == a.begin && current == a.current && end == a.end; }
    bool operator!=(const CodomainState& a) const { return !(*this == a); }

    codomain_iterator begin;
    codomain_iterator current;
    codomain_iterator end;
  };
  typedef std::unordered_map<domain_type, CodomainState> DomainCodomainState;

  DomainCodomainState dcd_;
  Maybe<typename DomainCodomainState::iterator> iter_;
};

template<typename InputIt1, typename InputIt2>
class joined_iterator {
 public:
  typedef std::ptrdiff_t difference_type;
  typedef typename InputIt1::value_type value_type;
  typedef typename InputIt1::pointer pointer;
  typedef typename InputIt1::reference reference;
  typedef typename std::conditional<
      std::is_convertible<typename InputIt1::iterator_category, std::forward_iterator_tag>::value &&
      std::is_convertible<typename InputIt2::iterator_category, std::forward_iterator_tag>::value,
      std::forward_iterator_tag, std::input_iterator_tag>::type iterator_category;
  typedef iterator_proxy<InputIt1> proxy;

  joined_iterator() = default;
  joined_iterator(InputIt1 it1, InputIt1 end1, InputIt2 it2) : it1_(it1), end1_(end1), it2_(it2) {}

  bool operator==(const joined_iterator& it) const { return it1_ == it.it1_ && it2_ == it.it2_; }
  bool operator!=(const joined_iterator& it) const { return !(*this == it); }

  reference operator*() const { return it1_ != end1_ ? *it1_ : *it2_; }
  joined_iterator& operator++() {
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

template<typename InputIt1, typename InputIt2>
class joined_iterators {
 public:
  typedef joined_iterator<InputIt1, InputIt2> iterator;

  joined_iterators(InputIt1 begin1, InputIt1 end1, InputIt2 begin2, InputIt2 end2) :
      begin1_(begin1), end1_(end1), begin2_(begin2), end2_(end2) {}

  iterator begin() const { return iterator(begin1_, end1_, begin2_); }
  iterator end()   const { return iterator(end1_, end1_, end2_); }

 private:
  static_assert(std::is_convertible<typename InputIt1::iterator_category, std::input_iterator_tag>::value,
                "InputIt1 has wrong iterator category");
  static_assert(std::is_convertible<typename InputIt2::iterator_category, std::input_iterator_tag>::value,
                "InputIt2 has wrong iterator category");

  InputIt1 begin1_;
  InputIt1 end1_;
  InputIt2 begin2_;
  InputIt2 end2_;
};

template<typename InputIt1, typename InputIt2>
inline joined_iterators<InputIt1, InputIt2> join_ranges(InputIt1 begin1,
                                                        InputIt1 end1,
                                                        InputIt2 begin2,
                                                        InputIt2 end2) {
  return joined_iterators<InputIt1, InputIt2>(begin1, end1, begin2, end2);
}

template<typename Range1, typename Range2, typename InputIt1, typename InputIt2>
inline joined_iterators<InputIt1, InputIt2> join_ranges(Range1 r1, Range2 r2) {
  return join_ranges(r1.begin(), r1.end(), r2.begin(), r2.end());
}

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_ITER_H_

