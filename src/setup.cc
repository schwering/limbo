// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setup.h"
#include <cassert>
#include <algorithm>
#include <numeric>

namespace esbl {

void Setup::AddClause(const Clause& c) {
  if (c.box()) {
    boxes_.insert(c);
    UpdateHPlusFor(c);
  } else {
    AddClauseWithoutConsistencyCheck(c);
    if (!incons_.empty() && !incons_[incons_.size() - 1]) {
      for (const Assignment theta : c.ewff().Models(hplus_)) {
        for (split_level k = 0; k < incons_.size(); ++k) {
          if (!incons_[k]) {
            bool negation_entailed = true;
            for (const Literal l : c.literals().Ground(theta)) {
              if (!Entails({l.Flip()}, k)) {
                negation_entailed = false;
                break;
              }
            }
            if (negation_entailed) {
              for (; k < incons_.size(); ++k) {
                incons_[k] = true;
              }
              return;
            }
          }
        }
      }
    }
  }
}

void Setup::AddClauseWithoutConsistencyCheck(const Clause& c) {
  assert(!c.box());
  cs_.insert(c);
  UpdateHPlusFor(c);
}

void Setup::UpdateHPlusFor(const Variable::SortedSet& vs) {
  for (const auto& p : vs) {
    const Term::Sort& sort = p.first;
    StdName::Set& ns = hplus_[sort];
    const int need_n_vars = p.second.size();
    const int have_n_vars = std::distance(ns.begin(),
                                          ns.lower_bound(StdName::MIN_NORMAL));
    for (int i = have_n_vars; i < need_n_vars; ++i) {
      const StdName n = Term::Factory::CreatePlaceholderStdName(i, sort);
      const auto p = ns.insert(n);
      assert(p.second);
    }
  }
}

void Setup::UpdateHPlusFor(const SimpleClause& c) {
  c.CollectNames(&hplus_);
  Variable::SortedSet vs;
  c.CollectVariables(&vs);
  UpdateHPlusFor(vs);
}

void Setup::UpdateHPlusFor(const Clause& c) {
  c.CollectNames(&hplus_);
  Variable::SortedSet vs;
  c.CollectVariables(&vs);
  UpdateHPlusFor(vs);
}

void Setup::GuaranteeConsistency(split_level k) {
  for (split_level l = 0; l <= k; ++l) {
    assert(l >= incons_.size() || !incons_[l]);
    incons_[l] = false;
  }
}

void Setup::GroundBoxes(const TermSeq& z) {
  for (size_t n = 0; n <= z.size(); ++n) {
    const TermSeq zz(z.begin(), z.begin() + n);
    const auto p = grounded_.insert(zz);
    if (p.second) {
      for (const Clause& c : boxes_) {
        AddClauseWithoutConsistencyCheck(c.InstantiateBox(zz));
      }
    }
  }
}

Atom::Set Setup::FullStaticPel() const {
  Atom::Set pel;
  for (const Clause& c : cs_) {
    if (!c.box()) {
      for (const Assignment theta : c.ewff().Models(hplus_)) {
        for (const SimpleClause d :
             c.literals().Ground(theta).Instances(hplus_)) {
          for (const Literal& l : d) {
            pel.insert(l);
          }
        }
      }
    }
  }
  return pel;
}

Literal::Set Setup::Rel(const SimpleClause& c) const {
  Literal::Set rel;
  std::deque<Literal> frontier(c.begin(), c.end());
  while (!frontier.empty()) {
    const Literal& l = frontier.front();
    const auto p = rel.insert(l);
    if (p.second) {
      for (const Clause& c : cs_) {
        c.Rel(hplus_, l, &frontier);
      }
    }
    frontier.pop_front();
  }
  return rel;
}

Atom::Set Setup::Pel(const SimpleClause& c) const {
  // Pel is the set of all atoms a such that both a and ~a are relevant to prove
  // some literal in l:
  // Pel(c) = { a | for all a, l such that a in Rel(l), ~a in Rel(l), l in c }
  // Furthermore we only take literals from the initial situation. The idea is
  // here that splitting any later literal is redundant because it could also be
  // achieved from the initial literal through unit propagatoin since successor
  // state axioms introduce no nondeterminism. However, we add new knowledge
  // through sensing literals in future situations. Is the limitation to initial
  // literals still complete?
  const Literal::Set rel = Rel(c);
  Atom::Set pel;
  for (auto it = rel.begin(); it != rel.end(); ++it) {
    const Literal& l = *it;
    if (!l.sign()) {
      continue;
    }
    // This optimization breaks things when we've sensed a disjunction.
    if (!l.z().empty() && c.find(l) == c.end()) {
      continue;
    }
    for (const Literal& ll : rel.range(!l.sign(), l.pred())) {
      assert(l.pred() == ll.pred());
      assert(l.sign() != ll.sign());
      Unifier theta;
      const bool succ = Atom::Unify(l, ll, &theta);
      if (succ) {
        pel.insert(l.Substitute(theta));
      }
    }
  }
  return pel;
}

bool Setup::ContainsEmptyClause() const {
  const auto it = cs_.begin();
  return it != cs_.end() && it->literals().size() == 0;
}

size_t Setup::MinimizeWrt(Clause::Set::iterator it) {
  if (ContainsEmptyClause()) {
    size_t n = cs_.size();
    cs_.clear();
    cs_.insert(Clause::EMPTY);
    incons_.resize(1, true);
    return n - 1;
  }
  size_t n = 0;
  const Clause c = *it;
  for (++it; it != cs_.end(); ) {
    const Clause& d = *it;
    if (d.literals().size() > c.literals().size() && c.Subsumes(d)) {
      it = cs_.erase(it);
      ++n;
    } else {
      ++it;
    }
  }
  return n;
}

void Setup::Minimize() {
  for (auto it = cs_.begin(); it != cs_.end(); ++it) {
    const Clause c = *it;
    const size_t n = MinimizeWrt(it);
    if (n > 0) {
      it = cs_.find(c);
    }
  }
}

void Setup::PropagateUnits() {
  if (ContainsEmptyClause()) {
    return;
  }
  size_t n_units = 0;
  for (;;) {
    const auto first = cs_.first_unit();
    const size_t d = count_while(first, cs_.end(),
                                 [](const Clause& c) { return c.is_unit(); });
    if (d <= n_units) {
      break;
    }
    n_units = d;
    size_t n_new_clauses = 0;
    for (const Clause& c : cs_) {
      for (auto it = first; it != cs_.end() && it->is_unit(); ++it) {
        const Clause& unit = *it;
        n_new_clauses += c.ResolveWithUnit(unit, &cs_);
      }
    }
    if (n_new_clauses > 0) {
      Minimize();
    }
  }
}

bool Setup::Subsumes(const Clause& c) {
  PropagateUnits();
  for (const Clause& d : cs_) {
    if (d.Subsumes(c)) {
      return true;
    }
  }
  return false;
}

bool Setup::SplitRelevant(const Atom& a, const Clause& c, split_level k) {
  assert(k >= 0);
  const Literal l1(true, a);
  const Literal l2(false, a);
  if (c.literals().find(l1) != c.literals().end() ||
      c.literals().find(l2) != c.literals().end()) {
    return true;
  }
  if (Subsumes(Clause(Ewff::TRUE, {l1})) ||
      Subsumes(Clause(Ewff::TRUE, {l2}))) {
    return false;
  }
  for (const Clause& d : cs_) {
    if (d.SplitRelevant(a, c, k)) {
      return true;
    }
  }
  return false;
}

bool Setup::SubsumesWithSplits(Atom::Set pel,
                               const SimpleClause& c,
                               split_level k) {
  if (Subsumes(Clause(Ewff::TRUE, c))) {
    return true;
  }
  if (k == 0) {
#if 0
    pel = c.Sensings();
#else
    return false;
#endif
  }
  for (auto it = pel.begin(); it != pel.end(); ) {
    const Atom a = *it;
    it = pel.erase(it);
    if (!SplitRelevant(a, Clause(Ewff::TRUE, c), k)) {
      continue;
    }
    const Clause c1(Ewff::TRUE, {Literal(true, a)});
    const Clause c2(Ewff::TRUE, {Literal(false, a)});
    Setup s1 = *this;
    Setup s2 = *this;
    s1.AddClauseWithoutConsistencyCheck(c1);
    s2.AddClauseWithoutConsistencyCheck(c2);
    if (s1.SubsumesWithSplits(pel, c, k-1) &&
        s2.SubsumesWithSplits(pel, c, k-1)) {
      return true;
    }
  }
  return false;
}

bool Setup::Inconsistent(split_level k) {
  assert(k >= 0);
  if (ContainsEmptyClause()) {
    return true;
  }
  split_level l = static_cast<split_level>(incons_.size());
  if (l > 0) {
    if (k < l) {
      return incons_[k];
    }
    if (incons_[l-1]) {
      return true;
    }
  }
  const Atom::Set pel = k <= 0 ? Atom::Set() : FullStaticPel();
  for (; l <= k; ++l) {
    const bool r = SubsumesWithSplits(pel, SimpleClause::EMPTY, l);
    incons_[l] = r;
    if (r) {
      for (++l; l <= k; ++l) {
        incons_[l] = r;
      }
    }
  }
  assert(k < static_cast<split_level>(incons_.size()));
  return incons_[k];
}

bool Setup::Entails(const SimpleClause& c, split_level k) {
  assert(k >= 0);
  if (Inconsistent(k)) {
    return true;
  }
  for (const Literal& l : c) {
    GroundBoxes(l.z());
  }
  UpdateHPlusFor(c);
  const Atom::Set pel = k <= 0 ? Atom::Set() : Pel(c);
  return SubsumesWithSplits(pel, c, k);
}

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

void Setups::AddBeliefConditional(const Clause& neg_phi, const Clause& psi,
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
  bool one_belief_cond_active = true;
  for (belief_level p = 0; one_belief_cond_active; ++p) {
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
    one_belief_cond_active = false;
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
  }
  assert(!one_belief_cond_active);
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

bool Setups::Entails(const SimpleClause& neg_phi, const SimpleClause& psi,
                     split_level k) {
  for (Setup& s : ss_) {
    if (!s.Entails(neg_phi, k)) {
      return s.Entails(psi, k);
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Clause::Set& cs) {
  for (const Clause& c : cs) {
    os << "    " << c << std::endl;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  os << "Setup:" << std::endl;
  for (const Clause& c : s.clauses()) {
    os << "    " << c << std::endl;
  }
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

}  // namespace esbl

