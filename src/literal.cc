// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./literal.h"

namespace esbl {

const Literal Literal::MIN(false, Atom::MIN);
const Literal Literal::MAX(true, Atom::MAX);

bool Literal::operator==(const Literal& l) const {
  return Atom::operator==(l) && sign_ == l.sign_;
}

bool Literal::operator!=(const Literal& l) const {
  return !operator==(l);
}

bool Literal::operator<(const Literal& l) const {
  return Atom::operator<(l) || (Atom::operator==(l) && sign_ < l.sign_);
}

Literal Literal::LowerBound() const {
  return Literal(false, Atom::LowerBound());
}

Literal Literal::UpperBound() const {
  return Literal(false, Atom::UpperBound());
}

std::ostream& operator<<(std::ostream& os, const Literal& l) {
  if (!l.sign()) {
    os << '~';
  }
  os << static_cast<const Atom&>(l);
  return os;
}

std::ostream& operator<<(std::ostream& os, const std::set<Literal>& ls) {
  os << "{ ";
  for (auto it = ls.begin(); it != ls.end(); ++it) {
    if (it != ls.begin()) {
      os << ", ";
    }
    os << *it;
  }
  os << " }";
  return os;
}

}  // namespace esbl

