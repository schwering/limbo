// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
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

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <cassert>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>
#include "./bloom.h"
#include "./compar.h"
#include "./iter.h"
#include "./literal.h"
#include "./maybe.h"

namespace lela {

class Clause {
 public:
  typedef std::vector<Literal>::const_iterator const_iterator;

  Clause() = default;
  Clause(std::initializer_list<Literal> lits) : lits_(lits) { Minimize(); }
  template<typename Iter>
  Clause(Iter begin, Iter end) : lits_(begin, end) { Minimize(); }

  bool operator==(const Clause& c) const { return bloom_ == c.bloom_ && lits_ == c.lits_; }
  bool operator!=(const Clause& c) const { return !(*this == c); }

  const_iterator begin() const { return lits_.begin(); }
  const_iterator end()   const { return lits_.end(); }

  bool   empty() const { return lits_.empty(); }
  bool   unit()  const { return size() == 1; }
  size_t size()  const { return lits_.size(); }

  bool valid()   const { return std::any_of(begin(), end(), [](const Literal a) { return a.valid(); }); }
  bool invalid() const { return std::all_of(begin(), end(), [](const Literal a) { return a.invalid(); }); }

  bool Subsumes(const Clause& c) const {
    if (!BloomFilter::Subset(bloom_, c.bloom_)) {
      return false;
    }
    for (Literal a : *this) {
      const bool subsumed = std::any_of(c.begin(), c.end(), [a](const Literal b) { return a.Subsumes(b); });
      if (!subsumed) {
        return false;
      }
    }
    return true;
  }

  Maybe<Clause> PropagateUnit(Literal a) const {
    assert(primitive());
    assert(a.primitive());
    assert(a.lhs().function());
    if (!bloom_.Contains(a.lhs().hash())) {
      return Nothing;
    }
    auto r = filter_range([a](Literal b) { return !Literal::Complementary(a, b); }, begin(), end());
    Clause c(r.begin(), r.end());
    if (c.size() != size()) {
      return Just(c);
    } else {
      return Nothing;
    }
  }

  bool ground()         const { return std::all_of(begin(), end(), [](Literal a) { return a.ground(); }); }
  bool primitive()      const { return std::all_of(begin(), end(), [](Literal a) { return a.primitive(); }); }
  bool quasiprimitive() const { return std::all_of(begin(), end(), [](Literal a) { return a.quasiprimitive(); }); }

  template<typename UnaryFunction>
  Clause Substitute(UnaryFunction theta, Term::Factory* tf) const {
    auto r = transform_range([theta, tf](Literal a) { return a.Substitute(theta, tf); }, begin(), end());
    return Clause(r.begin(), r.end());
  }

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const {
    for (Literal a : *this) {
      a.Traverse(f);
    }
  }

 private:
  typedef std::vector<Literal>::iterator iterator;

  void Minimize() {
    lits_.erase(std::remove_if(lits_.begin(), lits_.end(), [](const Literal a) { return a.invalid(); }), end());
    std::sort(lits_.begin(), lits_.end(), Literal::Comparator());
    lits_.erase(std::unique(lits_.begin(), lits_.end()), end());
    InitBloom();
  }

  void InitBloom() {
    bloom_.Clear();
    for (Literal a : *this) {
      bloom_.Add(a.lhs().hash());
    }
  }

  BloomFilter bloom_;
  std::vector<Literal> lits_;
};

}  // namespace lela

#endif  // SRC_CLAUSE_H_

