// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <cassert>
#include <set>
#include "./compar.h"
#include "./term.h"

namespace lela {

class Literal {
 public:
  struct Comparator;

  static Literal Eq(Term lhs, Term rhs) {
    return Literal(true, lhs, rhs);
  }

  static Literal Neq(Term lhs, Term rhs) {
    return Literal(false, lhs, rhs);
  }

  Term lhs() const { return lhs_; }
  Term rhs() const { return rhs_; }

  bool ground() const { return lhs_.ground() && rhs_.ground(); }
  bool primitive() const { return lhs_.primitive() && rhs_.primitive(); }

  Literal Flip() const { return Literal(!sign_, lhs_, rhs_); }
  Literal Dual() const { return Literal(sign_, rhs_, lhs_); }

  bool operator==(Literal l) const {
    return sign_ == l.sign_ && lhs_ == l.lhs_ && rhs_ == l.rhs_;
  }

  bool Valid() const { return sign_ && lhs_ == rhs_; }
  bool Invalid() const { return !sign_ && lhs_.name() && rhs_.name() && lhs_ != rhs_; }

  static bool Complementary(Literal a, Literal b) {
    return a.lhs_ == b.lhs_ &&
        ((a.sign_ != b.sign_ && a.rhs_ == b.rhs_) ||
         (a.sign_ && b.sign_ && a.rhs_.name() && b.rhs_.name() && a.rhs_ == b.rhs_));
  }

  static bool Subsumes(Literal a, Literal b) {
    return a.lhs_ == b.lhs_ &&
        ((a.sign_ == b.sign_ && a.rhs_ == b.rhs_) ||
         (a.sign_ && !b.sign_ && a.rhs_.name() && b.rhs_.name() && a.rhs_ != b.rhs_));
  }

  bool Subsumes(Literal b) {
    return Subsumes(*this, b);
  }

  Literal Substitute(Term pre, Term post) const {
    return Literal(sign_, lhs_.Substitute(pre, post), rhs_.Substitute(pre, post));
  }

  Literal Ground(const Term::Substitution& theta) const {
    return Literal(sign_, lhs_.Ground(theta), rhs_.Ground(theta));
  }

 private:
  Literal(bool sign, Term lhs, Term rhs)
      : sign_(sign),
        lhs_(lhs < rhs || (!lhs.name() && rhs.name()) ? lhs : rhs),
        rhs_(lhs < rhs || (!lhs.name() && rhs.name()) ? rhs : lhs) {
    assert(!lhs.null());
    assert(!rhs.null());
  }

  bool sign_;
  Term lhs_;
  Term rhs_;
};

struct Literal::Comparator {
  typedef Literal value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a.lhs_, a.rhs_, a.sign_,
                b.lhs_, b.rhs_, b.sign_);
  }

 private:
  LexicographicComparator<Term::Comparator,
                          Term::Comparator,
                          LessComparator<bool>> comp;
};

}  // namespace lela

#endif  // SRC_LITERAL_H_

