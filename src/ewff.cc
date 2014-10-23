// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./ewff.h"
#include <algorithm>
#include <cassert>
#include <memory>

Ewff::Conj::Conj(const std::map<Term::Var, Term::Name>& eq_name,
                 const std::multimap<Term::Var, Term::Var>& eq_var,
                 const std::multimap<Term::Var, Term::Name>& neq_name,
                 const std::multimap<Term::Var, Term::Var>& neq_var)
  : eq_name_(eq_name),
    eq_var_(eq_var),
    neq_name_(neq_name),
    neq_var_(neq_var) {
  for (const auto& xem : eq_name_) {
    assert(xem.first.is_variable());
    assert(xem.second.is_name());
    vars_.insert(xem.first);
    fix_vars_.insert(xem.first);
    names_.insert(xem.second);
  }
  for (const auto& xey : eq_var_) {
    assert(xey.first.is_variable());
    assert(xey.second.is_variable());
    vars_.insert(xey.first);
    vars_.insert(xey.second);
    var_vars_.insert(xey.first);
    var_vars_.insert(xey.second);
  }
  for (const auto& xnm : neq_name_) {
    assert(xnm.first.is_variable());
    assert(xnm.second.is_name());
    vars_.insert(xnm.first);
    var_vars_.insert(xnm.first);
    names_.insert(xnm.second);
  }
  for (const auto& xny : neq_var_) {
    assert(xny.first.is_variable());
    assert(xny.second.is_variable());
    vars_.insert(xny.first);
    vars_.insert(xny.second);
    var_vars_.insert(xny.first);
    var_vars_.insert(xny.second);
  }
}

bool Ewff::Conj::operator==(const Conj& c) {
  return eq_name_ == c.eq_name_ && eq_var_ == c.eq_var_ &&
      neq_name_ == c.neq_name_ && neq_var_ == c.neq_var_;
}

bool Ewff::Conj::operator<(const Conj& c) {
  if (eq_name_ < c.eq_name_) {
    return true;
  }
  if (eq_name_ != c.eq_name_) {
    return false;
  }
  if (eq_var_ < c.eq_var_) {
    return true;
  }
  if (eq_var_ != c.eq_var_) {
    return false;
  }
  if (neq_name_ < c.neq_name_) {
    return false;
  }
  if (neq_name_ != c.neq_name_) {
    return false;
  }
  if (neq_var_ < c.neq_var_) {
    return false;
  }
}

bool Ewff::Conj::CheckModel(const Term::Assignment& theta) const {
  const auto& end = theta.end();
  for (const auto& xey : eq_var_) {
    assert(xey.first.is_variable());
    assert(xey.second.is_variable());
    const auto& xem = theta.find(xey.first);
    const auto& yen = theta.find(xey.second);
    if (xem == end || yen == end || xem->second != yen->second) {
      return false;
    }
  }
  for (const auto& xnm : neq_name_) {
    assert(xnm.first.is_variable());
    assert(xnm.second.is_ground());
    const auto& xen = theta.find(xnm.first);
    if (xen == end || xnm.second == xen->second) {
      return false;
    }
  }
  for (const auto& xny : neq_var_) {
    assert(xny.first.is_variable());
    assert(xny.second.is_variable());
    const auto& xem = theta.find(xny.first);
    const auto& yen = theta.find(xny.second);
    if (xem == end || yen == end || xem->second == yen->second) {
      return false;
    }
  }
  for (const auto& xen1 : eq_name_) {
    assert(xen1.first.is_variable());
    assert(xen1.second.is_ground());
    const auto& xen2 = theta.find(xen1.first);
    if (xen2 == end || xen1.second == xen2->second) {
      return false;
    }
  }
  return true;
}

void Ewff::Conj::GenerateModels(const Term::VarSet::const_iterator var_first,
                                const Term::VarSet::const_iterator var_last,
                                const Term::NameSet& hplus,
                                Term::Assignment* theta,
                                std::list<Term::Assignment> *models) const {
  if (var_first != var_last) {
    const Term::Var& x = *var_first;
    for (const Term::Name& n : hplus) {
      (*theta)[x] = n;
      GenerateModels(std::next(var_first), var_last, hplus, theta, models);
    }
    theta->erase(x);
  } else if (CheckModel(*theta)) {
    models->push_back(*theta);
  }
}

void Ewff::Conj::FindModels(const Term::NameSet& hplus,
                            std::list<Term::Assignment> *models) const {
  Term::Assignment theta;
  GenerateModels(var_vars_.begin(), var_vars_.end(), hplus, &theta, models);
}

const Term::VarSet& Ewff::Conj::variables() const {
  return vars_;
}

const Term::NameSet& Ewff::Conj::names() const {
  return names_;
}


Ewff::Ewff(std::initializer_list<Ewff::Conj> cs) {
  cs_.insert(cs_.end(), cs.begin(), cs.end());
}

bool Ewff::CheckModel(const Term::Assignment& theta) const {
  for (auto& c : cs_) {
    c.CheckModel(theta);
  }
  return false;
}

std::list<Term::Assignment> Ewff::FindModels(const Term::NameSet& hplus) const {
  std::list<Term::Assignment> models;
  for (auto& c : cs_) {
    c.FindModels(hplus, &models);
  }
  return models;
}

Term::VarSet Ewff::variables() const {
  Term::VarSet vars;
  for (auto& c : cs_) {
    const Term::VarSet& vs = c.variables();
    std::copy(vs.begin(), vs.end(), std::inserter(vars, vars.begin()));
  }
  return vars;
}

Term::NameSet Ewff::names() const {
  Term::NameSet names;
  for (auto& c : cs_) {
    const Term::NameSet& ns = c.names();
    std::copy(ns.begin(), ns.end(), std::inserter(names, names.begin()));
  }
  return names;
}

