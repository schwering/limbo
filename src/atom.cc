// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <limits>
#include "./atom.h"

namespace esbl {

const Atom Atom::MIN({}, std::numeric_limits<PredId>::min(), {});
const Atom Atom::MAX({}, std::numeric_limits<PredId>::max(), {});

bool Atom::operator==(const Atom& a) const {
  return pred_ == a.pred_ && z_ == a.z_ && args_ == a.args_;
}

bool Atom::operator<(const Atom& a) const {
  return std::tie(pred_, z_, args_) < std::tie(a.pred_, a.z_, a.args_);
}

Atom Atom::PrependActions(const TermSeq& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.begin(), z.begin(), z.end());
  return a;
}

Atom Atom::Substitute(const Unifier& theta) const {
  const TermSeq z = z_.Substitute(theta);
  const TermSeq args = args_.Substitute(theta);
  return Atom(z, pred_, args);
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

bool Atom::Matches(const Atom& a, Unifier* theta) const {
  if (pred_ != a.pred_ ||
      z_.size() != a.z_.size() ||
      args_.size() != a.args_.size()) {
    return false;
  }
  if (!z_.Matches(a.z_, theta)) {
    return false;
  }
  if (!args_.Matches(a.args_, theta)) {
    return false;
  }
  return true;
}

bool Atom::Unify(const Atom& a, const Atom& b, Unifier* theta) {
  if (a.pred_ != b.pred_ ||
      a.z_.size() != b.z_.size() ||
      a.args_.size() != b.args_.size()) {
    return false;
  }
  if (!TermSeq::Unify(a.z_, b.z_, theta)) {
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

Atom Atom::LowerBound() const {
  return Atom({}, pred_, {});
}

Atom Atom::UpperBound() const {
  assert(pred_ < std::numeric_limits<PredId>::max());
  return Atom({}, pred_ + 1, {});
}

bool Atom::is_ground() const {
  return std::all_of(z_.begin(), z_.end(),
                     [](const Term& t) { return t.is_ground(); }) &&
         std::all_of(args_.begin(), args_.end(),
                     [](const Term& t) { return t.is_ground(); });
}

void Atom::CollectVariables(Variable::Set* vs) const {
  z_.CollectVariables(vs);
  args_.CollectVariables(vs);
}

void Atom::CollectVariables(Variable::SortedSet* vs) const {
  z_.CollectVariables(vs);
  args_.CollectVariables(vs);
}

void Atom::CollectNames(StdName::SortedSet* ns) const {
  z_.CollectNames(ns);
  args_.CollectNames(ns);
}

std::ostream& operator<<(std::ostream& os, const Atom& a) {
  if (!a.z().empty()) {
    os << '[' << a.z() << ']';
  }
  os << 'P' << a.pred();
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

}  // namespace esbl

