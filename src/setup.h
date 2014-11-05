// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// Notice that HPlus must be initialized and kept up to date using
// UpdateHPlus().
// FullStaticPel(), Rel(), Pel(), ... depend on HPlus.
//
// Is a setup for a minimal HPlus inconsistent iff it is inconsistent for any
// HPlus?

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <set>
#include <vector>
#include "./clause.h"

namespace esbl {

class Setup {
 public:
  Setup() = default;
  Setup(std::initializer_list<Clause> cs) : cs_(cs) {}
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void UpdateHPlus(const Term::Factory& tf);
  void GuaranteeConsistency(int k);
  void AddSensingResult(const TermSeq& z, const StdName& a, bool r);

  std::set<Literal> Rel(const SimpleClause& c) const;
  std::set<Atom> Pel(const SimpleClause& c) const;

  bool Inconsistent(int k);
  bool Entails(const SimpleClause& c, int k);

  const std::set<Clause> clauses() const { return cs_; }
  Variable::SortedSet variables() const;
  StdName::SortedSet names() const;

 private:
  void UpdateHPlusFor(const StdName::SortedSet& ns);
  void UpdateHPlusFor(const Variable::SortedSet& vs);
  void UpdateHPlusFor(const SimpleClause& c);
  void UpdateHPlusFor(const Clause& c);
  std::set<Atom> FullStaticPel() const;
  bool ContainsEmptyClause() const;
  size_t MinimizeWrt(const Clause& c);
  size_t Minimize();
  void PropagateUnits();
  bool Subsumes(const SimpleClause& c) const;
  bool Subsumes(const Clause& c) const;
  bool SubsumesWithSplits(std::set<Atom> pel, const SimpleClause& c,
                          int k) const;
  bool SplitRelevant(const Atom& a, const Clause& c, int k) const;

  std::set<Clause> cs_;
  std::vector<bool> incons_;
  StdName::SortedSet hplus_;
};

std::ostream& operator<<(std::ostream& os, const Setup& s);

}  // namespace esbl

#endif  // SRC_SETUP_H_

