// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./term.h"

Term::Id Term::var_id_ = 0;

Term::Term()
  : kind_(DUMMY), id_(0) {
}

Term::Term(Kind kind, int id, Sort sort) : kind_(kind), id_(id), sort_(sort) {
}

Variable Term::CreateVariable(Sort sort) {
  return Variable(Term(VAR, var_id_++, sort));
}

StdName Term::CreateStdName(Id id, Sort sort) {
  return StdName(Term(NAME, id, sort));
}

bool Term::operator==(const Term& t) const {
  return kind_ == t.kind_ && id_ == t.id_ && sort_ == t.sort_;
}

bool Term::operator!=(const Term& t) const {
  return !(*this == t);
}

bool Term::operator<(const Term& t) const {
  return kind_ < t.kind_ || (kind_ == t.kind_ && id_ < t.id_) ||
      (kind_ == t.kind_ && id_ == t.id_ && sort_ < t.sort_);
}

const Term& Term::Substitute(const Unifier& theta) const {
  if (kind_ == VAR) {
    auto it = theta.find(Variable(*this));
    return it != theta.end() ? it->second.Substitute(theta) : *this;
  } else {
    return *this;
  }
}

namespace {
inline bool Update(Unifier* theta, const Variable& x, const Term& t) {
  if (x == t) {
    return true;
  }
  auto p = theta->insert(std::make_pair(x, t));
  if (p.second) {
    return true;
  }
  const Term t_old = p.first->second;
  if (t.is_ground() && t_old.is_ground()) {
    return false;
  }
  return Update(theta, Variable(t_old), t);
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

std::ostream& operator<<(std::ostream& os, const Term& t) {
  char c;
  if (t.is_variable()) {
    c = 'V';
  } else if (t.is_name()) {
    c = '#';
  } else {
    c = '?';
  }
  return os << c << t.id() << ':' << t.sort();
}

std::ostream& operator<<(std::ostream& os, const Unifier& theta) {
  os << '{';
  for (auto it = theta.begin(); it != theta.end(); ++it) {
    if (it != theta.begin()) {
      os << ',' << ' ';
    }
    os << it->first << " : " << it->second;
  }
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Assignment& theta) {
  os << '{';
  for (auto it = theta.begin(); it != theta.end(); ++it) {
    if (it != theta.begin()) {
      os << ',' << ' ';
    }
    os << it->first << " : " << it->second;
  }
  os << '}';
  return os;
}

