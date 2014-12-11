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
#include "./maybe.h"
#include "./term.h"

namespace lela {

class Ewff {
 public:
  struct VarNeqName : public std::pair<Variable, StdName> {
    typedef std::set<VarNeqName> Set;
    using std::pair<Variable, StdName>::pair;
  };
  struct VarNeqVar : public std::pair<Variable, Variable> {
    typedef std::set<VarNeqVar> Set;
    using std::pair<Variable, Variable>::pair;
  };
  struct Comparator;

  static const Ewff TRUE;

  Ewff() = default;
  Ewff(const Ewff&) = default;
  Ewff& operator=(const Ewff&) = default;

  static Maybe<Ewff> Create(const VarNeqName::Set& neq_name,
                            const VarNeqVar::Set& neq_var);
  static Ewff And(const Ewff& e1, const Ewff& e2);

  bool operator==(const Ewff& e) const {
    return neq_name_ == e.neq_name_ && neq_var_ == e.neq_var_;
  }

  Maybe<Ewff> Substitute(const Unifier& theta) const;
  Maybe<Ewff> Ground(const Assignment& theta) const;

  bool Subsumes(const Ewff& e) const;

  bool SatisfiedBy(const Assignment &theta) const;
  std::list<Assignment> Models(const StdName::SortedSet& hplus) const;

  void RestrictVariable(const Variable::Set& vs);

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  friend std::ostream& operator<<(std::ostream& os, const Ewff& e);

  Ewff(const VarNeqName::Set& neq_name, const VarNeqVar::Set& neq_var);

  bool SubstituteName(const Variable& x, const StdName& n);
  bool SubstituteVariable(const Variable& x, const Variable& y);

  void GenerateModels(const Variable::Set::const_iterator first,
                      const Variable::Set::const_iterator last,
                      const StdName::SortedSet& hplus,
                      Assignment* theta,
                      std::list<Assignment>* models) const;
  Variable::Set Variables() const;

  VarNeqName::Set neq_name_;
  VarNeqVar::Set neq_var_;
};

struct Ewff::Comparator {
  typedef Ewff value_type;

  bool operator()(const Ewff& e1, const Ewff& e2) const {
    return std::tie(e1.neq_name_, e1.neq_var_) <
           std::tie(e2.neq_name_, e2.neq_var_);
  }
};

std::ostream& operator<<(std::ostream& os, const Ewff& e);

}  // namespace lela

#endif  // SRC_EWFF_H_

