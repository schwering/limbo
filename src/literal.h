// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#ifndef _LITERAL_H_
#define _LITERAL_H_

#include "atom.h"

class Literal : public Atom {
 public:
  Literal(const std::vector<Term>& z, bool sign, PredId pred,
          const std::vector<Term>& args);
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  Literal Flip() const;
  Literal Positive() const;
  Literal Negative() const;

  bool operator==(const Literal& l) const;
  bool operator<(const Literal& l) const;

  bool sign() const { return sign_; }

 private:
  bool sign_;
};

#endif

