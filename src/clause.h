// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <list>
#include <set>
#include <utility>
#include "./ewff.h"
#include "./literal.h"

namespace esbl {

typedef std::set<Literal> SimpleClause;

class Clause {
 public:
  Clause() = default;
  Clause(bool box, const Ewff& e, const SimpleClause& ls)
      : box_(box), e_(e), ls_(ls) {}
  Clause(bool box, const Ewff& e, std::initializer_list<Literal> ls)
      : box_(box), e_(e), ls_(ls) {}
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;

  bool operator==(const Clause& c) const;
  bool operator!=(const Clause& c) const;
  bool operator<(const Clause& c) const;

  Clause PrependActions(const TermSeq& z) const;

  std::set<Literal> Rel(const StdName::SortedSet& hplus,
                        const Literal& ext_l) const;
  bool Subsumes(const SimpleClause& c) const;
  bool SplitRelevant(const Atom& a, const SimpleClause c, unsigned int k) const;
  std::list<Clause> PropagateUnit(const Literal& ext_l) const;

  bool box() const { return box_; }
  const Ewff& ewff() const { return e_; }
  const SimpleClause& literals() const { return ls_; }

  std::set<Atom::PredId> positive_preds() const;
  std::set<Atom::PredId> negative_preds() const;

 private:
  std::tuple<bool, Clause> Substitute(const Unifier& theta) const;
  std::tuple<bool, Unifier, Clause> Unify(const Atom& cl_a,
                                          const Atom& ext_a) const;
  bool Subsumes(SimpleClause::const_iterator cl_l_it,
                const SimpleClause& c) const;

  bool box_;
  Ewff e_;
  SimpleClause ls_;
};

std::ostream& operator<<(std::ostream& os, const SimpleClause& c);
std::ostream& operator<<(std::ostream& os, const Clause& c);

}  // namespace esbl

#endif  // SRC_CLAUSE_H_

