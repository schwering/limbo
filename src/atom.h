// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_ATOM_H_
#define SRC_ATOM_H_

#include <map>
#include <set>
#include <vector>
#include "./term.h"

class Atom {
 public:
  typedef int PredId;

  static const PredId SF;

  Atom(const std::vector<Term>& z, PredId pred, const std::vector<Term>& args);
  Atom(const Atom&) = default;
  Atom& operator=(const Atom&) = default;

  Atom PrependAction(const Term& t) const;
  Atom AppendAction(const Term& t) const;
  Atom PrependActions(const std::vector<Term>& z) const;
  Atom AppendActions(const std::vector<Term>& z) const;

  Atom Substitute(const Unifier& theta) const;
  bool Unify(const Atom& a, Unifier* theta) const;
  std::pair<bool, Unifier> Unify(const Atom& a) const;

  static bool Unify(const Atom& a, const Atom& b, Unifier* theta);
  static std::pair<bool, Unifier> Unify(const Atom& a, const Atom& b);

  bool operator==(const Atom& a) const;
  bool operator<(const Atom& a) const;

  const std::vector<Term>& z() const { return z_; }
  PredId pred() const { return pred_; }
  const std::vector<Term>& args() const { return args_; }

  bool is_ground() const;
  std::set<Variable> variables() const;
  std::set<StdName> names() const;

 private:
  std::vector<Term> z_;
  PredId pred_;
  std::vector<Term> args_;
};

#endif  // SRC_ATOM_H_

