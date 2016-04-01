// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 Christoph Schwering

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
  bool pos() const { return eq_; }
  Term rhs() const { return rhs_; }

  bool ground() const { return lhs_.ground() && rhs_.ground(); }
  bool primitive() const { return lhs_.primitive() && rhs_.name(); }

  Literal Flip() const { return Literal(!eq_, lhs_, rhs_); }
  Literal Dual() const { return Literal(eq_, rhs_, lhs_); }

  bool operator==(Literal l) const { return eq_ == l.eq_ && lhs_ == l.lhs_ && rhs_ == l.rhs_; }

  // valid() holds for (t = t) and (n1 != n2).
  bool valid() const { return (eq_ && lhs_ == rhs_) || (!eq_ && lhs_.name() && rhs_.name() && lhs_ != rhs_); }
  //
  // invalid() holds for (t != t) and (n1 = n2).
  bool invalid() const { return (!eq_ && lhs_ == rhs_) || (eq_ && lhs_.name() && rhs_.name() && lhs_ != rhs_); }

  // Complementary(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 != t2)
  // (t1 != t2), (t1 = t2)
  // (t = n1), (t = n2) for distinct n1, n2.
  static bool Complementary(Literal a, Literal b) {
    return a.lhs_ == b.lhs_ &&
        ((a.eq_ != b.eq_ && a.rhs_ == b.rhs_) ||
         (a.eq_ && b.eq_ && a.rhs_.name() && b.rhs_.name() && a.rhs_ != b.rhs_));
  }

  // Subsumes(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 = t2)
  // (t1 = n1), (t1 != n2) for distinct n1, n2.
  static bool Subsumes(Literal a, Literal b) {
    return a.lhs_ == b.lhs_ &&
        ((a.eq_ == b.eq_ && a.rhs_ == b.rhs_) ||
         (a.eq_ && !b.eq_ && a.rhs_.name() && b.rhs_.name() && a.rhs_ != b.rhs_));
  }

  bool Subsumes(Literal b) const {
    return Subsumes(*this, b);
  }

  Literal Substitute(Term pre, Term post) const {
    return Literal(eq_, lhs_.Substitute(pre, post), rhs_.Substitute(pre, post));
  }

  Literal Ground(const Term::Substitution& theta) const {
    return Literal(eq_, lhs_.Ground(theta), rhs_.Ground(theta));
  }

  template<class UnaryPredicate>
  void CollectTerms(UnaryPredicate p, Term::Set* ts) const {
    lhs_.CollectTerms(p, ts);
    rhs_.CollectTerms(p, ts);
  }

 private:
  Literal(bool sign, Term lhs, Term rhs)
      : eq_(sign),
        lhs_(lhs < rhs || (!lhs.name() && rhs.name()) ? lhs : rhs),
        rhs_(lhs < rhs || (!lhs.name() && rhs.name()) ? rhs : lhs) {
    assert(!lhs.null());
    assert(!rhs.null());
  }

  bool eq_;
  Term lhs_;
  Term rhs_;
};

struct Literal::Comparator {
  typedef Literal value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a.lhs_, a.rhs_, a.eq_,
                b.lhs_, b.rhs_, b.eq_);
  }

 private:
  LexicographicComparator<Term::Comparator,
                          Term::Comparator,
                          LessComparator<bool>> comp;
};

}  // namespace lela

#endif  // SRC_LITERAL_H_

