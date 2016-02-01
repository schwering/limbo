// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_SETUPS_H_
#define SRC_SETUPS_H_

#include <vector>
#include "./setup.h"

namespace lela {

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
  const StdName::SortedSet& names() const { return setup(0).names(); }

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

