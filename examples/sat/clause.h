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

  static constexpr bool kGuaranteeInvalid = true;
  static constexpr bool kGuaranteeNormalized = true;

  template<bool guarantee_invalid = !kGuaranteeInvalid>
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

  bool valid()         const { return unit() && as_[0].pos() && as_[0].lhs() == as_[0].rhs(); }
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
  Clause(const Literal a, bool guaranteed_normalized) {
    if (!guaranteed_normalized && a.unsatisfiable()) {
      h_.size = 0;
    } else {
      h_.size = 1;
      as_[0] = a.valid() ? Literal::Eq(a.rhs(), a.rhs()) : a;
    }
    assert(Normalized());
  }

  Clause(int size, const Literal* first, bool guaranteed_normalized) {
    h_.size = size;
    std::memcpy(begin(), first, size * sizeof(Literal));
    if (!guaranteed_normalized) {
      size = Normalize(h_.size, as_);
      h_.size = size >= 0 ? size : 1;
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

class Clause::Factory {
 public:
  using cref_t = unsigned int;

  Factory() = default;
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;
  Factory(Factory&&) = default;
  Factory& operator=(Factory&&) = default;

  template<bool guaranteed_normalized = !kGuaranteeNormalized>
  cref_t New(Literal a) {
    const cref_t cr = memory_.Allocate(clause_size(1));
    new (memory_.address(cr)) Clause(a, guaranteed_normalized);
    return cr;
  }

  template<bool guaranteed_normalized = !kGuaranteeNormalized>
  cref_t New(int k, const Literal* as) {
    const cref_t cr = memory_.Allocate(clause_size(k));
    new (memory_.address(cr)) Clause(k, as, guaranteed_normalized);
    return cr;
  }

  template<bool guaranteed_normalized = !kGuaranteeNormalized>
  cref_t New(const std::vector<Literal>& as) {
    return New<guaranteed_normalized>(as.size(), as.data());
  }

  void Delete(cref_t cr, int k) { memory_.Free(cr, k); }

  Clause& operator[](cref_t r) { return reinterpret_cast<Clause&>(memory_[r]); }
  const Clause& operator[](cref_t r) const { return reinterpret_cast<const Clause&>(memory_[r]); }

 private:
  template<typename T>
  class MemoryPool {
   public:
    using size_t = unsigned int;
    using ref_t = unsigned int;

    explicit MemoryPool(size_t n = 1024 * 1024) { Capacitate(n); }
    ~MemoryPool() { if (memory_) { std::free(memory_); } }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = default;
    MemoryPool& operator=(MemoryPool&&) = default;

    size_t bytes_to_chunks(size_t n) { return (n + sizeof(T) - 1) / sizeof(T); }

    ref_t Allocate(size_t n) {
      const ref_t r = size_;
      size_ += n;
      Capacitate(size_);
      return r;
    }

    void Free(cref_t r, int k) {
      if (r + k == size_) {
        size_ = r;
      }
    }

    T& operator[](ref_t r) { return memory_[r]; }
    const T& operator[](ref_t r) const { return memory_[r]; }

    T* address(ref_t r) const { return &memory_[r]; }
    ref_t reference(const T* r) const { return r - &memory_[0]; }

   private:
    void Capacitate(size_t n) {
      if (n > capacity_) {
        while (n > capacity_) {
          capacity_ += ((capacity_ / 2) + (capacity_ / 8) + 2) & ~1;
        }
        memory_ = static_cast<T*>(std::realloc(memory_, capacity_ * sizeof(T)));
      }
    }

    T* memory_ = nullptr;
    size_t capacity_ = 0;
    size_t size_ = 1;
  };

  using Pool = MemoryPool<unsigned int>;

  Pool::size_t clause_size(Pool::size_t size) {
    return memory_.bytes_to_chunks(sizeof(Clause) + size * sizeof(Literal));
  }

  Pool memory_;
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

