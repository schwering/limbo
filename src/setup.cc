// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setup.h"
#include <cassert>
#include <iostream>

namespace esbl {

void Setup::AddClause(const Clause& c) {
  if (c.box()) {
    boxes_.insert(c);
  } else {
    cs_.insert(c);
    UpdateHPlusFor(c);
  }
}

void Setup::UpdateHPlusFor(const Variable::SortedSet& vs) {
  for (const auto& p : vs) {
    const Term::Sort& sort = p.first;
    std::set<StdName>& ns = hplus_[sort];
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

void Setup::GuaranteeConsistency(int k) {
  if (k >= static_cast<int>(incons_.size())) {
    incons_.resize(k+1);
  }
  for (int l = 0; l <= k; ++l) {
    incons_[l] = false;
  }
}

void Setup::AddSensingResult(const TermSeq& z, const StdName& a, bool r) {
  const Literal l(z, r, Atom::SF, {a});
  const SimpleClause c({l});
  AddClause(Clause(Ewff::TRUE, c));
  UpdateHPlusFor(c);

  const SimpleClause d({l.Flip()});
  for (int k = 0; k < static_cast<int>(incons_.size()); ++k) {
    if (!incons_[k] && Entails(d, k)) {
      incons_[k] = true;
    }
  }
}

void Setup::GroundBoxes(const TermSeq& z) {
  for (auto it = z.begin(); it != z.end(); ++it) {
    const TermSeq zz(z.begin(), it);
    const auto p = grounded_.insert(zz);
    if (p.second) {
      for (const Clause& c : boxes_) {
        AddClause(c.InstantiateBox(zz));
      }
    }
  }
}

std::set<Atom> Setup::FullStaticPel() const {
  std::set<Atom> pel;
  for (const Clause& c : cs_) {
    if (!c.box()) {
      for (const Assignment& theta : c.ewff().Models(hplus_)) {
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

std::set<Literal> Setup::Rel(const SimpleClause& c) const {
  std::set<Literal> rel;
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

std::set<Atom> Setup::Pel(const SimpleClause& c) const {
  // Pel is the set of all atoms a such that both a and ~a are relevant to prove
  // some literal in l:
  // Pel(c) = { a | for all a, l such that a in Rel(l), ~a in Rel(l), l in c }
  // Furthermore we only take literals from the initial situation. The idea is
  // here that splitting any later literal is redundant because it could also be
  // achieved from the initial literal through unit propagatoin since successor
  // state axioms introduce no nondeterminism. However, we add new knowledge
  // through sensing literals in future situations. Is the limitation to initial
  // literals still complete?
  const std::set<Literal> rel = Rel(c);
  std::set<Atom> pel;
  for (auto it = rel.begin(); it != rel.end(); ++it) {
    const Literal& l = *it;
    if (!l.sign()) {
      continue;
    }
    if (!l.z().empty()) {
      continue;
    }
    const auto first = rel.lower_bound(l.LowerBound());
    const auto last = rel.lower_bound(l.UpperBound());
    for (auto jt = first; jt != last; ++jt) {
      const Literal& ll = *jt;
      assert(l.pred() == ll.pred());
      if (ll.sign()) {
        continue;
      }
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

size_t Setup::MinimizeWrt(std::set<Clause>::iterator it) {
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
#if 0
  size_t n;
  do {
    n = 0;
    const auto first = cs_.lower_bound(Clause::MIN_UNIT);
    const auto last = cs_.upper_bound(Clause::MAX_UNIT);
    for (const Clause& c : cs_) {
      std::set<Clause> new_cs;
      for (auto it = first; it != last; ++it) {
        const Clause& unit = *it;
        assert(unit.is_unit());
        c.ResolveWithUnit(unit, &new_cs);
        for (const Clause& d : new_cs) {
          const auto p = cs_.insert(d);
          if (p.second) {
            ++n;
            MinimizeWrt(d);
          }
        }
      }
    }
  } while (n > 0);
#endif
  size_t n_units = 0;
  for (;;) {
    const auto first = cs_.lower_bound(Clause::MIN_UNIT);
    const auto last = cs_.upper_bound(Clause::MAX_UNIT);
    const size_t d = std::distance(first, last);
    if (d <= n_units) {
      break;
    }
    n_units = d;
    size_t n_new_clauses = 0;
    for (const Clause& c : cs_) {
      for (auto it = first; it != last; ++it) {
        const Clause& unit = *it;
        assert(unit.is_unit());
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

bool Setup::SplitRelevant(const Atom& a, const Clause& c, int k) {
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

bool Setup::SubsumesWithSplits(std::set<Atom> pel,
                               const SimpleClause& c,
                               int k) {
  if (Subsumes(Clause(Ewff::TRUE, c))) {
    return true;
  }
  if (k == 0) {
    // generate SF literals from the actions in c
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
    s1.AddClause(c1);
    s2.AddClause(c2);
    if (s1.SubsumesWithSplits(pel, c, k-1) &&
        s2.SubsumesWithSplits(pel, c, k-1)) {
      return true;
    }
  }
  return false;
}

bool Setup::Inconsistent(int k) {
  assert(k >= 0);
  if (ContainsEmptyClause()) {
    return true;
  }
  int l = static_cast<int>(incons_.size());
  if (l > 0) {
    if (k < l) {
      return incons_.at(k);
    }
    if (incons_.at(l-1)) {
      return true;
    }
  }
  const std::set<Atom> pel = k <= 0 ? std::set<Atom>() : FullStaticPel();
  if (k >= l) {
    incons_.resize(k+1);
  }
  for (; l <= k; ++l) {
    const bool r = SubsumesWithSplits(pel, SimpleClause::EMPTY, l);
    incons_[l] = r;
    if (r) {
      for (++l; l <= k; ++l) {
        incons_[l] = r;
      }
    }
  }
  assert(k < static_cast<int>(incons_.size()));
  return incons_[k];
}

bool Setup::Entails(const SimpleClause& c, int k) {
  assert(k >= 0);
  if (Inconsistent(k)) {
    return true;
  }
  for (const Literal& l : c) {
    GroundBoxes(l.z());
  }
  UpdateHPlusFor(c);
  const std::set<Atom> pel = k <= 0 ? std::set<Atom>() : Pel(c);
  return SubsumesWithSplits(pel, c, k);
}

Variable::SortedSet Setup::variables() const {
  Variable::SortedSet vs;
  for (const Clause& c : cs_) {
    c.CollectVariables(&vs);
  }
  return vs;
}

StdName::SortedSet Setup::names() const {
  StdName::SortedSet ns;
  for (const Clause& c : cs_) {
    c.CollectNames(&ns);
  }
  return ns;
}

std::ostream& operator<<(std::ostream& os, const std::set<Clause>& cs) {
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

}  // namespace esbl

