// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./literal.h"

namespace esbl {

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
  return Literal(sign_, Atom::LowerBound());
}

Literal Literal::UpperBound() const {
  return Literal(sign_, Atom::UpperBound());
}

std::ostream& operator<<(std::ostream& os, const Literal& l) {
  if (!l.sign()) {
    os << '~';
  }
  os << static_cast<const Atom&>(l);
  return os;
}

}  // namespace esbl

