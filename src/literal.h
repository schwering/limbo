// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <set>
#include <vector>
#include "./atom.h"

namespace esbl {

class Literal : public Atom {
 public:
  static const Literal MIN;
  static const Literal MAX;

  Literal(bool sign, const Atom& a) : Atom(a), sign_(sign) {}
  Literal(const TermSeq& z, bool sign, PredId pred, const TermSeq& args)
      : Atom(z, pred, args), sign_(sign) {}
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  bool operator==(const Literal& l) const;
  bool operator!=(const Literal& l) const;
  bool operator<(const Literal& l) const;

  static Literal Positive(const Atom& a) { return Literal(true, a); }
  static Literal Negative(const Atom& a) { return Literal(false, a); }

  Literal Flip() const { return Literal(!sign(), *this); }
  Literal Positive() const { return Literal(true, *this); }
  Literal Negative() const { return Literal(false, *this); }

  Literal PrependActions(const TermSeq& z) const {
    return Literal(sign(), Atom::PrependActions(z));
  }

  Literal Substitute(const Unifier& theta) const {
    return Literal(sign(), Atom::Substitute(theta));
  }

  Literal Ground(const Assignment& theta) const {
    return Literal(sign(), Atom::Ground(theta));
  }

  Literal LowerBound() const;
  Literal UpperBound() const;

  bool sign() const { return sign_; }

 private:
  bool sign_;
};

class SfLiteral : public Literal {
 public:
  SfLiteral(const TermSeq& z, const Term t, bool r)
      : Literal(z, r, Atom::SF, {t}) {}

  SfLiteral(const SfLiteral&) = default;
  SfLiteral& operator=(const SfLiteral&) = default;
};

std::ostream& operator<<(std::ostream& os, const Literal& l);
std::ostream& operator<<(std::ostream& os, const std::set<Literal>& ls);

}  // namespace esbl

#endif  // SRC_LITERAL_H_

