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
    Conj(const std::map<Term::Var, Term::Name>& eq_name,
         const std::multimap<Term::Var, Term::Var>& eq_var,
         const std::multimap<Term::Var, Term::Name>& neq_name,
         const std::multimap<Term::Var, Term::Var>& neq_var);
    Conj(const Conj&) = default;
    Conj& operator=(const Conj&) = default;

    bool operator==(const Conj& c);
    bool operator<(const Conj& c);

    bool CheckModel(const Term::Assignment &theta) const;
    void FindModels(const Term::NameSet& hplus,
                    std::list<Term::Assignment> *models) const;

    const Term::VarSet& variables() const;
    const Term::NameSet& names() const;

   private:
    void GenerateModels(const Term::VarSet::const_iterator var_first,
                        const Term::VarSet::const_iterator var_last,
                        const Term::NameSet& hplus,
                        Term::Assignment* theta,
                        std::list<Term::Assignment> *models) const;

    std::map<Term::Var, Term::Name> eq_name_;
    std::multimap<Term::Var, Term::Var> eq_var_;
    std::multimap<Term::Var, Term::Name> neq_name_;
    std::multimap<Term::Var, Term::Var> neq_var_;
    Term::VarSet vars_;
    Term::VarSet fix_vars_;
    Term::VarSet var_vars_;
    Term::NameSet names_;
  };

  Ewff(std::initializer_list<Ewff::Conj> cs);
  Ewff(const Ewff&) = default;
  Ewff& operator=(const Ewff&) = default;

  bool CheckModel(const Term::Assignment &theta) const;
  std::list<Term::Assignment> FindModels(const Term::NameSet& hplus) const;

  Term::VarSet variables() const;
  Term::NameSet names() const;

 private:
  std::vector<Conj> cs_;
};

#endif  // SRC_EWFF_H_

