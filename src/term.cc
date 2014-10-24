// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./term.h"

Term::Id Term::var_id_ = 0;

Term::Term()
  : type_(DUMMY), id_(0) {
}

Term::Term(Type type, int id, Sort sort) : type_(type), id_(id), sort_(sort) {
}

Variable Term::CreateVariable(Sort sort) {
  return Variable(Term(VAR, var_id_++, sort));
}

StdName Term::CreateStdName(Id id, Sort sort) {
  return StdName(Term(NAME, id, sort));
}

bool Term::operator==(const Term& t) const {
  return type_ == t.type_ && id_ == t.id_ && sort_ == t.sort_;
}

bool Term::operator!=(const Term& t) const {
  return !(*this == t);
}

bool Term::operator<(const Term& t) const {
  return type_ < t.type_ || (type_ == t.type_ && id_ < t.id_) ||
      (type_ == t.type_ && id_ == t.id_ && sort_ < t.sort_);
}

const Term& Term::Substitute(const Unifier& theta) const {
  if (type_ == VAR) {
    auto it = theta.find(Variable(*this));
    return it != theta.end() ? it->second.Substitute(theta) : *this;
  } else {
    return *this;
  }
}

namespace {
inline bool Update(Unifier* theta, const Variable& t1, const Term& t2) {
  auto p = theta->insert(std::make_pair(t1, t2));
  if (p.second) {
    return true;
  }
  std::pair<const Variable, Term>& old = *p.first;
  assert(old.first == t1);
  if (t2.is_ground() && old.second.is_ground()) {
    return false;
  }
  return Update(theta, Variable(old.second), t2);
}
}  // namespace

bool Term::Unify(const Term& t1, const Term& t2, Unifier* theta) {
  const Term tt1 = t1.Substitute(*theta);
  const Term tt2 = t2.Substitute(*theta);
  if (tt1.is_variable()) {
    return ::Update(theta, Variable(tt1), tt2);
  } else if (tt2.is_variable()) {
    return ::Update(theta, Variable(tt2), tt1);
  } else {
    return tt1 == tt2;
  }
}

