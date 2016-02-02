// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setup.h"
#include <cassert>
#include <deque>
#include <algorithm>
#include <numeric>

namespace lela {

#if 0
void Setup::AddClause(const Clause& c) {
  AddClauseWithoutConsistencyCheck(c);
  if (!incons_.empty()) {
    for (const Assignment theta : c.ewff().Models(names_)) {
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
            if (k == 0) {
              return;
            }
          }
        }
      }
    }
  }
}

void Setup::AddClauseWithoutConsistencyCheck(const Clause& c) {
  cs_.insert(c);
  UpdateHPlusFor(c);
}

void Setup::UpdateHPlusFor(const Variable::SortedSet& vs) {
  for (const auto& p : vs) {
    const Term::Sort& sort = p.first;
    StdName::Set& ns = names_[sort];
    const int need_n_placeholders = p.second.size();
    const int have_n_placeholders = ns.n_placeholders();
    for (int i = have_n_placeholders; i < need_n_placeholders; ++i) {
      names_.AddNewPlaceholder(sort);
    }
  }
}

void Setup::UpdateHPlusFor(const SimpleClause& c) {
  c.CollectNames(&names_);
  Variable::SortedSet vs;
  c.CollectVariables(&vs);
  UpdateHPlusFor(vs);
}

void Setup::UpdateHPlusFor(const Clause& c) {
  c.CollectNames(&names_);
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

Atom::Set Setup::FullPel() const {
  Atom::Set pel;
  for (const Clause& c : cs_) {
    for (const Assignment theta : c.ewff().Models(names_)) {
      for (const SimpleClause d :
           c.literals().Ground(theta).Instances(names_)) {
        for (const Literal& l : d) {
          pel.insert(l);
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
        c.Rel(names_, l, &frontier);
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
  const Literal::Set rel = Rel(c);
  Atom::Set pel;
  for (auto it = rel.begin(); it != rel.end(); ++it) {
    const Literal& l = *it;
    if (!l.sign()) {
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
                                 [](const Clause& c) { return c.unit(); });
    if (d <= n_units) {
      break;
    }
    n_units = d;
    size_t n_new_clauses = 0;
    for (const Clause& c : cs_) {
      for (auto it = first; it != cs_.end() && it->unit(); ++it) {
        const Clause& unit = *it;
        const Literal& unit_l = *unit.literals().begin();
        n_new_clauses += Clause::ResolveWrt(c, unit, unit_l.pred(), &cs_);
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
    return false;
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
  const Atom::Set pel = k <= 0 ? Atom::Set() : FullPel();
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
  UpdateHPlusFor(c);
  const Atom::Set pel = k <= 0 ? Atom::Set() : Pel(c);
  return SubsumesWithSplits(pel, c, k);
}
#endif

}  // namespace lela

