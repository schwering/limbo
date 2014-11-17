// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_ATOM_H_
#define SRC_ATOM_H_

#include <map>
#include <set>
#include <utility>
#include <vector>
#include "./maybe.h"
#include "./term.h"

namespace esbl {

class Atom {
 public:
  typedef std::set<Atom> Set;

  typedef int PredId;

  static constexpr PredId SF = -1;
  static constexpr PredId POSS = -2;

  static const Atom MIN;
  static const Atom MAX;

  Atom(const TermSeq& z, PredId pred, const TermSeq& args)
      : z_(z), pred_(pred), args_(args) {}
  Atom(const Atom&) = default;
  Atom& operator=(const Atom&) = default;

  bool operator==(const Atom& a) const {
    return pred_ == a.pred_ && z_ == a.z_ && args_ == a.args_;
  }
  bool operator<(const Atom& a) const {
    return std::tie(pred_, z_, args_) < std::tie(a.pred_, a.z_, a.args_);
  }

  Atom PrependActions(const TermSeq& z) const;

  Atom Substitute(const Unifier& theta) const;
  Atom Ground(const Assignment& theta) const;
  bool Matches(const Atom& a, Unifier* theta) const;
  static bool Unify(const Atom& a, const Atom& b, Unifier* theta);
  static Maybe<Unifier> Unify(const Atom& a, const Atom& b);

  Atom LowerBound() const;
  Atom UpperBound() const;

  const TermSeq& z() const { return z_; }
  PredId pred() const { return pred_; }
  const TermSeq& args() const { return args_; }

  bool ground() const;

  void CollectVariables(Variable::Set* vs) const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  TermSeq z_;
  PredId pred_;
  TermSeq args_;
};

std::ostream& operator<<(std::ostream& os, const Atom& a);
std::ostream& operator<<(std::ostream& os, const Atom::Set& as);

}  // namespace esbl

#endif  // SRC_ATOM_H_

