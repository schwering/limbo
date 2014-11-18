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
  struct Comparator;
  typedef std::set<Literal, Comparator> Set;

  static const Literal MIN;
  static const Literal MAX;

  Literal(bool sign, const Atom& a) : Atom(a), sign_(sign) {}
  Literal(const TermSeq& z, bool sign, PredId pred, const TermSeq& args)
      : Atom(z, pred, args), sign_(sign) {}
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  bool operator==(const Literal& l) const {
    return Atom::operator==(l) && sign_ == l.sign_;
  }

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

  Literal LowerBound() const {
    return Literal(false, Atom::LowerBound());
  }
  Literal UpperBound() const {
    return Literal(false, Atom::UpperBound());
  }

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

struct Literal::Comparator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return atom_comp(l1, l2) || (!atom_comp(l2, l1) && l1.sign_ < l2.sign_);
  }

  Literal LowerBound(const PredId& p) const {
    return Literal({}, false, p, {});
  }

  Literal UpperBound(const PredId& p) const {
    return Literal({}, false, p + 1, {});
  }

 private:
  Atom::Comparator atom_comp;
};

std::ostream& operator<<(std::ostream& os, const Literal& l);
std::ostream& operator<<(std::ostream& os, const Literal::Set& ls);

}  // namespace esbl

#endif  // SRC_LITERAL_H_

