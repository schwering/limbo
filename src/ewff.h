// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// An ewff is a formula that mentions no predicates other than equality and all
// variables are universally quantified. We further restrict it to be in
// disjunctive normal form (DNF).
// The restriction to DNF allows to quickly generate minimal models.
//
// A potential optimization is to manage a map of dependent variables, which can
// be assigned once the variable has been assigned on which they depend (because
// there is a constraint x=y).
//
// We could restrict us even only to conjunctions, because a clause
// (e1 v e2)->c is equivalent to two clauses e1->c and e2->c. What's better
// in terms of performance.

#ifndef SRC_EWFF_H_
#define SRC_EWFF_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "./term.h"

namespace esbl {

class Ewff {
 public:
  class Conj {
   public:
    Conj() = default;
    Conj(const Assignment& eq_name,
         const std::set<std::pair<Variable, StdName>>& neq_name)
        : eq_name_(eq_name), neq_name_(neq_name) {}
    Conj(const Conj&) = default;
    Conj& operator=(const Conj&) = default;

    static std::pair<bool, Ewff::Conj> Create(
        const Assignment& eq_name,
        const std::set<std::pair<Variable, Variable>>& eq_var,
        const std::set<std::pair<Variable, StdName>>& neq_name,
        const std::set<std::pair<Variable, Variable>>& neq_var);
    static std::pair<bool, Conj> And(const Conj& c1, const Conj& c2);

    bool operator==(const Conj& c) const;
    bool operator!=(const Conj& c) const;
    bool operator<(const Conj& c) const;

    std::pair<bool, Conj> Substitute(const Unifier& theta) const;
    std::pair<bool, Conj> Ground(const Assignment& theta) const;

    bool CheckModel(const Assignment &theta) const;
    bool Satisfiable(const StdName::SortedSet& hplus) const;
    void Models(const StdName::SortedSet& hplus,
                std::list<Assignment>* models) const;

    void CollectVariables(Variable::SortedSet* vs) const;
    void CollectNames(StdName::SortedSet* ns) const;

   private:
    friend std::ostream& operator<<(std::ostream& os, const Conj& c);

    bool SubstituteName(const Variable& x, const StdName& n);
    bool SubstituteVariable(const Variable& x, const Variable& y,
                            bool maybe_new_vars = false);
    bool GenerateModel(const std::set<Variable>::const_iterator first,
                       const std::set<Variable>::const_iterator last,
                       const StdName::SortedSet& hplus,
                       Assignment* theta) const;
    void GenerateModels(const std::set<Variable>::const_iterator var_first,
                        const std::set<Variable>::const_iterator var_last,
                        const StdName::SortedSet& hplus,
                        Assignment* theta,
                        std::list<Assignment>* models) const;

    bool is_fix_var(const Variable& x) const { return eq_name_.count(x) > 0; }
    bool is_var_var(const Variable& x) const { return var_vars_.count(x) > 0; }

    Assignment eq_name_;
    std::set<std::pair<Variable, Variable>> eq_var_;
    std::set<std::pair<Variable, StdName>> neq_name_;
    std::set<std::pair<Variable, Variable>> neq_var_;
    std::set<Variable> var_vars_;
  };

  static const Ewff TRUE;
  static const Ewff FALSE;

  Ewff() = default;
  Ewff(std::initializer_list<Ewff::Conj> cs) : cs_(cs) {}
  Ewff(const Ewff&) = default;
  Ewff& operator=(const Ewff&) = default;

  bool operator==(const Ewff& e) const;
  bool operator!=(const Ewff& e) const;
  bool operator<(const Ewff& e) const;

  static Ewff And(const Ewff& e1, const Ewff& e2);

  std::pair<bool, Ewff> Substitute(const Unifier& theta) const;
  std::pair<bool, Ewff> Ground(const Assignment& theta) const;

  bool CheckModel(const Assignment &theta) const;
  std::list<Assignment> Models(const StdName::SortedSet& hplus) const;
  bool Satisfiable(const StdName::SortedSet& hplus) const;

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  friend std::ostream& operator<<(std::ostream& os, const Ewff& e);

  std::set<Conj> cs_;
};

std::ostream& operator<<(std::ostream& os, const Ewff::Conj& c);
std::ostream& operator<<(std::ostream& os, const Ewff& e);

}  // namespace esbl

#endif  // SRC_EWFF_H_

