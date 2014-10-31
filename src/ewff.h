// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

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
  static const Ewff TRUE;

  Ewff() = default;
  Ewff(const Ewff&) = default;
  Ewff& operator=(const Ewff&) = default;

  static std::pair<bool, Ewff> Create(
      const std::set<std::pair<Variable, StdName>>& neq_name,
      const std::set<std::pair<Variable, Variable>>& neq_var);
  static Ewff And(const Ewff& e1, const Ewff& e2);

  bool operator==(const Ewff& e) const;
  bool operator!=(const Ewff& e) const;
  bool operator<(const Ewff& e) const;

  std::pair<bool, Ewff> Substitute(const Unifier& theta) const;
  std::pair<bool, Ewff> Ground(const Assignment& theta) const;

  bool Subsumes(const Ewff& e) const;

  bool SatisfiedBy(const Assignment &theta) const;
  std::list<Assignment> Models(const StdName::SortedSet& hplus) const;

  void RestrictVariable(const std::set<Variable>& vs);

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  friend std::ostream& operator<<(std::ostream& os, const Ewff& e);

  Ewff(const std::set<std::pair<Variable, StdName>>& neq_name,
       const std::set<std::pair<Variable, Variable>>& neq_var);

  bool SubstituteName(const Variable& x, const StdName& n);
  bool SubstituteVariable(const Variable& x, const Variable& y);

  void GenerateModels(const std::set<Variable>::const_iterator first,
                      const std::set<Variable>::const_iterator last,
                      const StdName::SortedSet& hplus,
                      Assignment* theta,
                      std::list<Assignment>* models) const;
  std::set<Variable> Variables() const;

  std::set<std::pair<Variable, StdName>> neq_name_;
  std::set<std::pair<Variable, Variable>> neq_var_;
};

std::ostream& operator<<(std::ostream& os, const Ewff& e);

}  // namespace esbl

#endif  // SRC_EWFF_H_

