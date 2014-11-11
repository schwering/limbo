// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <deque>
#include <list>
#include <set>
#include <utility>
#include "./ewff.h"
#include "./literal.h"

namespace esbl {

class SimpleClause : public std::set<Literal> {
 public:
  static const SimpleClause EMPTY;

  using std::set<Literal>::set;
  bool operator==(const SimpleClause& c) const;
  bool operator!=(const SimpleClause& c) const;
  bool operator<=(const SimpleClause& c) const;
  bool operator>=(const SimpleClause& c) const;
  bool operator<(const SimpleClause& c) const;
  bool operator>(const SimpleClause& c) const;

  SimpleClause PrependActions(const TermSeq& z) const;

  SimpleClause Substitute(const Unifier& theta) const;
  SimpleClause Ground(const Assignment& theta) const;
  std::list<Unifier> Subsumes(const SimpleClause& c) const;
  Maybe<std::pair<Unifier, SimpleClause>> Unify(const Atom& cl_a,
                                                const Atom& ext_a) const;

  std::list<SimpleClause> Instances(const StdName::SortedSet& hplus) const;

  std::set<Atom> Sensings() const;

  std::set<Variable> Variables() const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

  bool is_ground() const;

 private:
  void SubsumedBy(const const_iterator first, const const_iterator last,
                  const Unifier& theta, std::list<Unifier>* thetas) const;

  void GenerateInstances(const std::set<Variable>::const_iterator first,
                         const std::set<Variable>::const_iterator last,
                         const StdName::SortedSet& hplus,
                         Assignment* theta,
                         std::list<SimpleClause>* instances) const;
};

class Clause {
 public:
  static const Clause EMPTY;
  static const Clause MIN_UNIT;
  static const Clause MAX_UNIT;

  Clause() = default;
  Clause(const Ewff& e, const SimpleClause& ls)
      : Clause(false, e, ls) {}
  Clause(bool box, const Ewff& e, const SimpleClause& ls)
      : box_(box), e_(e), ls_(ls) { e_.RestrictVariable(ls_.Variables()); }
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;

  bool operator==(const Clause& c) const;
  bool operator!=(const Clause& c) const;
  bool operator<(const Clause& c) const;

  Clause InstantiateBox(const TermSeq& z) const;

  void Rel(const StdName::SortedSet& hplus,
           const Literal& ext_l,
           std::deque<Literal>* rel) const;
  bool Subsumes(const Clause& c) const;
  bool SplitRelevant(const Atom& a, const Clause& c, int k) const;
  size_t ResolveWithUnit(const Clause& unit, std::set<Clause>* rs) const;

  bool box() const { return box_; }
  const Ewff& ewff() const { return e_; }
  const SimpleClause& literals() const { return ls_; }
  bool is_unit() const { return ls_.size() == 1; }

  std::set<Atom::PredId> positive_preds() const;
  std::set<Atom::PredId> negative_preds() const;

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  Maybe<Clause> Substitute(const Unifier& theta) const;
  Maybe<std::pair<Unifier, Clause>> Unify(const Atom& cl_a,
                                          const Atom& ext_a) const;

  bool box_;
  Ewff e_;
  SimpleClause ls_;
};

std::ostream& operator<<(std::ostream& os, const SimpleClause& c);
std::ostream& operator<<(std::ostream& os, const Clause& c);

}  // namespace esbl

#endif  // SRC_CLAUSE_H_

