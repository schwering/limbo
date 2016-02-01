// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setups.h"
#include <cassert>
#include <algorithm>
#include <numeric>

namespace lela {

Setups::Setups() {
  // Need one setup so that AddClause(), GuaranteeConsistency(), and
  // AddSensingResult() have something to take effect on. Since
  // PropagateBeliefs() uses copies of the last setups, these effects remain in
  // force in all setups newly created through AddBeliefConditional().
  if (ss_.empty()) {
    ss_.push_back(Setup());
  }
}

void Setups::AddClause(const Clause& c) {
  for (Setup& s : ss_) {
    s.AddClause(c);
  }
}

void Setups::AddBeliefConditional(const Clause& neg_phi,
                                  const Clause& psi,
                                  split_level k) {
  // I'm not sure if and how non-ground belief conditionals should be
  // implemented. I suppose that in ESB they could lead to infinitely many
  // plausibility levels.
  assert(neg_phi.literals().ground());
  assert(psi.literals().ground());
  assert(!ss_.empty());
  Ewff e = Ewff::And(neg_phi.ewff(), psi.ewff());
  SimpleClause c = neg_phi.literals();
  c.insert(psi.literals().begin(), psi.literals().end());
  assert(c.ground());
  Clause neg_phi_or_psi(e, c);
  bcs_.push_back(BeliefConditional(neg_phi, neg_phi_or_psi, k));
  PropagateBeliefs();
}

void Setups::PropagateBeliefs() {
  assert(!ss_.empty());
  for (belief_level p = 0; ; ++p) {
    // Add phi => psi to the current setup.
    for (const BeliefConditional& bc : bcs_) {
      if (bc.p == p) {
        // Keep a last, clean setup until we have reached the bound of
        // bcs_.size() + 1 setups.
        if (p+1 == ss_.size() && p+1 <= bcs_.size()) {
          ss_.push_back(ss_.back());
        }
        ss_[p].AddClause(bc.neg_phi_or_psi);
      }
    }
    // If ~phi holds, keep phi => psi for the next level.
    bool one_belief_cond_active = false;
    for (BeliefConditional& bc : bcs_) {
      if (bc.p == p) {
        assert(bc.neg_phi.ewff() == Ewff::TRUE);
        if (ss_[p].Entails(bc.neg_phi.literals(), bc.k)) {
          ++bc.p;
        } else {
          one_belief_cond_active = true;
        }
      }
    }
    // Remove unused setups at the end.
    if (!one_belief_cond_active) {
      ss_.erase(ss_.begin() + p + 1, ss_.end());
      break;
    }
  }
  assert(ss_.size() <= bcs_.size() + 1);
}

void Setups::GuaranteeConsistency(split_level k) {
  assert(!ss_.empty());
  for (Setup& s : ss_) {
    s.GuaranteeConsistency(k);
  }
}

bool Setups::Inconsistent(split_level k) {
  for (Setup& s : ss_) {
    if (!s.Inconsistent(k)) {
      return false;
    }
  }
  return true;
}

bool Setups::Entails(const SimpleClause& c, split_level k) {
  for (Setup& s : ss_) {
    if (!s.Inconsistent(k)) {
      return s.Entails(c, k);
    }
  }
  return true;
}

bool Setups::Entails(const SimpleClause& neg_phi,
                     const SimpleClause& psi,
                     split_level k) {
  SimpleClause neg_phi_or_psi = neg_phi;
  neg_phi_or_psi.insert(psi.begin(), psi.end());
  for (Setup& s : ss_) {
    if (!s.Entails(neg_phi, k)) {
      return s.Entails(neg_phi_or_psi, k);
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  os << "Setup:" << std::endl;
  os << s.clauses() << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const Setups& ss) {
//  os << "Belief Conditionals:" << std::endl;
//  for (const Setups::BeliefConditional& bc : ss.bcs_) {
//    os << "  neg_phi = " << bc.neg_phi << std::endl;
//    os << "  neg_phi_or_psi = " << bc.neg_phi_or_psi << std::endl;
//    os << "  p = " << bc.p << std::endl;
//    os << "  k = " << bc.k << std::endl;
//  }
  os << "Belief Setups:" << std::endl;
  int n = 0;
  for (const Setup& s : ss.setups()) {
    os << "Level " << n << ": " << s << std::endl;
    ++n;
  }
  return os;
}

}  // namespace lela

