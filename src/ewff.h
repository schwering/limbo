// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// An ewff is a formula that mentions no predicates other than equality and all
// variables are universally quantified. We further restrict it to be in
// disjunctive normal form (DNF).
//
// The restriction to DNF allows to quickly generate minimal models.
// Notice that the objects provided to the ctor of Conj should be complete, that
// is, if x=y then also y=x etc.

#ifndef SRC_EWFF_H_
#define SRC_EWFF_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "./term.h"

class Ewff {
 public:
  class Conj {
   public:
    Conj(const std::map<Variable, StdName>& eq_name,
         const std::multimap<Variable, Variable>& eq_var,
         const std::multimap<Variable, StdName>& neq_name,
         const std::multimap<Variable, Variable>& neq_var);
    Conj(const Conj&) = default;
    Conj& operator=(const Conj&) = default;

    bool operator==(const Conj& c);
    bool operator<(const Conj& c);

    bool Ground(const Assignment& theta, Conj* c) const;
    bool CheckModel(const Assignment &theta) const;
    void FindModels(const std::set<StdName>& hplus,
                    std::list<Assignment> *models) const;

    const std::set<Variable>& variables() const;
    const std::set<StdName>& names() const;

   private:
    friend class Ewff;

    Conj() = default;

    void GenerateModels(const std::set<Variable>::const_iterator var_first,
                        const std::set<Variable>::const_iterator var_last,
                        const std::set<StdName>& hplus,
                        Assignment* theta,
                        std::list<Assignment> *models) const;

    std::map<Variable, StdName> eq_name_;
    std::multimap<Variable, Variable> eq_var_;
    std::multimap<Variable, StdName> neq_name_;
    std::multimap<Variable, Variable> neq_var_;
    std::set<Variable> vars_;
    std::set<Variable> fix_vars_;
    std::set<Variable> var_vars_;
    std::set<StdName> names_;
  };

  Ewff(std::initializer_list<Ewff::Conj> cs);
  Ewff(const Ewff&) = default;
  Ewff& operator=(const Ewff&) = default;

  bool Ground(const Assignment& theta, Ewff* e) const;
  bool CheckModel(const Assignment &theta) const;
  std::list<Assignment> FindModels(const std::set<StdName>& hplus) const;

  std::set<Variable> variables() const;
  std::set<StdName> names() const;

 private:
  std::vector<Conj> cs_;
};

#endif  // SRC_EWFF_H_

