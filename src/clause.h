// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <list>
#include <set>
#include <utility>
#include "./ewff.h"
#include "./literal.h"

typedef std::set<Literal> GroundClause;

class Clause {
 public:
  Clause() = default;
  Clause(bool box, const Ewff& e, const GroundClause& ls)
      : box_(box), e_(e), ls_(ls) {}
  Clause(bool box, const Ewff& e, std::initializer_list<Literal> ls)
      : box_(box), e_(e), ls_(ls) {}
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;

  Clause PrependActions(const TermSeq& z) const;

  GroundClause Rel(const StdName::SortedSet& hplus,
                        const Literal &ext_l) const;

  bool box() const { return box_; }
  const Ewff& ewff() const { return e_; }
  const GroundClause& literals() const { return ls_; }
  size_t size() const { return ls_.size(); }

  std::set<Atom::PredId> positive_preds() const;
  std::set<Atom::PredId> negative_preds() const;

 private:
  std::pair<bool, Clause> Substitute(const Unifier& theta) const;
  std::pair<bool, Clause> Unify(const Atom& cl_a, const Atom& ext_a) const;

  bool box_;
  Ewff e_;
  GroundClause ls_;
};

std::ostream& operator<<(std::ostream& os, const Clause& c);

#endif  // SRC_CLAUSE_H_

