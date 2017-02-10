// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering
//
// A clause is a set of literals. Clauses are immutable.
//
// A clause is stored as vector, which is initially sorted to remove duplicates.
// Thus, and since clauses are immutable, they represent sets of literals. Note
// that copying and comparing clauses is more expensive than for literals.
//
// Perhaps the most important operations are PropagateUnit() and Subsumes(),
// which are only defined for primitive clauses and literals. Thus all involved
// literals mention a primitive term on the left-hand side. By definition of
// Complementary() and Subsumes() in the Literal class, a literal can react with
// another only if they refer to the same term. By hashing these terms and
// storing these values in Bloom filters, we can (hopefully often) detect early
// that unit propagation or subsumption won't work early (in a sound but
// incomplete way).

#ifndef LELA_CLAUSE_H_
#define LELA_CLAUSE_H_

#include <cassert>
#include <cstring>

#include <algorithm>
#include <memory>

#include <lela/literal.h>
#include <lela/internal/bloom.h>
#include <lela/internal/ints.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>
#include <lela/internal/traits.h>

namespace lela {

class Clause {
 public:
  typedef internal::size_t size_t;
  typedef internal::array_iterator<const Clause, const Literal> const_iterator;

  Clause() = default;

  explicit Clause(const Literal a) : size_(!a.invalid() ? 1 : 0) { lits1_[0] = a; InitBloom(); }

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
    Minimize();
    InitBloom();
  }

  Clause(const Clause& c) : Clause(c.size_) {
    std::memcpy(lits1_, c.lits1_, size1() * sizeof(Literal));
    if (size2() > 0) {
      std::memcpy(lits2_.get(), c.lits2_.get(), size2() * sizeof(Literal));
    }
    lhs_bloom_ = c.lhs_bloom_;
    assert(!any([](Literal a) { return a.invalid(); }));
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
    lhs_bloom_ = c.lhs_bloom_;
    assert(!any([](Literal a) { return a.invalid(); }));
    return *this;
  }

  Clause(Clause&&) = default;
  Clause& operator=(Clause&&) = default;

