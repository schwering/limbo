// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <cassert>
#include <algorithm>
#include <set>
#include <utility>
#include "./compar.h"
#include "./literal.h"
#include "./maybe.h"
#include "./range.h"

namespace lela {

class Clause {
  typedef std::vector<Literal> Vector;

  Clause() = default;
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;
  template<class InputIt> Clause(InputIt first, InputIt last) : ls_(first, last) { }

  const Vector& literals() const { return ls_; }

  void Minimize() {
    std::remove_if(ls_.begin(), ls_.end(), [](const Literal& a) { return a.invalid(); });
    std::sort(ls_.begin(), ls_.end(), Literal::Comparator());
    ls_.erase(std::unique(ls_.begin(), ls_.end()), ls_.end());
  }

  bool valid() const { return std::any_of(ls_.begin(), ls_.end(), [](const Literal& a) { return a.valid(); }); }
  bool invalid() const { return std::all_of(ls_.begin(), ls_.end(), [](const Literal& a) { return a.invalid(); }); }

  bool Subsumes(const Clause& c) const {
    // Some kind of hashing might help to check if subsumption could be true.
    for (Literal a : ls_) {
      bool subsumed = false;
      for (Literal b : c.ls_) {
        subsumed = a.Subsumes(b);
      }
      if (!subsumed) {
        return false;
      }
    }
    return true;
  }

  bool PropagateUnit(Literal a) {
    const size_t pre = ls_.size();
    std::remove_if(ls_.begin(), ls_.end(), [a](Literal b) { return Literal::Complementary(a, b); });
    const size_t post = ls_.size();
    return pre != post;
  }

  bool ground() const { return std::all_of(ls_.begin(), ls_.end(), [](Literal l) { return l.ground(); }); }
  bool primitive() const { return std::all_of(ls_.begin(), ls_.end(), [](Literal l) { return l.primitive(); }); }

  Clause Substitute(Term pre, Term post) const {
    Clause c;
    std::transform(ls_.begin(), ls_.end(), c.ls_.begin(),
                   [pre, post](Literal a) { return a.Substitute(pre, post); });
    return c;
  }

  Clause Ground(const Term::Substitution& theta) const {
    Clause c;
    std::transform(ls_.begin(), ls_.end(), c.ls_.begin(),
                   [&theta](Literal a) { return a.Ground(theta); });
    return c;
  }

  template<class UnaryPredicate>
  void CollectTerms(UnaryPredicate p, Term::Set* ts) const {
    for (Literal a : ls_) {
      a.CollectTerms(p, ts);
    }
  }

 private:
  Vector ls_;
};

}  // namespace lela

#endif  // SRC_CLAUSE_H_

