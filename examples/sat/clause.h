// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_CLAUSE_H_
#define LIMBO_CLAUSE_H_

#include <cassert>
#include <cstring>
#include <cstdlib>

#include <functional>
#include <memory>
#include <vector>

#include <limbo/literal.h>

#include <limbo/internal/ints.h>

namespace limbo {

class Clause {
 public:
  using iterator = Literal*;
  using const_iterator = const Literal*;
  class Factory;

  template<bool guarantee_invalid = false>
  static int Normalize(int size, Literal* as) {
    int i1 = 0;
    int i2 = 0;
    while (i2 < size) {
      assert(i1 <= i2);
      if (!guarantee_invalid && as[i2].valid()) {
        as[0] = Literal::Eq(as[i2].rhs(), as[i2].rhs());
        return -1;
      }
      if (as[i2].unsatisfiable()) {
        ++i2;
        goto next;
      }
      for (int j = 0; j < i1; ++j) {
        if (!guarantee_invalid && Literal::Valid(as[i2], as[j])) {
          as[0] = Literal::Eq(as[i2].rhs(), as[i2].rhs());
          return -1;
        }
        if (as[i2].Subsumes(as[j])) {
          ++i2;
          goto next;
        }
      }
      for (int j = i2 + 1; j < size; ++j) {
        if (as[i2].ProperlySubsumes(as[j])) {
          ++i2;
          goto next;
        }
      }
      as[i1++] = as[i2++];
next: {}
    }
    return i1;
  }

  static Clause* New(Literal a) { return new (alloc(1)) Clause(a); }
  static Clause* New(const std::vector<Literal>& as) { return New(as.size(), as.data()); }
  static Clause* New(int size, const Literal* as) { return new (alloc(size)) Clause(size, as, false); }
  static Clause* NewNormalized(const std::vector<Literal>& as) { return NewNormalized(as.size(), as.data()); }
  static Clause* NewNormalized(int size, const Literal* as) { return new (alloc(size)) Clause(size, as, true); }
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

  template<typename UnaryPredicate>
  int RemoveIf(UnaryPredicate p) {
    int i1 = 0;
    int i2 = 0;
    while (i2 < h_.size) {
      if (p(as_[i2])) {
        ++i2;
      } else {
        as_[i1++] = as_[i2++];
      }
    }
    h_.size = i1;
    assert(Normalized());
    return i2 - i1;
  }

 private:
  static void* alloc(int k) { return std::malloc(sizeof(Clause) + sizeof(Literal) * k); }

  Clause() = default;

  explicit Clause(const Literal a) {
    if (!a.unsatisfiable()) {
      h_.size = 1;
      as_[0] = a.valid() ? Literal::Eq(a.rhs(), a.rhs()) : a;
    } else {
      h_.size = 0;
    }
    assert(Normalized());
  }

  Clause(int size, const Literal* first, bool guaranteed_normalized) {
    h_.size = size;
    std::memcpy(begin(), first, size * sizeof(Literal));
    if (!guaranteed_normalized) {
      h_.size = Normalize(h_.size, as_);
    }
    assert(Normalized());
  }

  Clause(const Clause&) = delete;
  Clause& operator=(const Clause& c) = delete;
  Clause(Clause&&) = default;
  Clause& operator=(Clause&& c) = default;

#ifndef NDEBUG
  bool Normalized() {
    for (int i = 0; i < h_.size; ++i) {
      if (as_[i].valid() && (h_.size != 1 || as_[i].lhs() != as_[i].rhs())) {
        return false;
      }
      if (as_[i].unsatisfiable()) {
        return false;
      }
      for (int j = 0; j < h_.size; ++j) {
        if (i == j) {
          continue;
        }
        if (Literal::Valid(as_[i], as_[j])) {
          return false;
        }
        if (as_[i].Subsumes(as_[j])) {
          return false;
        }
      }
    }
    return true;
  }
#endif

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

