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

#include <algorithm>
#include <utility>
#include <vector>

#include <lela/literal.h>
#include <lela/internal/bloom.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>
#include <lela/internal/traits.h>

namespace lela {

class Clause {
 public:
  struct IsNull { bool operator()(Literal a) { return !a.null(); } };
  typedef internal::filter_iterator<std::vector<Literal>::const_iterator, IsNull> const_iterator;

  Clause() = default;
  Clause(std::initializer_list<Literal> lits) : lits_(lits) { Minimize(); }
  template<typename InputIt>
  Clause(InputIt begin, InputIt end) : lits_(begin, end) { Minimize(); }

  bool operator==(const Clause& c) const { return lhs_bloom_ == c.lhs_bloom_ && lits_ == c.lits_; }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  const_iterator begin() const { return const_iterator(lits_.begin(), lits_.end()); }
  const_iterator end()   const { return const_iterator(lits_.end(), lits_.end()); }

  Literal head() const { return lits_[0]; }

  bool        empty() const { return lits_.empty(); }
  bool        unit()  const { return size() == 1; }
  std::size_t size()  const { return lits_.size(); }

  bool valid()   const { return any_of([](const Literal a) { return a.valid(); }); }
  bool invalid() const { return all_of([](const Literal a) { return a.invalid(); }); }

  internal::BloomSet<Term> lhs_bloom() const { return lhs_bloom_; }

  bool Subsumes(const Clause& c) const {
    assert(primitive());
    assert(c.primitive());
    return lhs_bloom_.PossiblySubsetOf(c.lhs_bloom_) &&
        all_of([&c](const Literal a) { return c.any_of([a](const Literal b) { return a.Subsumes(b); }); });
  }

  internal::Maybe<Clause> PropagateUnit(Literal a) const {
    assert(primitive());
    assert(a.primitive());
    assert(a.lhs().function());
    if (!lhs_bloom_.PossiblyContains(a.lhs())) {
      return internal::Nothing;
    }
    auto r = internal::filter_range(lits_.begin(), lits_.end(),
                                    [a](Literal b) { return !b.null() && !Literal::Complementary(a, b); });
    Clause c(r.begin(), r.end());
    return c.size() != size() ? internal::Just(c) : internal::Nothing;
  }

  bool ground()         const { return all_of([](Literal a) { return a.ground(); }); }
  bool primitive()      const { return all_of([](Literal a) { return a.primitive(); }); }
  bool quasiprimitive() const { return all_of([](Literal a) { return a.quasiprimitive(); }); }

  bool MentionsLhs(Term t) const {
    return lhs_bloom_.PossiblyContains(t) && any_of([t](Literal a) { return a.lhs() == t; });
  }

  template<typename UnaryPredicate>
  bool any_of(UnaryPredicate p) const {
    for (Literal a : lits_) {
      if (!a.null() && p(a)) {
        return true;
      }
    }
    return false;
  }

  template<typename UnaryPredicate>
  bool all_of(UnaryPredicate p) const {
    for (Literal a : lits_) {
      if (!a.null() && !p(a)) {
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
    for (Literal a : *this) {
      a.Traverse(f);
    }
  }

  template<typename UnaryFunction>
  typename internal::if_arg<UnaryFunction, Literal>::type Traverse(UnaryFunction f) const {
    for (Literal a : *this) {
      f(a);
    }
  }

 private:
  void Minimize() {
    lits_.erase(std::remove_if(lits_.begin(), lits_.end(), [](const Literal a) { return a.null() || a.invalid(); }),
                lits_.end());
    std::sort(lits_.begin(), lits_.end(), [](Literal a, Literal b) { return a.hash() < b.hash(); });
    lits_.erase(std::unique(lits_.begin(), lits_.end()), lits_.end());
    InitBloom();
  }

  void InitBloom() {
    lhs_bloom_.Clear();
    for (Literal a : lits_) {
      if (!a.null()) {
        lhs_bloom_.Add(a.lhs());
      }
    }
  }

  internal::BloomSet<Term> lhs_bloom_;
  std::vector<Literal> lits_;
};

}  // namespace lela

#endif  // LELA_CLAUSE_H_

