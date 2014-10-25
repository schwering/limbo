// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./ewff.h"
#include <algorithm>
#include <cassert>
#include <memory>

Ewff::Conj::Conj(const Assignment& eq_name,
                 const std::set<std::pair<Variable, Variable>>& eq_var,
                 const std::set<std::pair<Variable, StdName>>& neq_name,
                 const std::set<std::pair<Variable, Variable>>& neq_var)
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
    if (fix_vars_.count(xey.first) == 0) {
      var_vars_.insert(xey.first);
    }
    if (fix_vars_.count(xey.second) == 0) {
      var_vars_.insert(xey.second);
    }
  }
  for (const auto& xnm : neq_name_) {
    assert(xnm.first.is_variable());
    assert(xnm.second.is_name());
    vars_.insert(xnm.first);
    if (fix_vars_.count(xnm.first) == 0) {
      var_vars_.insert(xnm.first);
    }
    names_.insert(xnm.second);
  }
  for (const auto& xny : neq_var_) {
    assert(xny.first.is_variable());
    assert(xny.second.is_variable());
    vars_.insert(xny.first);
    vars_.insert(xny.second);
    if (fix_vars_.count(xny.first) == 0) {
      var_vars_.insert(xny.first);
    }
    if (fix_vars_.count(xny.second) == 0) {
      var_vars_.insert(xny.second);
    }
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

#if 0
// Could be used to replace
//    (*theta)[x] = n;
// in GenerateModels, but perhaps it's not worth the cost because then we'd
// have to create a copy of theta in the loop.
// TODO(schwering) Should be tested.
bool Ewff::Conj::Assign(Assignment* theta, const Variable& x,
                        const StdName& n) const {
  assert(vars_.count(x) > 0);
  const auto p = theta->insert(std::make_pair(x, n));
  const StdName& n_old = p.first->second;
  if (!p.second) {
    return n == n_old;
  }
  bool r = true;
  for (auto yez = eq_var_.begin(); yez != eq_var_.end(); ++yez) {
    const Variable& y = yez->first;
    const Variable& z = yez->second;
    if (y == x) {
      r &= Assign(theta, y, n);
    } else if (z == x) {
      r &= Assign(theta, z, n);
    }
  }
  return r;
}
#endif

#if 0
// A simpler version of Substitute(). Will be deleted if Substitute() works
// well.
bool Ewff::Conj::Ground(const Variable& x, const StdName& n) {
  if (vars_.count(x) == 0) {
    return true;
  }

  const auto p = eq_name_.insert(std::make_pair(x, n));
  const StdName& n_old = p.first->second;
  if (!p.second) {
    const bool r = n == n_old;
    return r;
  }
  fix_vars_.insert(x);
  var_vars_.erase(x);
  names_.insert(n);

  std::list<Variable> recursive_calls;
  for (auto yez = eq_var_.begin(); yez != eq_var_.end(); ++yez) {
    const Variable& y = yez->first;
    const Variable& z = yez->second;
    if (y == x) {
      recursive_calls.push_front(z);
      eq_var_.erase(yez);
    } else if (z == x) {
      recursive_calls.push_front(y);
      eq_var_.erase(yez);
    }
  }
  for (const Variable& y : recursive_calls) {
    const bool r = Ground(y, n);
    if (!r) {
      return false;
    }
  }
  return true;
}
#endif

bool Ewff::Conj::Substitute(const Variable& x, const StdName& n) {
  if (vars_.count(x) == 0) {
    return true;
  }
  assert(vars_.count(x) > 0);

  const auto p = eq_name_.insert(std::make_pair(x, n));
  const StdName& n_old = p.first->second;
  if (!p.second) {
    return n == n_old;
  }

  fix_vars_.insert(x);
  var_vars_.erase(x);
  names_.insert(n);

  std::list<Variable> recursive_calls;
  for (auto yez = eq_var_.begin(); yez != eq_var_.end(); ++yez) {
    const Variable& y = yez->first;
    const Variable& z = yez->second;
    if (y == x) {
      recursive_calls.push_front(z);
      eq_var_.erase(yez);
    } else if (z == x) {
      recursive_calls.push_front(y);
      eq_var_.erase(yez);
    }
  }
  for (const Variable& y : recursive_calls) {
    const bool r = Substitute(y, n);
    if (!r) {
      return false;
    }
  }

  for (auto ynm = neq_name_.begin(); ynm != neq_name_.end(); ++ynm) {
    const Variable& y = ynm->first;
    const StdName& m = ynm->second;
    if (y == x) {
      if (m == n) {
        return false;
      }
      neq_name_.erase(ynm);
    }
  }

  for (auto ynz = neq_var_.begin(); ynz != neq_var_.end(); ++ynz) {
    const Variable& y = ynz->first;
    const Variable& z = ynz->second;
    if (y == x) {
      neq_name_.insert(std::make_pair(z, n));
      eq_var_.erase(ynz);
    } else if (z == x) {
      neq_name_.insert(std::make_pair(y, n));
      eq_var_.erase(ynz);
    }
  }
  return true;
}

bool Ewff::Conj::Substitute(const Variable& x, const Variable& y) {
  if (vars_.count(x) == 0) {
    return true;
  }
  assert(vars_.count(x) > 0);

  const auto& xem = eq_name_.find(x);
  const auto& yen = eq_name_.find(y);
  if (xem != eq_name_.end() && yen != eq_name_.end()) {
    const StdName& m = xem->second;
    const StdName& n = yen->second;
    return m == n;
  } else if (xem != eq_name_.end()) {
    const StdName& m = xem->second;
    return Substitute(y, m);
  } else if (yen != eq_name_.end()) {
    const StdName& n = yen->second;
    return Substitute(x, n);
  }

  eq_var_.insert(std::make_pair(x, y));
  var_vars_.insert(x);
  var_vars_.insert(y);
  return true;
}

std::pair<bool, Ewff::Conj> Ewff::Conj::Substitute(const Unifier& theta) const {
  std::pair<bool, Conj> r = std::make_pair(true, *this);
  for (const auto& xet : theta) {
    const Variable& x = xet.first;
    const Term& t = xet.second;
    if (t.is_name()) {
      if (!r.second.Substitute(x, StdName(t))) {
        r.first = false;
        return r;
      }
    } else {
      if (!r.second.Substitute(x, Variable(t))) {
        r.first = false;
        return r;
      }
    }
  }
  return r;
}

std::pair<bool, Ewff::Conj> Ewff::Conj::Ground(const Assignment& theta) const {
  std::pair<bool, Conj> r = std::make_pair(true, *this);
  for (const auto& xen : theta) {
    if (!r.second.Substitute(xen.first, xen.second)) {
      r.first = false;
      return r;
    }
  }
  return r;
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
    if (xen2 == end || xen1.second != xen2->second) {
      return false;
    }
  }
  return true;
}