  bool operator==(const Clause& c) const {
    return size_ == c.size_ &&
           lhs_bloom_ == c.lhs_bloom_ &&
           std::memcmp(lits1_, c.lits1_, size1() * sizeof(Literal)) == 0 &&
           (size2() == 0 || std::memcmp(lits2_.get(), c.lits2_.get(), size2() * sizeof(Literal)) == 0);
  }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  const_iterator cbegin() const { return const_iterator(this, 0); }
  const_iterator cend()   const { return const_iterator(this, size_); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end()   const { return cend(); }

  Literal head() const { return lits1_[0]; }
  const Literal& operator[](size_t i) const {
    assert(i <= size_);
    return i < kArraySize ? lits1_[i] : lits2_[i - kArraySize];
  }

  bool   empty() const { return size_ == 0; }
  bool   unit()  const { return size() == 1; }
  size_t size()  const { return size_; }

  bool valid()   const { return any([](const Literal a) { return a.valid(); }); }
  bool invalid() const { return empty(); }

  internal::BloomSet<Term> lhs_bloom() const { return lhs_bloom_; }

  bool Subsumes(const Clause& c) const {
    return Subsumes(*this, c);
  }

  static bool Subsumes(const Clause& c, const Clause& d) {
    assert(c.primitive());
    assert(d.primitive());
    if (!c.lhs_bloom_.PossiblySubsetOf(d.lhs_bloom_)) {
      return false;
    }
    size_t i = 0;
    size_t j = 0;
    for (; i < c.size(); ++i) {
      for (; j < d.size() && c[i].lhs() > d[j].lhs(); ++j) {
      }
      for (auto k = j; k < d.size() && c[i].lhs() == d[k].lhs(); ++k) {
        if (c[i].Subsumes(d[k])) {
          goto next;
        }
      }
      assert(!c.all([&d](const Literal a) { return d.any([a](const Literal b) { return a.Subsumes(b); }); }));
      return false;
next:
      ;
    }
    assert(c.all([&d](const Literal a) { return d.any([a](const Literal b) { return a.Subsumes(b); }); }));
    return true;
  }

  internal::Maybe<Clause> PropagateUnit(const Literal b) const {
    assert(primitive());
    assert(b.primitive());
    if (!lhs_bloom_.PossiblyContains(b.lhs())) {
      return internal::Nothing;
    }
    Clause c(size());
    c.size_ = 0;
    for (size_t i = 0; i < size(); ++i) {
      const Literal a = (*this)[i];
      if (!Literal::Complementary(a, b)) {
        c[c.size_++] = a;
      }
    }
    c.InitBloom();
    return c.size() != size() ? internal::Just(c) : internal::Nothing;
  }

  template<typename ForwardIt>
  internal::Maybe<Clause> PropagateUnits(ForwardIt unit_begin, ForwardIt unit_end) const {
    assert(primitive());
    auto it = unit_begin;
    auto end = unit_end;
    Clause c(size());
    c.size_ = 0;
    for (size_t i = 0; i < size(); ++i) {
      const Literal a = (*this)[i];
      for (; it != end && a.lhs() > it->lhs(); ++it) {
        assert(a.primitive());
      }
      for (auto jt = it; jt != end && a.lhs() == jt->lhs(); ++jt) {
        if (Literal::Complementary(a, *jt)) {
          goto next;
        }
      }
      c[c.size_++] = a;
next:
      ;
    }
    c.InitBloom();
    return c.size() != size() ? internal::Just(c) : internal::Nothing;
  }

  bool ground()         const { return all([](Literal a) { return a.ground(); }); }
  bool primitive()      const { return all([](Literal a) { return a.primitive(); }); }
  bool quasiprimitive() const { return all([](Literal a) { return a.quasiprimitive(); }); }

  bool Mentions(Literal a) const {
    return lhs_bloom_.PossiblyContains(a.lhs()) && any([a](Literal b) { return a == b; });
  }

  bool MentionsLhs(Term t) const {
    return lhs_bloom_.PossiblyContains(t) && any([t](Literal a) { return a.lhs() == t; });
  }

  template<typename UnaryPredicate>
  bool any(UnaryPredicate p) const {
    for (size_t i = 0; i < size(); ++i) {
      if (p((*this)[i])) {
        return true;
      }
    }
    return false;
  }

  template<typename UnaryPredicate>
  bool all(UnaryPredicate p) const {
    for (size_t i = 0; i < size(); ++i) {
      if (!p((*this)[i])) {
        return false;
      }
    }
    return true;
  }

  template<typename UnaryFunction>
  Clause Substitute(UnaryFunction theta, Term::Factory* tf) const {
    auto r = internal::transform_range(begin(), end(), [theta, tf](Literal a) { return a.Substitute(theta, tf); });
    return Clause(r.begin(), r.end());
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
  friend struct internal::array_iterator<Clause, Literal>;
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
  iterator end()   { return iterator(this, size_); }

  void Minimize() {
    iterator new_end = std::remove_if(begin(), end(), [](const Literal a) { return a.invalid(); });
    std::sort(begin(), new_end, [](Literal a, Literal b) { return a < b; });
    new_end = std::unique(begin(), new_end);
    size_ = new_end - begin();
    assert(!any([](Literal a) { return a.invalid(); }));
  }

  void InitBloom() {
    lhs_bloom_.Clear();
    for (size_t i = 0; i < size(); ++i) {
      lhs_bloom_.Add((*this)[i].lhs());
    }
  }

  size_t size_ = 0;
  internal::BloomSet<Term> lhs_bloom_;
  Literal lits1_[kArraySize];
  std::unique_ptr<Literal[]> lits2_;
};

}  // namespace lela

#endif  // LELA_CLAUSE_H_

