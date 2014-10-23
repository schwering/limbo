// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./term.h"

Term::Id Term::var_id_ = 0;

Term::Term()
  : type_(DUMMY), id_(0) {
}

Term::Term(Type type, int id, Sort sort) : type_(type), id_(id), sort_(sort) {
}

Term::Var Term::CreateVariable(Sort sort) {
  return Var(Term(VAR, var_id_++, sort));
}

Term::Name Term::CreateStdName(Id id, Sort sort) {
  return Name(Term(NAME, id, sort));
}

bool Term::operator==(const Term& t) const {
  return type_ == t.type_ && id_ == t.id_ && sort_ == t.sort_;
}

bool Term::operator!=(const Term& t) const {
  return !(*this == t);
}

bool Term::operator<(const Term& t) const {
  return type_ < t.type_ || (type_ == t.type_ && id_ < t.id_);
}

const Term& Term::Substitute(const Unifier& theta) const {
  if (type_ == VAR) {
    auto it = theta.find(Var(*this));
    return it != theta.end() ? it->second : *this;
  } else {
    return *this;
  }
}

