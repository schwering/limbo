// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./term.h"

namespace esbl {


Variable Term::Factory::CreateVariable(Term::Sort sort) {
  return Variable(Term(Term::VAR, var_counter_++, sort));
}

StdName Term::Factory::CreateStdName(Term::Id id, Term::Sort sort) {
  const StdName n(Term(Term::NAME, id, sort));
  names_[n.sort()].insert(n);
  return n;
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

Term Term::Ground(const Assignment& theta) const {
  if (kind_ == VAR) {
    auto it = theta.find(Variable(*this));
    return it != theta.end() ? it->second : *this;
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
    return Update(theta, Variable(tt1), tt2);
  } else if (tt2.is_variable()) {
    return Update(theta, Variable(tt2), tt1);
  } else {
    return tt1 == tt2;
  }
}

bool Term::UnifySeq(const TermSeq& z1, const TermSeq& z2, Unifier* theta) {
  for (auto i = z1.begin(), j = z2.begin();
       i != z1.end() && j != z2.end();
       ++i, ++j) {
    const Term& t1 = *i;
    const Term& t2 = *j;
    const bool r = Unify(t1, t2, theta);
    if (!r) {
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const TermSeq& z) {
  for (auto it = z.begin(); it != z.end(); ++it) {
    if (it != z.begin()) {
      os << ", ";
    }
    os << *it;
  }
  return os;
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

}  // namespace esbl

