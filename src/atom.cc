// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include "./atom.h"

const Atom::PredId Atom::SF = -1;

Atom::Atom(const std::vector<Term>& z, PredId id, const std::vector<Term>& args)
  : z_(z), args_(args) {
}

Atom Atom::PrependAction(const Term& t) const {
  Atom a = *this;
  a.z_.insert(a.z_.begin(), t);
  return a;
}

Atom Atom::PrependActions(const std::vector<Term>& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.begin(), z_.begin(), z_.end());
  return a;
}

Atom Atom::AppendAction(const Term& t) const {
  Atom a = *this;
  a.z_.push_back(t);
  return a;
}

Atom Atom::AppendActions(const std::vector<Term>& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.end(), z_.begin(), z_.end());
  return a;
}

Atom Atom::Substitute(const Unifier theta) const {
  Atom a = *this;
  for (Term& t : a.z_) {
    t = t.Substitute(theta);
  }
  for (Term& t : a.args_) {
    t = t.Substitute(theta);
  }
  return a;
}

bool Atom::Unify(const Atom& a, Unifier theta) const {
  assert(is_ground());
  return Unify(*this, a, theta);
}

namespace {
bool Unify(const std::vector<Term>& a, const std::vector<Term>& b,
           Unifier theta) {
  for (auto i = a.begin(), j = b.begin();
       i != a.end() && j != b.end();
       ++i, ++j) {
    const Term& t1 = *i;
    const Term& t2 = *j;
    if (!t1.is_variable() && !t2.is_variable()) {
      return false;
    } else if (t1.is_variable()) {
      theta[Variable(t1)] = t2;
    } else if (t2.is_variable()) {
      theta[Variable(t2)] = t1;
    }
  }
}
}  // namespace

bool Atom::Unify(const Atom& a, const Atom& b, Unifier theta) {
  if (a.pred_ != b.pred_ ||
      a.z_.size() != b.z_.size() ||
      a.args_.size() != b.args_.size()) {
    return false;
  }
  if (!::Unify(a.z_, b.z_, theta)) {
    return false;
  }
  if (!::Unify(a.args_, b.args_, theta)) {
    return false;
  }
  return true;
}

bool Atom::operator==(const Atom& a) const {
  return pred_ == a.pred_ && z_ == a.z_ && args_ == a.args_;
}

bool Atom::operator<(const Atom& a) const {
  return pred_ < a.pred_ && z_ < a.z_ && args_ < a.args_;
}

bool Atom::is_ground() const {
  return std::all_of(z_.begin(), z_.end(),
                     [](const Term& t) { return t.is_ground(); });
}

std::set<Variable> Atom::variables() const {
  std::set<Variable> vs;
  for (auto& t : z_) {
    if (t.is_variable()) {
      vs.insert(Variable(t));
    }
  }
  for (auto& t : args_) {
    if (t.is_variable()) {
      vs.insert(Variable(t));
    }
  }
  return vs;
}

std::set<StdName> Atom::names() const {
  std::set<StdName> ns;
  for (auto& t : z_) {
    if (t.is_name()) {
      ns.insert(StdName(t));
    }
  }
  for (auto& t : args_) {
    if (t.is_name()) {
      ns.insert(StdName(t));
    }
  }
  return ns;
}

