// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A clause is a set of literals. Clauses are immutable.
//
// A clause is stored as vector, which is initially sorted to remove duplicates.
// Thus, and since clauses are immutable, they represent sets of literals. Note
// that copying and comparing clauses is more expensive than for literals.
//
// Clauses are always normalized, that is, no literal subsumes another literal.
// An unsatisfiable clause is always empty, and a clause with a valid literal is
// a unit clause.
//
// Perhaps the most important operations are PropagateUnit[s]() and Subsumes(),
// which are only defined for primitive clauses and literals. Thus all involved
// literals mention a primitive term on the left-hand side. By definition of
// Complementary() and Subsumes() in the Literal class, a literal can react with
// another only if they refer to the same term. By hashing these terms and
// storing these values in Bloom filters, we can (hopefully often) detect early
// that unit propagation or subsumption won't work early (in a sound but
// incomplete way).

#ifndef LIMBO_CLAUSE_H_
#define LIMBO_CLAUSE_H_

#include <cassert>
#include <cstring>

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
  typedef internal::size_t size_t;
  typedef internal::array_iterator<const Clause, const Literal> const_iterator;

  enum Result { kUnchanged, kPropagated, kSubsumed };

  Clause() = default;

  explicit Clause(const Literal a) : size_(!a.unsatisfiable() ? 1 : 0) {
    lits1_[0] = a;
#ifdef BLOOM
    InitBloom();
#endif
  }

  Clause(std::initializer_list<Literal> lits) : Clause(lits.size(), lits.begin(), lits.end()) {}

  template<typename ForwardIt>
  Clause(ForwardIt begin, ForwardIt end) : Clause(std::distance(begin, end), begin, end) {}

  template<typename InputIt>
  Clause(size_t size, InputIt begin, InputIt end) : Clause(size) {
    auto it = begin;
    for (size_t i = 0; it != end && i < kArraySize; ) {
      lits1_[i++] = *it++;
    }
    for (size_t i = 0; it != end; ) {
      lits2_[i++] = *it++;
    }
    Normalize();
#ifdef BLOOM
    InitBloom();
#endif
  }

  Clause(const Clause& c) : Clause(c.size_) {
    std::memcpy(lits1_, c.lits1_, size1() * sizeof(Literal));
    if (size2() > 0) {
      std::memcpy(lits2_.get(), c.lits2_.get(), size2() * sizeof(Literal));
    }
#ifdef BLOOM
    lhs_bloom_ = c.lhs_bloom_;
#endif
    assert(!any([](Literal a) { return a.unsatisfiable(); }));
    assert(*this == c);
  }

  Clause& operator=(const Clause& c) {
    const size_t old_size = c.size_;
    size_ = c.size_;
    std::memcpy(lits1_, c.lits1_, size1() * sizeof(Literal));
    if (size_ > kArraySize) {
      if (size_ > old_size) {
        lits2_ = std::unique_ptr<Literal[]>(new Literal[size2()]);
      }
      std::memcpy(lits2_.get(), c.lits2_.get(), size2() * sizeof(Literal));
    }
#ifdef BLOOM
    lhs_bloom_ = c.lhs_bloom_;
#endif
    assert(!any([](Literal a) { return a.unsatisfiable(); }));
    return *this;
  }

  Clause(Clause&&) = default;
  Clause& operator=(Clause&&) = default;

  bool operator==(const Clause& c) const {
    return size() == c.size() &&
#ifdef BLOOM
           lhs_bloom_ == c.lhs_bloom_ &&
#endif
           std::memcmp(lits1_, c.lits1_, size1() * sizeof(Literal)) == 0 &&
           (size2() == 0 || std::memcmp(lits2_.get(), c.lits2_.get(), size2() * sizeof(Literal)) == 0);
  }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  internal::hash32_t hash() const {
    internal::hash32_t h = 0;
    for (size_t i = 0; i < size(); ++i) {
      h ^= (*this)[i].hash();
    }
    return h;
  }

  const_iterator cbegin() const { return const_iterator(this, 0); }
  const_iterator cend()   const { return const_iterator(this, size()); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end()   const { return cend(); }

  const Literal& operator[](size_t i) const {
    assert(i <= size());
    return i < kArraySize ? lits1_[i] : lits2_[i - kArraySize];
  }

  Literal first() const { return lits1_[0]; }
  Literal last()  const { return (*this)[size() - 1]; }

  bool   empty() const { return size() == 0; }
  bool   unit()  const { return size() == 1; }
  size_t size()  const { return size_; }

  bool valid() const {
    if (unit() && first().valid()) {
      return true;
    }
    const Clause& c = *this;
    for (size_t i = 0; i < size(); ++i) {
      for (size_t j = i + 1; j < size() && c[i].lhs() == c[j].lhs(); ++j) {
        if (Literal::Valid(c[i], c[j])) {
          return true;
        }
      }
    }
    return false;
  }

  bool unsatisfiable() const { return empty(); }

  static bool Subsumes(const Literal a, const Clause c) {
    assert(a.primitive());
    assert(c.primitive());
#ifdef BLOOM
    if (!c.lhs_bloom_.PossiblyContains(a.lhs())) {
      return false;
    }
#endif
    size_t i = 0;
    for (; i < c.size() && a.lhs() > c[i].lhs(); ++i) {
    }
    for (; i < c.size() && a.lhs() == c[i].lhs(); ++i) {
      if (a.Subsumes(c[i])) {
        return true;
      }
    }
    return false;
  }

  static bool Subsumes(const Literal a, const Literal b, const Clause c) {
    assert(a < b);
    assert(c.primitive());
#ifdef BLOOM
    if (!c.lhs_bloom_.PossiblyContains(a.lhs()) || !c.lhs_bloom_.PossiblyContains(b.lhs())) {
      return false;
    }
#endif
    size_t i = 0;
    for (; i < c.size() && a.lhs() > c[i].lhs(); ++i) {
    }
    for (; i < c.size() && a.lhs() == c[i].lhs(); ++i) {
      if (a.Subsumes(c[i])) {
        goto next;
      }
    }
    return false;
next:
    for (; i < c.size() && b.lhs() > c[i].lhs(); ++i) {
    }
    for (; i < c.size() && b.lhs() == c[i].lhs(); ++i) {
      if (b.Subsumes(c[i])) {
        return true;
      }
    }
    return false;
  }

  static bool Subsumes(const Clause& c, const Clause& d) {
    assert(c.primitive());
    assert(d.primitive());
#ifdef BLOOM
    if (!c.lhs_bloom_.PossiblySubsetOf(d.lhs_bloom_)) {
      return false;
    }
#endif
    size_t i = 0;
    size_t j = 0;
    for (; i < c.size(); ++i) {
      for (; j < d.size() && c[i].lhs() > d[j].lhs(); ++j) {
      }
      for (size_t k = j; k < d.size() && c[i].lhs() == d[k].lhs(); ++k) {
        if (c[i].Subsumes(d[k])) {
          goto next;
        }
      }
      assert(!c.all([&d](const Literal a) { return d.any([a](const Literal b) { return a.Subsumes(b); }); }));
      return false;
next:
      {}
    }
    assert(c.all([&d](const Literal a) { return d.any([a](const Literal b) { return a.Subsumes(b); }); }));
    return true;
  }

  bool Subsumes(const Clause& c) const { return Subsumes(*this, c); }

  Result PropagateUnit(const Literal a) {
    assert(primitive());
    assert(a.primitive());
    assert(!valid());
    assert(!a.valid() && !a.unsatisfiable());
    size_t n_nulls = 0;
#ifdef BLOOM
    if (!lhs_bloom_.PossiblyContains(a.lhs())) {
      return kUnchanged;
    }
#endif
    for (size_t i = 0; i < size(); ++i) {
      const Literal b = (*this)[i];
      if (a.Subsumes(b)) {
        if (n_nulls > 0) {
          RemoveNulls();
        }
        return kSubsumed;
      } else if (Literal::Complementary(a, b)) {
        Nullify(i);
        ++n_nulls;
      }
    }
    if (n_nulls > 0) {
      RemoveNulls();
    }
    return n_nulls > 0 ? kPropagated : kUnchanged;
  }

  template<typename InputIt>
  Result PropagateUnits(InputIt first, InputIt last) {
    assert(primitive());
    assert(!valid());
    assert(std::all_of(first, last, [](const Literal a) { return a.primitive(); }));
    assert(std::all_of(first, last, [](const Literal a) { return !a.valid() && !a.unsatisfiable(); }));
    size_t n_nulls = 0;
    for (size_t i = 0; i < size(); ++i) {
      const Literal b = (*this)[i];
      for (; first != last && b.lhs() > first->lhs(); ++first) {}
      bool complementary = false;
      for (auto jt = first; jt != last && b.lhs() == jt->lhs(); ++jt) {
        const Literal a = *jt;
        if (a.Subsumes(b)) {
          if (n_nulls > 0) {
            RemoveNulls();
          }
          return kSubsumed;
        } else if (Literal::Complementary(a, b)) {
          complementary = true;
        }
      }
      if (complementary) {
        Nullify(i);
        ++n_nulls;
      }
    }
    if (n_nulls > 0) {
      RemoveNulls();
    }
    return n_nulls > 0 ? kPropagated : kUnchanged;
  }

  Result PropagateUnits(const std::unordered_set<Literal, Literal::LhsHash>& units) {
    assert(primitive());
    assert(!valid());
    assert(std::all_of(units.begin(), units.end(), [](Literal a) { return a.primitive(); }));
    assert(std::all_of(units.begin(), units.end(), [](Literal a) { return !a.valid() && !a.unsatisfiable(); }));
    size_t n_nulls = 0;
    for (size_t i = 0; i < size(); ++i) {
      const Literal b = (*this)[i];
      if (units.bucket_count() > 0) {
        auto bucket = units.bucket(b);
        bool complementary = false;
        for (auto it = units.begin(bucket), end = units.end(bucket); it != end; ++it) {
          const Literal a = *it;
          if (a.Subsumes(b)) {
            if (n_nulls > 0) {
              RemoveNulls();
            }
            return kSubsumed;
          } else if (Literal::Complementary(b, a)) {
            complementary = true;
          }
        }
        if (complementary) {
          Nullify(i);
          ++n_nulls;
        }
      }
    }
    if (n_nulls > 0) {
      RemoveNulls();
    }
    return n_nulls > 0 ? kPropagated : kUnchanged;
  }

  bool ground()      const { return all([](const Literal a) { return a.ground(); }); }
  bool primitive()   const { return all([](const Literal a) { return a.primitive(); }); }
  bool well_formed() const { return all([](const Literal a) { return a.well_formed(); }); }

  bool Mentions(const Literal a) const {
    return
#ifdef BLOOM
        lhs_bloom_.PossiblyContains(a.lhs()) &&
#endif
        any([a](Literal b) { return a == b; });
  }

  bool MentionsLhs(Term t) const {
    return
#ifdef BLOOM
        lhs_bloom_.PossiblyContains(t) &&
#endif
        any([t](Literal a) { return a.lhs() == t; });
  }

  template<typename UnaryPredicate>
  bool any(UnaryPredicate p) const {
    const size_t s = size();
    for (size_t i = 0; i < s; ++i) {
      if (p((*this)[i])) {
        return true;
      }
    }
    return false;
  }

  template<typename UnaryPredicate>
  bool all(UnaryPredicate p) const {
    const size_t s = size();
    for (size_t i = 0; i < s; ++i) {
      if (!p((*this)[i])) {
        return false;
      }
    }
    return true;
  }

  template<typename UnaryFunction>
  Clause Substitute(UnaryFunction theta, Term::Factory* tf) const {
    auto r = internal::transform_crange(*this, [theta, tf](Literal a) { return a.Substitute(theta, tf); });
    return Clause(size(), r.begin(), r.end());
  }

  template<typename UnaryFunction>
  typename internal::if_arg<UnaryFunction, Term>::type Traverse(UnaryFunction f) const {
    for (size_t i = 0; i < size(); ++i) {
      (*this)[i].Traverse(f);
    }
  }

  template<typename UnaryFunction>
  typename internal::if_arg<UnaryFunction, Literal>::type Traverse(UnaryFunction f) const {
    for (size_t i = 0; i < size(); ++i) {
      f((*this)[i]);
    }
  }

 private:
  friend class internal::array_iterator<Clause, Literal>;
  typedef internal::array_iterator<Clause, Literal> iterator;
  static constexpr size_t kArraySize = 5;

  explicit Clause(size_t size) : size_(size) {
    if (size2() > 0) {
      lits2_ = std::unique_ptr<Literal[]>(new Literal[size2()]);
    }
  }

  size_t size1() const { return size_ < kArraySize ? size_ : kArraySize; }
  size_t size2() const { return size_ < kArraySize ? 0 : (size_ - kArraySize); }

  Literal& operator[](size_t i) {
    assert(i <= size_);
    return i < kArraySize ? lits1_[i] : lits2_[i - kArraySize];
  }

  iterator begin() { return iterator(this, 0); }
  iterator end()   { return iterator(this, size()); }

  void Nullify(size_t i) {
    (*this)[i] = Literal();
    assert((*this)[i].null());
  }

  void RemoveNulls() {
    iterator new_end = std::remove_if(begin(), end(), [](const Literal a) { return a.null(); });
    size_ = new_end - begin();
#ifdef BLOOM
    InitBloom();
#endif
    assert(!any([](Literal a) { return a.null(); }));
  }

  void Normalize() {
    Clause& c = *this;
    size_t j = 0;
    for (size_t i = 0; i < size(); ++i) {
      if (c[i].valid()) {
        c[0] = c[i];
        size_ = 1;
        return;
      }
      if (c[i].unsatisfiable()) {
        goto skip;
      }
      for (size_t k = 0; k < j; ++k) {
        if (c[i].Subsumes(c[k])) {
          goto skip;
        }
      }
      for (size_t k = i + 1; k < size(); ++k) {
        if (c[i].ProperlySubsumes(c[k])) {
          goto skip;
        }
      }
      {
        const Literal a = c[i];
        size_t k = j++;
        for (; k > 0 && a < c[k - 1]; --k) {
          c[k] = c[k - 1];
        }
        c[k] = a;
      }
skip:
      {}
    }
    size_ = j;
  }

#ifdef BLOOM
  void InitBloom() {
    lhs_bloom_.Clear();
    for (size_t i = 0; i < size(); ++i) {
      lhs_bloom_.Add((*this)[i].lhs());
    }
  }
#endif

  size_t size_ = 0;
#ifdef BLOOM
  internal::BloomSet<Term> lhs_bloom_;
#endif
  Literal lits1_[kArraySize];
  std::unique_ptr<Literal[]> lits2_;
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

