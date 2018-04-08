// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_CLAUSE_H_
#define LIMBO_CLAUSE_H_

#include <cassert>
#include <cstring>
#include <cstdlib>

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_set>
#include <utility>

#include <limbo/literal.h>

#include <limbo/internal/bloom.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>
#include <limbo/internal/traits.h>

namespace limbo {

class Clause {
 public:
  using iterator = Literal*;
  using const_iterator = const Literal*;
  class Factory;

  static Clause* New(Literal a) { return new Clause(a); }
  static Clause* New(const std::vector<Literal>& as) { return New(as.size(), as.data()); }
  static Clause* New(int size, const Literal* first) {
    void* ptr = std::malloc(sizeof(Clause) + sizeof(Literal) * size);
    return new (ptr) Clause(size, first);
  }
  static void Delete(Clause* c) { free(c); }

  bool operator==(const Clause& c) const {
    if (size() != c.size()) {
      return false;
    }
    for (Literal a : *this) {
      for (Literal b : c) {
        if (a == b) {
          goto next;
        }
      }
next: {}
    }
    return true;
  }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  internal::hash32_t hash() const {
    internal::hash32_t h = 0;
    for (Literal a : *this) {
      h ^= a.hash();
    }
    return h;
  }

  bool empty()  const { return h_.size == 0; }
  bool unit()   const { return h_.size == 1; }
  int size() const { return h_.size; }

  Literal first()  const { return as_[0]; }
  Literal second() const { return as_[1]; }
  Literal last()   const { return as_[h_.size - 1]; }

  Literal& operator[](int i)       { assert(i >= 0 && i < size()); return as_[i]; }
  Literal  operator[](int i) const { assert(i >= 0 && i < size()); return as_[i]; }

  iterator begin() { return &as_[0]; }
  iterator end()   { return &as_[0] + h_.size; }

  const_iterator cbegin() const { return &as_[0]; }
  const_iterator cend()   const { return &as_[0] + h_.size; }

  const_iterator begin()  const { return cbegin(); }
  const_iterator end()    const { return cend(); }

  bool ground()      const { return std::all_of(begin(), end(), [](const Literal a) { return a.ground(); }); }
  bool primitive()   const { return std::all_of(begin(), end(), [](const Literal a) { return a.primitive(); }); }
  bool well_formed() const { return std::all_of(begin(), end(), [](const Literal a) { return a.well_formed(); }); }

  bool valid()         const { return unit() && first().pos() && first().lhs() == first().rhs(); }
  bool unsatisfiable() const { return empty(); }

  bool Subsumes(const Clause& c) const {
    for (Literal a : *this) {
      for (Literal b : c) {
        if (a.Subsumes(b)) {
          goto next;
        }
      }
      return false;
next: {}
    }
    return true;
  }

  template<typename UnaryFunction>
  Clause* Substitute(UnaryFunction theta, Term::Factory* tf) const {
    auto r = internal::transform_crange(*this, [theta, tf](Literal a) { return a.Substitute(theta, tf); });
    return New(size(), r.begin(), r.end());
  }

 private:
  Clause() = default;

  explicit Clause(const Literal a) {
    if (!a.unsatisfiable()) {
      h_.size = 1;
      as_[0] = a.valid() ? Literal::Eq(a.rhs(), a.rhs()) : a;
    } else {
      h_.size = 0;
    }
  }

  Clause(int size, const Literal* first) {
    h_.size = size;
    std::memcpy(begin(), first, size * sizeof(Literal));
    Normalize();
  }

  Clause(const Clause&) = delete;
  Clause& operator=(const Clause& c) = delete;
  Clause(Clause&&) = default;
  Clause& operator=(Clause&& c) = default;

  void Normalize() {
    int i1 = 0;
    int i2 = 0;
    while (i2 < h_.size) {
      assert(i1 <= i2);
      if (as_[i2].valid()) {
        h_.size = 1;
        as_[0] = Literal::Eq(as_[i2].rhs(), as_[i2].rhs());
        return;
      }
      if (as_[i2].unsatisfiable()) {
        ++i2;
        goto next;
      }
      for (int j = 0; j < i1; ++j) {
        if (Literal::Valid(as_[i2], as_[j])) {
          h_.size = 1;
          as_[0] = Literal::Eq(as_[i2].rhs(), as_[i2].rhs());
          return;
        }
        if (as_[i2].Subsumes(as_[j])) {
          ++i2;
          goto next;
        }
      }
      for (int j = i2 + 1; j < h_.size; ++j) {
        if (as_[i2].ProperlySubsumes(as_[j])) {
          ++i2;
          goto next;
        }
      }
      as_[i1++] = as_[i2++];
next:
      {}
    }
    h_.size = i1;
  }

  struct {
    unsigned learnt :  1;
    unsigned size   : 31;
  } h_;
  Literal as_[0];
};

}  // namespace limbo


namespace std {

template<>
struct hash<limbo::Clause> {
  limbo::internal::hash32_t operator()(const limbo::Clause& a) const { return a.hash(); }
};

template<>
struct equal_to<limbo::Clause> {
  bool operator()(const limbo::Clause& a, const limbo::Clause& b) const { return a == b; }
};

}  // namespace std

#endif  // LIMBO_CLAUSE_H_

