// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#ifndef _ATOM_H_
#define _ATOM_H_

#include "term.h"
#include <map>
#include <set>
#include <vector>

class Atom {
 public:
  typedef int PredId;

  static const PredId SF;

  Atom(const std::vector<Term>& z, PredId pred, const std::vector<Term>& args);

  Atom PrependAction(const Term& t) const;
  Atom AppendAction(const Term& t) const;
  Atom PrependActions(const std::vector<Term>& z) const;
  Atom AppendActions(const std::vector<Term>& z) const;

  Atom Substitute(const std::map<Term,Term> theta) const;

  static bool Unify(const Atom& a, const Atom& b, std::map<Term,Term> theta);

  bool operator==(const Atom& a) const;
  bool operator<(const Atom& a) const;

  const std::vector<Term>& z() const { return z_; }
  const PredId pred() const { return pred_; }
  const std::vector<Term>& args() const { return args_; }

  bool is_ground() const;
  std::set<Term> variables() ;
  std::set<Term> names() const;

 private:
  std::vector<Term> z_;
  PredId pred_;
  std::vector<Term> args_;
};

#endif


