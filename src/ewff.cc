// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./ewff.h"
#include <algorithm>
#include <cassert>
#include <memory>

Ewff::Conj::Conj(const std::map<Variable, StdName>& eq_name,
                 const std::multimap<Variable, Variable>& eq_var,
                 const std::multimap<Variable, StdName>& neq_name,
                 const std::multimap<Variable, Variable>& neq_var)
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

bool Ewff::Conj::operator!=(const Conj& c) {
  return !operator==(c);
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
    return true;
  }
  if (neq_name_ != c.neq_name_) {
    return false;
  }
  if (neq_var_ < c.neq_var_) {
    return true;
  }
  return false;
}

std::pair<bool, Ewff::Conj> Ewff::Conj::Ground(const Assignment& theta) const {
  std::pair<bool, Conj> p;
  for (const auto& xen : theta) {
    const auto r = p.second.eq_name_.insert(xen);
    if (r.second) {
      p.first = false;
      return p;
    }
    p.second.fix_vars_.insert(xen.first);
    p.second.var_vars_.erase(xen.first);
    p.second.names_.insert(xen.second);
  }
  p.first = true;
  return p;
}

bool Ewff::Conj::CheckModel(const Assignment& theta) const {
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

void Ewff::Conj::GenerateModels(const std::set<Variable>::const_iterator first,
                                const std::set<Variable>::const_iterator last,
                                const std::set<StdName>& hplus,
                                Assignment* theta,
                                std::list<Assignment> *models) const {
  if (first != last) {
    const Variable& x = *first;
    for (const StdName& n : hplus) {
      (*theta)[x] = n;
      GenerateModels(std::next(first), last, hplus, theta, models);
    }
    theta->erase(x);
  } else if (CheckModel(*theta)) {
    models->push_back(*theta);
  }
}

void Ewff::Conj::FindModels(const std::set<StdName>& hplus,
                            std::list<Assignment> *models) const {
  Assignment theta = eq_name_;
  GenerateModels(var_vars_.begin(), var_vars_.end(), hplus, &theta, models);
}

const std::set<Variable>& Ewff::Conj::variables() const {
  return vars_;
}

const std::set<StdName>& Ewff::Conj::names() const {
  return names_;
}


Ewff::Ewff(std::initializer_list<Ewff::Conj> cs) {
  cs_.insert(cs_.end(), cs.begin(), cs.end());
}

std::pair<bool, Ewff> Ewff::Ground(const Assignment& theta) const {
  std::pair<bool, Ewff> p;
  for (const auto& c1 : cs_) {
    const auto q = c1.Ground(theta);
    if (q.first) {
      p.second.cs_.push_back(q.second);
    }
  }
  p.first = !p.second.cs_.empty();
  return p;
}

bool Ewff::CheckModel(const Assignment& theta) const {
  for (auto& c : cs_) {
    c.CheckModel(theta);
  }
  return false;
}

std::list<Assignment> Ewff::FindModels(const std::set<StdName>& hplus) const {
  std::list<Assignment> models;
  for (auto& c : cs_) {
    c.FindModels(hplus, &models);
  }
  return models;
}

std::set<Variable> Ewff::variables() const {
  std::set<Variable> vars;
  for (auto& c : cs_) {
    const std::set<Variable>& vs = c.variables();
    std::copy(vs.begin(), vs.end(), std::inserter(vars, vars.begin()));
  }
  return vars;
}

std::set<StdName> Ewff::names() const {
  std::set<StdName> names;
  for (auto& c : cs_) {
    const std::set<StdName>& ns = c.names();
    std::copy(ns.begin(), ns.end(), std::inserter(names, names.begin()));
  }
  return names;
}

std::ostream& operator<<(std::ostream& os, const Ewff::Conj& c) {
  os << '(';
  for (auto it = c.eq_name_.begin(); it != c.eq_name_.end(); ++it) {
    if (it != c.eq_name_.begin()) {
      os << " ^ " << ' ';
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = c.eq_var_.begin(); it != c.eq_var_.end(); ++it) {
    if (!c.eq_name_.empty() || it != c.eq_var_.begin()) {
      os << " ^ " << ' ';
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = c.neq_name_.begin(); it != c.neq_name_.end(); ++it) {
    if (!c.eq_var_.empty() || it != c.neq_name_.begin()) {
      os << " ^ " << ' ';
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = c.neq_var_.begin(); it != c.neq_var_.end(); ++it) {
    if (!c.neq_name_.empty() || it != c.neq_var_.begin()) {
      os << " ^ " << ' ';
    }
    os << it->first << " = " << it->second;
  }
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Ewff& e) {
  os << '(';
  for (auto it = e.cs_.begin(); it != e.cs_.end(); ++it) {
    if (it != e.cs_.begin()) {
      os << " v " << ' ';
    }
    os << *it;
  }
  os << ')';
  return os;
}

