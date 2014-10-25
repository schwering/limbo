// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <vector>
#include "./atom.h"

class Literal : public Atom {
 public:
  Literal(bool sign, const Atom& a) : Atom(a), sign_(sign) {}
  Literal(const TermSeq& z, bool sign, PredId pred, const TermSeq& args)
      : Atom(z, pred, args), sign_(sign) {}
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  static Literal Positive(const Atom& a);
  static Literal Negative(const Atom& a);

  Literal Flip() const;
  Literal Positive() const;
  Literal Negative() const;

  Literal PrependActions(const TermSeq& z) const {
    return Literal(sign(), PrependActions(z));
  }
  Literal AppendActions(const TermSeq& z) const {
    return Literal(sign(), AppendActions(z));
  }
  Literal DropActions(size_t n) const {
    return Literal(sign(), DropActions(n));
  }

  Literal Substitute(const Unifier& theta) const {
    return Literal(sign(), Substitute(theta));
  }
  Literal Ground(const Assignment& theta) const {
    return Literal(sign(), Ground(theta));
  }

  bool operator==(const Literal& l) const;
  bool operator!=(const Literal& l) const;
  bool operator<(const Literal& l) const;

  bool sign() const { return sign_; }

 private:
  bool sign_;
};

std::ostream& operator<<(std::ostream& os, const Literal& l);

#endif  // SRC_LITERAL_H_

