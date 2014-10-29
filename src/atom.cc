// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include "./atom.h"

namespace esbl {

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

Atom Atom::PrependActions(const TermSeq& z) const {
  Atom a = *this;
  a.z_.insert(a.z_.begin(), z.begin(), z.end());
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

bool Atom::Unify(const Atom& a, const Atom& b, Unifier* theta) {
  if (a.pred_ != b.pred_ ||
      a.z_.size() != b.z_.size() ||
      a.args_.size() != b.args_.size()) {
    return false;
  }
  if (!Term::UnifySeq(a.z_, b.z_, theta)) {
    return false;
  }
  if (!Term::UnifySeq(a.args_, b.args_, theta)) {
    return false;
  }
  assert(a.Substitute(*theta) == b.Substitute(*theta));
  return true;
}

std::pair<bool, Unifier> Atom::Unify(const Atom& a, const Atom& b) {
  Unifier theta;
  const bool succ = Unify(a, b, &theta);
  return std::make_pair(succ, theta);
}

Atom Atom::LowerBound() const {
  return Atom({}, pred_, {});
}

Atom Atom::UpperBound() const {
  return Atom({}, pred_ + 1, {});
}

bool Atom::is_ground() const {
  return std::all_of(z_.begin(), z_.end(),
                     [](const Term& t) { return t.is_ground(); }) &&
         std::all_of(args_.begin(), args_.end(),
                     [](const Term& t) { return t.is_ground(); });
}

void Atom::CollectVariables(Variable::SortedSet* vs) const {
  for (auto& t : z_) {
    if (t.is_variable()) {
      (*vs)[t.sort()].insert(Variable(t));
    }
  }
  for (auto& t : args_) {
    if (t.is_variable()) {
      (*vs)[t.sort()].insert(Variable(t));
    }
  }
}

void Atom::CollectNames(StdName::SortedSet* ns) const {
  for (auto& t : z_) {
    if (t.is_name()) {
      (*ns)[t.sort()].insert(StdName(t));
    }
  }
  for (auto& t : args_) {
    if (t.is_name()) {
      (*ns)[t.sort()].insert(StdName(t));
    }
  }
}

std::ostream& operator<<(std::ostream& os, const Atom& a) {
  os << '[' << a.z() << ']' << 'P' << a.pred() << '(' << a.args() << ')';
  return os;
}

}  // namespace esbl

