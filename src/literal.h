// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <set>
#include "./term.h"

namespace lela {

class Literal {
 public:
  static Literal Eq(const Term* lhs, const Term* rhs) {
    return Literal(true, lhs, rhs);
  }

  static Literal Neq(const Term* lhs, const Term* rhs) {
    return Literal(false, lhs, rhs);
  }

  const Term* lhs() const { return lhs_; }
  const Term* rhs() const { return rhs_; }

  Literal flip() const { return Literal(!sign_, lhs_, rhs_); }
  Literal dual() const { return Literal(sign_, rhs_, lhs_); }

  bool operator==(const Literal& l) const {
    return sign_ == l.sign_ && lhs_ == l.lhs_ && rhs_ == l.rhs_;
  }

  bool Valid() const { return sign_ && lhs_ == rhs_; }
  bool Invalid() const { return !sign_ && lhs_->name() && rhs_->name() && lhs_ != rhs_; }

  static bool Complementary(const Literal& a, const Literal& b) {
    return a.lhs_ == b.lhs_ &&
        ((a.sign_ != b.sign_ && a.rhs_ == b.rhs_) ||
         (a.sign_ && b.sign_ && a.rhs_->name() && b.rhs_->name() && a.rhs_ == b.rhs_));
  }

  static bool Subsumes(const Literal& a, const Literal& b) {
    return a.lhs_ == b.lhs_ &&
        ((a.sign_ == b.sign_ && a.rhs_ == b.rhs_) ||
         (a.sign_ && !b.sign_ && a.rhs_->name() && b.rhs_->name() && a.rhs_ != b.rhs_));
  }

 private:
  Literal(bool sign, const Term* lhs, const Term* rhs)
      : sign_(sign),
        lhs_(lhs < rhs || (!lhs->name() && rhs->name()) ? lhs : rhs),
        rhs_(lhs < rhs || (!lhs->name() && rhs->name()) ? rhs : lhs) {}

  bool sign_;
  const Term* lhs_;
  const Term* rhs_;
};

}  // namespace lela

#endif  // SRC_LITERAL_H_

