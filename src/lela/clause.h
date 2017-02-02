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
#include <initializer_list>
#include <utility>
#include <vector>

#include <lela/literal.h>
#include <lela/internal/bloom.h>
#include <lela/internal/iter.h>
#include <lela/internal/hashset.h>
#include <lela/internal/maybe.h>
#include <lela/internal/traits.h>

namespace lela {

class Clause {
 public:
  typedef internal::HashSet<Literal, Literal::LhsHasher> LiteralSet;
  typedef LiteralSet::const_iterator const_iterator;

  Clause() = default;
  Clause(std::initializer_list<Literal> lits) : lits_(lits) { Minimize(); }
  template<typename ForwardIt>
  Clause(ForwardIt begin, ForwardIt end) : lits_(begin, end) { Minimize(); }

  bool operator==(const Clause& c) const { return lhs_bloom_ == c.lhs_bloom_ && Subsumes(c) && c.Subsumes(*this); }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  const_iterator begin() const { return lits_.begin(); }
  const_iterator end()   const { return lits_.end(); }

  Literal head() const { return *begin(); }

  bool        empty() const { return lits_.empty(); }
  bool        unit()  const { return size() == 1; }
  std::size_t size()  const { return lits_.size(); }

  bool valid()   const { return std::any_of(begin(), end(), [](const Literal a) { return a.valid(); }); }
  bool invalid() const { return std::all_of(begin(), end(), [](const Literal a) { return a.invalid(); }); }

  internal::BloomSet<Term> lhs_bloom() const { return lhs_bloom_; }

  bool Subsumes(const Clause& c) const {
    assert(primitive());
    assert(c.primitive());
    return lhs_bloom_.PossiblySubsetOf(c.lhs_bloom_) &&
        std::all_of(begin(), end(),
                    [&c](const Literal a) {
                      return std::any_of(c.lits_.bucket_begin(a), c.lits_.bucket_end(),
                                         [a](const Literal b) {
                                           return a.Subsumes(b);
                                         });
                    });
  }

  internal::Maybe<Clause> PropagateUnit(Literal a) const {
    assert(primitive());
    assert(a.primitive());
    assert(a.lhs().function());
    if (!lhs_bloom_.PossiblyContains(a.lhs())) {
      return internal::Nothing;
    } else {
      Clause c = *this;
      for (auto it = c.lits_.bucket_begin(a), jt = c.lits_.bucket_end(); it != jt; ++it) {
        if (Literal::Complementary(a, *it)) {
          c.lits_.Remove(it);
        }
      }
      if (c.lits_.size() != size()) {
        c.InitBloom();
        return internal::Just(c);
      } else {
        return internal::Nothing;
      }
    }
  }

  bool ground()         const { return std::all_of(begin(), end(), [](Literal a) { return a.ground(); }); }
  bool primitive()      const { return std::all_of(begin(), end(), [](Literal a) { return a.primitive(); }); }
  bool quasiprimitive() const { return std::all_of(begin(), end(), [](Literal a) { return a.quasiprimitive(); }); }

  bool MentionsLhs(Term t) const {
    return lhs_bloom_.PossiblyContains(t) && std::any_of(begin(), end(), [t](Literal a) { return a.lhs() == t; });
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
    for (auto it = lits_.begin(), jt = lits_.end(); it != jt; ++it) {
      if (it->invalid()) {
        lits_.Remove(it);
      }
    }
    InitBloom();
  }

  void InitBloom() {
    lhs_bloom_.Clear();
    for (Literal a : *this) {
      lhs_bloom_.Add(a.lhs());
    }
  }

  internal::BloomSet<Term> lhs_bloom_;
  LiteralSet lits_;
};

}  // namespace lela

#endif  // LELA_CLAUSE_H_

