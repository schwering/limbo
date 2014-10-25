// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./literal.h"

Literal Literal::Positive(const Atom& a) {
  return Literal(true, a);
}

Literal Literal::Negative(const Atom& a) {
  return Literal(false, a);
}

Literal Literal::Flip() const {
  Literal l = *this;
  l.sign_ = !l.sign_;
  return l;
}

Literal Literal::Positive() const {
  Literal l = *this;
  l.sign_ = true;
  return l;
}

Literal Literal::Negative() const {
  Literal l = *this;
  l.sign_ = false;
  return l;
}

bool Literal::operator==(const Literal& l) const {
  return Atom::operator==(l) && sign_ == l.sign_;
}

bool Literal::operator!=(const Literal& l) const {
  return !operator==(l);
}

bool Literal::operator<(const Literal& l) const {
  return Atom::operator<(l) || (Atom::operator==(l) && sign_ < l.sign_);
}

std::ostream& operator<<(std::ostream& os, const Literal& l) {
  if (!l.sign()) {
    os << '~';
  }
  os << static_cast<const Atom&>(l);
  return os;
}

