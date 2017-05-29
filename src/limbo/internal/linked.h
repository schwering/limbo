// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Linked containers.

#ifndef LIMBO_INTERNAL_LINKED_H_
#define LIMBO_INTERNAL_LINKED_H_

#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>

namespace limbo {
namespace internal {

template<typename T>
class Linked {
public:
  class iterator {
   public:
    typedef std::ptrdiff_t difference_type;
    typedef T value_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;
    typedef std::forward_iterator_tag iterator_category;
    typedef iterator_proxy<iterator> proxy;

    iterator() = default;
    explicit iterator(const Linked* linked) : linked_(linked) {}

    bool operator==(iterator it) const { return linked_ == it.linked_; }
    bool operator!=(iterator it) const { return !(*this == it); }

    reference operator*() const { return linked_->container_; }
    iterator& operator++() { linked_ = linked_->parent_; return *this; }

    proxy operator->() const { return proxy(operator*()); }
    proxy operator++(int) { proxy p(operator*()); operator++(); return p; }

   private:
    const Linked* linked_ = nullptr;
  };
 
  explicit Linked(Linked* parent = nullptr, const T& c = T()) : parent_(parent), container_(c) {}
  explicit Linked(Linked* parent, T&& c) : parent_(parent), container_(c) {}

  T& container() { return container_; }
  const T& container() const { return container_; }

  iterator begin() const { return iterator(this); }
  iterator end() const { return iterator(); }

  template<typename UnaryFunction>
  transform_iterators<iterator, UnaryFunction> transform(UnaryFunction func) const {
    return transform_crange(*this, func);
  }

  template<typename UnaryFunction>
  flatten_iterators<typename transform_iterators<iterator, UnaryFunction>::iterator>
  transform_flatten(UnaryFunction func) const {
    return flatten_crange(transform_crange(*this, func));
  }

  template<typename UnaryFunction,
           typename BinaryFunction,
           typename U = decltype(std::declval<UnaryFunction>()(std::declval<T>()))>
  U Fold(UnaryFunction map, BinaryFunction reduce) const {
    U x = map(container_);
    return parent_ ? reduce(parent_->Fold(map, reduce), x) : x;
  }

 private:
  Linked* parent_ = nullptr;
  T container_;
};

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_LINKED_H_

