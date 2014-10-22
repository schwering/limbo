// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#include "term.h"

Term::Term()
  : type_(DUMMY), id_(0) {
}

Term::Term(Type type, int id) : type_(type), id_(id) {
}

Term Term::CreateVariable(VarId id) {
  return Term(VAR, id);
}

Term Term::CreateStdName(NameId id) {
  return Term(NAME, id);
}

bool Term::operator==(const Term& t) const {
  return type_ == t.type_ && id_ == t.id_;
}

bool Term::operator<(const Term& t) const {
  return type_ < t.type_ || (type_ == t.type_ && id_ < t.id_);
}

const Term &Term::Substitute(const std::map<Term,Term>& theta) const {
  auto it = theta.find(*this);
  return it != theta.end() ? it->second : *this;
}