void Ewff::Conj::GenerateModels(const std::set<Variable>::const_iterator first,
                                const std::set<Variable>::const_iterator last,
                                const StdName::SortedSet& hplus,
                                Assignment* theta,
                                std::list<Assignment>* models) const {
  if (first != last) {
    const Variable& x = *first;
    StdName::SortedSet::const_iterator ns_it = hplus.find(x.sort());
    assert(ns_it != hplus.end());
    if (ns_it == hplus.end()) {
      return;
    }
    const std::set<StdName>& ns = ns_it->second;
    for (const StdName& n : ns) {
      (*theta)[x] = n;
      GenerateModels(std::next(first), last, hplus, theta, models);
    }
    theta->erase(x);
  } else if (CheckModel(*theta)) {
    models->push_back(*theta);
  }
}

void Ewff::Conj::FindModels(const StdName::SortedSet& hplus,
                            std::list<Assignment>* models) const {
  Assignment theta = eq_name_;
  GenerateModels(var_vars_.begin(), var_vars_.end(), hplus, &theta, models);
}

const std::set<Variable>& Ewff::Conj::variables() const {
  return vars_;
}

const std::set<StdName>& Ewff::Conj::names() const {
  return names_;
}


const Ewff Ewff::TRUE = Ewff({ {}, {}, {}, {} });
const Ewff Ewff::FALSE = Ewff({});

std::pair<bool, Ewff> Ewff::Substitute(const Unifier& theta) const {
  std::pair<bool, Ewff> p;
  for (const auto& c1 : cs_) {
    const auto q = c1.Substitute(theta);
    if (q.first) {
      p.second.cs_.push_back(q.second);
    }
  }
  p.first = !p.second.cs_.empty();
  return p;
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
    if (c.CheckModel(theta)) {
      return true;
    }
  }
  return false;
}

std::list<Assignment> Ewff::FindModels(const StdName::SortedSet& hplus) const {
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
      os << " ^ ";
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = c.eq_var_.begin(); it != c.eq_var_.end(); ++it) {
    if (!c.eq_name_.empty() || it != c.eq_var_.begin()) {
      os << " ^ ";
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = c.neq_name_.begin(); it != c.neq_name_.end(); ++it) {
    if (!c.eq_var_.empty() || it != c.neq_name_.begin()) {
      os << " ^ ";
    }
    os << it->first << " != " << it->second;
  }
  for (auto it = c.neq_var_.begin(); it != c.neq_var_.end(); ++it) {
    if (!c.neq_name_.empty() || it != c.neq_var_.begin()) {
      os << " ^ ";
    }
    os << it->first << " != " << it->second;
  }
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Ewff& e) {
  os << '(';
  for (auto it = e.cs_.begin(); it != e.cs_.end(); ++it) {
    if (it != e.cs_.begin()) {
      os << " v ";
    }
    os << *it;
  }
  os << ')';
  return os;
}

