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

class Clause : public std::vector<Literal> {
 public:
  using std::vector<Literal>::vector;

  void Minimize() {
    std::remove_if(begin(), end(), [](const Literal& a) { return a.invalid(); });
    std::sort(begin(), end(), Literal::Comparator());
    erase(std::unique(begin(), end()), end());
  }

  bool unit() const { return empty(); }
  bool valid() const { return std::any_of(begin(), end(), [](const Literal a) { return a.valid(); }); }
  bool invalid() const { return std::all_of(begin(), end(), [](const Literal a) { return a.invalid(); }); }

  bool Subsumes(const Clause& c) const {
    for (Literal a : *this) {
      const bool subsumed = std::any_of(c.begin(), c.end(), [a](const Literal b) { return a.Subsumes(b); });
      if (!subsumed) {
        return false;
      }
    }
    return true;
  }

  Maybe<Clause> PropagateUnit(Literal a) {
    Clause c;
    std::copy_if(begin(), end(), std::back_inserter(c),
                [a](Literal b) { return !Literal::Complementary(a, b); }); 
    if (c.size() != size()) {
      return Just(c);
    } else {
      return Nothing;
    }
  }

  bool ground() const { return std::all_of(begin(), end(), [](Literal l) { return l.ground(); }); }
  bool primitive() const { return std::all_of(begin(), end(), [](Literal l) { return l.primitive(); }); }

  Clause Substitute(Term pre, Term post) const {
    Clause c;
    std::transform(begin(), end(), c.begin(),
                   [pre, post](Literal a) { return a.Substitute(pre, post); });
    return c;
  }

  Clause Ground(const Term::Substitution& theta) const {
    Clause c;
    std::transform(begin(), end(), c.begin(),
                   [&theta](Literal a) { return a.Ground(theta); });
    return c;
  }

  template<class UnaryPredicate>
  void CollectTerms(UnaryPredicate p, Term::Set* ts) const {
    for (Literal a : *this) {
      a.CollectTerms(p, ts);
    }
  }
};

}  // namespace lela

#endif  // SRC_CLAUSE_H_

