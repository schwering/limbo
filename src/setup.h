// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// FullStaticPel(), Rel(), Pel(), ... depend on HPlus.
//
// Assuming we have rather few different action sequences, we instantiate boxes
// by grounding them already instead of instantiating them with sequences of
// action variables.
//
// Is a setup for a minimal HPlus inconsistent iff it is inconsistent for any
// HPlus?

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <deque>
#include <set>
#include <vector>
#include "./clause.h"

namespace esbl {

class Setup {
 public:
  Setup() = default;
  Setup(std::initializer_list<Clause> cs) : cs_(cs) {}

  void AddClause(const Clause& c);
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
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void UpdateHPlusFor(const StdName::SortedSet& ns);
  void UpdateHPlusFor(const Variable::SortedSet& vs);
  void UpdateHPlusFor(const SimpleClause& c);
  void UpdateHPlusFor(const Clause& c);
  void GroundBoxes(const TermSeq& z);
  std::set<Atom> FullStaticPel() const;
  bool ContainsEmptyClause() const;
  size_t MinimizeWrt(std::set<Clause>::iterator c);
  void Minimize();
  void PropagateUnits();
  bool Subsumes(const Clause& c);
  bool SubsumesWithSplits(std::set<Atom> pel, const SimpleClause& c, int k);
  bool SplitRelevant(const Atom& a, const Clause& c, int k);

  std::set<Clause> cs_;
  std::set<Clause> boxes_;
  std::set<TermSeq> grounded_;
  std::vector<bool> incons_;
  StdName::SortedSet hplus_;
};

std::ostream& operator<<(std::ostream& os, const std::set<Clause>& cs);
std::ostream& operator<<(std::ostream& os, const Setup& s);

}  // namespace esbl

#endif  // SRC_SETUP_H_

