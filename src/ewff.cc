// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./ewff.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <iostream>

namespace esbl {

const Ewff Ewff::TRUE = Ewff({}, {});

Ewff::Ewff(const std::set<std::pair<Variable, StdName>>& neq_name,
           const std::set<std::pair<Variable, Variable>>& neq_var)
    : neq_name_(neq_name), neq_var_(neq_var) {
  for (auto xny = neq_var_.begin(); xny != neq_var_.end(); ) {
    if (xny->second < xny->first) {
      neq_var_.insert(std::make_pair(xny->second, xny->first));
      xny = neq_var_.erase(xny);
    } else {
      ++xny;
    }
  }
}

std::pair<bool, Ewff> Ewff::Create(
    const std::set<std::pair<Variable, StdName>>& neq_name,
    const std::set<std::pair<Variable, Variable>>& neq_var) {
  for (const auto& xny : neq_var) {
    if (xny.first == xny.second) {
      return failed<Ewff>();
    }
  }
  return std::make_pair(true, Ewff(neq_name, neq_var));
}

Ewff Ewff::And(const Ewff& e1, const Ewff& e2) {
  Ewff e =
      (e1.neq_var_.size() + e2.neq_name_.size() >
       e2.neq_var_.size() + e2.neq_name_.size())
      ? e1 : e2;
  const Ewff& add = (&e == &e1) ? e2 : e1;
  e.neq_name_.insert(add.neq_name_.begin(), add.neq_name_.end());
  e.neq_var_.insert(add.neq_var_.begin(), add.neq_var_.end());
  return e;
}

bool Ewff::operator==(const Ewff& c) const {
  if (this == &c) {
    return true;
  }
  return neq_name_ == c.neq_name_ && neq_var_ == c.neq_var_;
}

bool Ewff::operator!=(const Ewff& c) const {
  return !operator==(c);
}

bool Ewff::operator<(const Ewff& c) const {
  return neq_name_ < c.neq_name_ ||
      (neq_name_ == c.neq_name_ && neq_var_ < c.neq_var_);
}

bool Ewff::SubstituteName(const Variable& x, const StdName& n) {
  if (neq_name_.find(std::make_pair(x, n)) != neq_name_.end()) {
    return false;
  }

  const auto first = neq_name_.lower_bound(std::make_pair(x, StdName::MIN));
  const auto last = neq_name_.upper_bound(std::make_pair(x, StdName::MAX));
  neq_name_.erase(first, last);

  for (auto unv = neq_var_.begin(); unv != neq_var_.end(); ) {
    const Variable& u = unv->first;
    const Variable& v = unv->second;
    if (u == x) {
      neq_name_.insert(std::make_pair(v, n));
      unv = neq_var_.erase(unv);
    } else if (v == x) {
      neq_name_.insert(std::make_pair(u, n));
      unv = neq_var_.erase(unv);
    } else {
      ++unv;
    }
  }
  return true;
}

bool Ewff::SubstituteVariable(const Variable& x, const Variable& y) {
  if (x == y) {
    return true;
  }
  if (neq_var_.find(std::make_pair(std::min(x, y), std::max(x, y))) !=
      neq_var_.end()) {
    return false;
  }

  const auto first = neq_name_.lower_bound(std::make_pair(x, StdName::MIN));
  const auto last = neq_name_.upper_bound(std::make_pair(x, StdName::MAX));
  for (auto znm = first; znm != last && znm->first == x; ) {
    assert(znm->first == x);
    const StdName& n = znm->second;
    neq_name_.insert(std::make_pair(y, n));
    znm = neq_name_.erase(znm);
  }

  for (auto unv = neq_var_.begin(); unv != neq_var_.end(); ) {
    const Variable& u = unv->first;
    const Variable& v = unv->second;
    if (u == x) {
      if (v == y) {
        return false;
      }
      neq_var_.insert(std::make_pair(std::min(v, y), std::max(v, y)));
      unv = neq_var_.erase(unv);
    } else if (v == x) {
      if (u == y) {
        return false;
      }
      neq_var_.insert(std::make_pair(std::min(u, y), std::max(u, y)));
      unv = neq_var_.erase(unv);
    } else {
      ++unv;
    }
  }
  return true;
}

std::pair<bool, Ewff> Ewff::Substitute(const Unifier& theta) const {
  Ewff e = *this;
  for (const auto& xet : theta) {
    const Variable& x = xet.first;
    const Term& t = xet.second;
    if (t.is_name()) {
      if (!e.SubstituteName(x, StdName(t))) {
        return failed<Ewff>();
      }
    } else {
      assert(t.is_variable());
      if (!e.SubstituteVariable(x, Variable(t))) {
        return failed<Ewff>();
      }
    }
  }
  return std::make_pair(true, e);
}

std::pair<bool, Ewff> Ewff::Ground(const Assignment& theta) const {
  Ewff e = *this;
  for (const auto& xen : theta) {
    if (!e.SubstituteName(xen.first, xen.second)) {
      return failed<Ewff>();
    }
  }
  return std::make_pair(true, e);
}

bool Ewff::Subsumes(const Ewff& e) const {
  return std::includes(neq_name_.begin(), neq_name_.end(),
                       e.neq_name_.begin(), e.neq_name_.end()) &&
      std::includes(neq_var_.begin(), neq_var_.end(),
                    e.neq_var_.begin(), e.neq_var_.end());
}

bool Ewff::SatisfiedBy(const Assignment& theta) const {
  const auto& end = theta.end();
  for (const auto& xnm : neq_name_) {
    const auto xen = theta.find(xnm.first);
    if (xen == end || xnm.second == xen->second) {
      return false;
    }
  }
  for (const auto& xny : neq_var_) {
    const auto xem = theta.find(xny.first);
    const auto yen = theta.find(xny.second);
    if (xem == end || yen == end || xem->second == yen->second) {
      return false;
    }
  }
  return true;
}

void Ewff::GenerateModels(const std::set<Variable>::const_iterator first,
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
  } else if (SatisfiedBy(*theta)) {
    models->push_back(*theta);
  }
}

