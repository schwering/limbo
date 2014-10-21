// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#include "literal.h"

Literal::Literal(const std::vector<Term>& z, bool sign, PredId id,
                 const std::vector<Term>& args)
  : Atom(z, id, args), sign_(sign) {
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

bool Literal::operator<(const Literal& l) const {
  return Atom::operator<(l) && sign_ < l.sign_;
}

