// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <limits>
#include "./atom.h"

namespace lela {

Atom Atom::Substitute(const Unifier& theta) const {
  const TermSeq args = args_.Substitute(theta);
  return Atom(pred_, args);
}

Atom Atom::Ground(const Assignment& theta) const {
  Atom a = *this;
  for (Term& t : a.args_) {
    t = t.Ground(theta);
  }
  return a;
}

bool Atom::Matches(const Atom& a, Unifier* theta) const {
  if (pred_ != a.pred_ ||
      args_.size() != a.args_.size()) {
    return false;
  }
  if (!args_.Matches(a.args_, theta)) {
    return false;
  }
  return true;
}

bool Atom::Unify(const Atom& a, const Atom& b, Unifier* theta) {
  if (a.pred_ != b.pred_ ||
      a.args_.size() != b.args_.size()) {
    return false;
  }
  if (!TermSeq::Unify(a.args_, b.args_, theta)) {
    return false;
  }
  assert(a.Substitute(*theta) == b.Substitute(*theta));
  return true;
}

Maybe<Unifier> Atom::Unify(const Atom& a, const Atom& b) {
  Unifier theta;
  const bool succ = Unify(a, b, &theta);
  return Perhaps(succ, theta);
}

bool Atom::ground() const {
  return std::all_of(args_.begin(), args_.end(),
                     [](const Term& t) { return t.ground(); });
}

void Atom::CollectVariables(Variable::Set* vs) const {
  args_.CollectVariables(vs);
}

void Atom::CollectVariables(Variable::SortedSet* vs) const {
  args_.CollectVariables(vs);
}

void Atom::CollectNames(StdName::SortedSet* ns) const {
  args_.CollectNames(ns);
}

std::ostream& operator<<(std::ostream& os, const Atom& a) {
  if (a.pred() == Atom::SF) {
    os << "SF";
  } else {
    os << 'P' << a.pred();
  }
  if (!a.args().empty()) {
    os << '(' << a.args() << ')';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Atom::Set& as) {
  os << "{ ";
  for (auto it = as.begin(); it != as.end(); ++it) {
    if (it != as.begin()) {
      os << ", ";
    }
    os << *it;
  }
  os << " }";
  return os;
}

}  // namespace lela