std::list<Assignment> Ewff::Models(const StdName::SortedSet& hplus) const {
  std::list<Assignment> models;
  Assignment theta;
  std::set<Variable> vs = Variables();
  GenerateModels(vs.begin(), vs.end(), hplus, &theta, &models);
  return models;
}

std::set<Variable> Ewff::Variables() const {
  std::set<Variable> vs;
  for (const auto& xnn : neq_name_) {
    const Variable& x = xnn.first;
    vs.insert(x);
  }
  for (const auto& xny : neq_var_) {
    const Variable& x = xny.first;
    const Variable& y = xny.second;
    vs.insert(x);
    vs.insert(y);
  }
  return vs;
}

void Ewff::RestrictVariable(const std::set<Variable>& vs) {
  for (auto xnn = neq_name_.begin(); xnn != neq_name_.end(); ) {
    const Variable& x = xnn->first;
    if (vs.count(x) == 0) {
      xnn = neq_name_.erase(xnn);
    } else {
      ++xnn;
    }
  }
  for (auto xny = neq_var_.begin(); xny != neq_var_.end(); ) {
    const Variable& x = xny->first;
    const Variable& y = xny->second;
    if (vs.count(x) == 0 || vs.count(y) == 0) {
      xny = neq_var_.erase(xny);
    } else {
      ++xny;
    }
  }
}

void Ewff::CollectVariables(Variable::SortedSet* vs) const {
  for (const auto& xnn : neq_name_) {
    const Variable& x = xnn.first;
    (*vs)[x.sort()].insert(x);
  }
  for (const auto& xny : neq_var_) {
    const Variable& x = xny.first;
    const Variable& y = xny.second;
    (*vs)[x.sort()].insert(x);
    (*vs)[y.sort()].insert(y);
  }
}

void Ewff::CollectNames(StdName::SortedSet* ns) const {
  for (auto& xnn : neq_name_) {
    const StdName& n = xnn.second;
    (*ns)[n.sort()].insert(n);
  }
}

std::ostream& operator<<(std::ostream& os, const Ewff& e) {
  os << '(';
  for (auto it = e.neq_name_.begin(); it != e.neq_name_.end(); ++it) {
    if (it != e.neq_name_.begin()) {
      os << " ^ ";
    }
    os << it->first << " != " << it->second;
  }
  for (auto it = e.neq_var_.begin(); it != e.neq_var_.end(); ++it) {
    if (!e.neq_name_.empty() || it != e.neq_var_.begin()) {
      os << " ^ ";
    }
    os << it->first << " != " << it->second;
  }
  os << ')';
  return os;
}

}  // namespace esbl

