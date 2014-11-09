// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_ATOM_H_
#define SRC_ATOM_H_

#include <map>
#include <set>
#include <utility>
#include <vector>
#include "./term.h"

namespace esbl {

class Atom {
 public:
  typedef int PredId;

  static constexpr PredId SF = -1;
  static constexpr PredId POSS = -2;

  static const Atom MIN;
  static const Atom MAX;

  Atom(const TermSeq& z, PredId pred, const TermSeq& args)
      : z_(z), pred_(pred), args_(args) {}
  Atom(const Atom&) = default;
  Atom& operator=(const Atom&) = default;

  bool operator==(const Atom& a) const;
  bool operator!=(const Atom& a) const;
  bool operator<(const Atom& a) const;

  Atom PrependActions(const TermSeq& z) const;
  Atom DropActions(size_t n) const;

  Atom Substitute(const Unifier& theta) const;
  Atom Ground(const Assignment& theta) const;
  bool Matches(const Atom& a, Unifier* theta) const;
  static bool Unify(const Atom& a, const Atom& b, Unifier* theta);
  static std::pair<bool, Unifier> Unify(const Atom& a, const Atom& b);

  Atom LowerBound() const;
  Atom UpperBound() const;

  const TermSeq& z() const { return z_; }
  PredId pred() const { return pred_; }
  const TermSeq& args() const { return args_; }

  bool is_ground() const;

  void CollectVariables(std::set<Variable>* vs) const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  TermSeq z_;
  PredId pred_;
  TermSeq args_;
};

std::ostream& operator<<(std::ostream& os, const Atom& a);
std::ostream& operator<<(std::ostream& os, const std::set<Atom>& as);

}  // namespace esbl

#endif  // SRC_ATOM_H_

