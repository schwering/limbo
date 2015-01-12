// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// FullStaticPel(), Rel(), Pel(), ... depend on hplus.
//
// Assuming we have rather few different action sequences, we instantiate boxes
// by grounding them already instead of instantiating them with sequences of
// action variables.
//
// There are const versions of Inconsistent() and Entails() which just perform a
// const_cast. Their behavior is not-const for two reasons: Firstly, they update
// the inconsistency cache, which is hidden from the outside, though. Secondly,
// they add names to hplus, which is reflected in the return value of hplus().
//
// Is a setup for a minimal hplus inconsistent iff it is inconsistent for any
// hplus?

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <deque>
#include <list>
#include <set>
#include <vector>
#include "./clause.h"

namespace lela {

class Setup {
 public:
  typedef size_t split_level;

  Setup() = default;
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void AddClause(const Clause& c);
  void AddClauseWithoutConsistencyCheck(const Clause& c);
  void GuaranteeConsistency(split_level k);

  void Progress(const StdName& n);

  bool Inconsistent(split_level k);
  bool Entails(const SimpleClause& c, split_level k);

  bool Inconsistent(split_level k) const {
    return const_cast<Setup*>(this)->Inconsistent(k);
  }
  bool Entails(const SimpleClause& c, split_level k) const {
    return const_cast<Setup*>(this)->Entails(c, k);
  }

  const Clause::Set& clauses() const { return cs_; }
  const StdName::SortedSet& hplus() const { return hplus_; }

 private:
  class BitMap : public std::vector<bool> {
   public:
    using std::vector<bool>::vector;

    reference operator[](size_type pos) {
      if (pos >= size()) {
        resize(pos+1, false);
      }
      return std::vector<bool>::operator[](pos);
    }

    const_reference operator[](size_type pos) const {
      return pos < size() ? std::vector<bool>::operator[](pos) : false;
    }
  };

  void UpdateHPlusFor(const StdName::SortedSet& ns);
  void UpdateHPlusFor(const Variable::SortedSet& vs);
  void UpdateHPlusFor(const SimpleClause& c);
  void UpdateHPlusFor(const Clause& c);
  void GroundBoxes(const TermSeq& z);
  Atom::Set FullStaticPel() const;
  Literal::Set Rel(const SimpleClause& c) const;
  Atom::Set Pel(const SimpleClause& c) const;
  bool ContainsEmptyClause() const;
  size_t MinimizeWrt(Clause::Set::iterator c);
  void Minimize();
  void PropagateUnits();
  bool Subsumes(const Clause& c);
  bool SubsumesWithSplits(Atom::Set pel, const SimpleClause& c, split_level k);
  bool SplitRelevant(const Atom& a, const Clause& c, split_level k);

  Clause::Set cs_;
  Clause::Set boxes_;
  TermSeq::Set grounded_;
  BitMap incons_;
  StdName::SortedSet hplus_;
};

class Setups {
 public:
  typedef Setup::split_level split_level;
  typedef size_t belief_level;

  Setups();
  Setups(const Setups&) = default;
  Setups& operator=(const Setups&) = default;

  void AddClause(const Clause& c);
  void AddBeliefConditional(const Clause& neg_phi, const Clause& psi,
                            split_level k);
  void GuaranteeConsistency(split_level k);

  bool Inconsistent(split_level k);
  bool Entails(const SimpleClause& c, split_level k);
  bool Entails(const SimpleClause& neg_phi, const SimpleClause& psi,
               split_level k);

  bool Inconsistent(split_level k) const {
    return const_cast<Setups*>(this)->Inconsistent(k);
  }
  bool Entails(const SimpleClause& c, split_level k) const {
    return const_cast<Setups*>(this)->Entails(c, k);
  }
  bool Entails(const SimpleClause& neg_phi, const SimpleClause& psi,
               split_level k) const {
    return const_cast<Setups*>(this)->Entails(neg_phi, psi, k);
  }

  const std::vector<Setup>& setups() const { return ss_; }
  belief_level n_setups() const { return ss_.size(); }
  const Setup& setup(belief_level i) const { return ss_.at(i); }
  const Setup& first_setup() const { return ss_.front(); }
  const Setup& last_setup() const { return ss_.back(); }
  Setup* mutable_setup(belief_level i) { return &ss_.at(i); }
  Setup* mutable_first_setup() { return &ss_.front(); }
  Setup* mutable_last_setup() { return &ss_.back(); }
  const StdName::SortedSet& hplus() const { return setup(0).hplus(); }

 private:
  struct BeliefConditional {
    BeliefConditional() = default;
    BeliefConditional(const Clause& neg_phi, const Clause& neg_phi_or_psi,
                      split_level k)
        : neg_phi(neg_phi), neg_phi_or_psi(neg_phi_or_psi), k(k), p(0) {}
    BeliefConditional(const BeliefConditional&) = default;
    BeliefConditional& operator=(const BeliefConditional&);

    const Clause neg_phi;
    const Clause neg_phi_or_psi;
    const split_level k;
    belief_level p;
  };

  void PropagateBeliefs();

  std::vector<Setup> ss_;
  std::vector<BeliefConditional> bcs_;
};

std::ostream& operator<<(std::ostream& os, const Setup& s);
std::ostream& operator<<(std::ostream& os, const Setups& ss);

}  // namespace lela

#endif  // SRC_SETUP_H_

