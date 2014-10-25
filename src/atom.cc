// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include "./atom.h"

const Atom::PredId Atom::SF = -1;

Atom Atom::PrependActions(const TermSeq& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.begin(), z.begin(), z.end());
  return a;
}

Atom Atom::AppendActions(const TermSeq& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.end(), z.begin(), z.end());
  return a;
}

Atom Atom::DropActions(size_t n) const {
  Atom a = *this;
  a.z_.erase(a.z_.begin(), a.z_.begin() + n);
  return a;
}

Atom Atom::Substitute(const Unifier& theta) const {
  Atom a = *this;
  for (Term& t : a.z_) {
    t = t.Substitute(theta);
  }
  for (Term& t : a.args_) {
    t = t.Substitute(theta);
  }
  return a;
}

Atom Atom::Ground(const Assignment& theta) const {
  Atom a = *this;
  for (Term& t : a.z_) {
    t = t.Ground(theta);
  }
  for (Term& t : a.args_) {
    t = t.Ground(theta);
  }
  return a;
}

namespace {
bool Unify(const TermSeq& a, const TermSeq& b, Unifier* theta) {
  for (auto i = a.begin(), j = b.begin();
       i != a.end() && j != b.end();
       ++i, ++j) {
    const Term& t1 = *i;
    const Term& t2 = *j;
    const bool r = Term::Unify(t1, t2, theta);
    if (!r) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool Atom::Unify(const Atom& a, const Atom& b, Unifier* theta) {
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

std::pair<bool, Unifier> Atom::Unify(const Atom& a, const Atom& b) {
  std::pair<bool, Unifier> p;
  p.first = Unify(a, b, &p.second);
  return p;
}

bool Atom::operator==(const Atom& a) const {
  return pred_ == a.pred_ && z_ == a.z_ && args_ == a.args_;
}

bool Atom::operator!=(const Atom& a) const {
  return !operator==(a);
}

bool Atom::operator<(const Atom& a) const {
  return pred_ < a.pred_ ||
      (pred_ == a.pred_ && z_ < a.z_) ||
      (pred_ == a.pred_ && z_ == a.z_ && args_ < a.args_);
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

std::ostream& operator<<(std::ostream& os, const Atom& a) {
  os << '[';
  for (auto it = a.z().begin(); it != a.z().end(); ++it) {
    if (it != a.z().begin()) {
      os << ", ";
    }
    os << *it;
  }
  os << ']';
  os << 'P' << a.pred();
  os << '(';
  for (auto it = a.z().begin(); it != a.z().end(); ++it) {
    if (it != a.z().begin()) {
      os << ", ";
    }
    os << *it;
  }
  os << ')';
  return os;
}

