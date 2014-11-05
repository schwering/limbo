// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setup.h"
#include <cassert>

namespace esbl {

void Setup::AddClause(const Clause& c) {
  cs_.insert(c);
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

void Setup::UpdateHPlus(const Term::Factory& tf) {
  hplus_ = tf.sorted_names();
  for (const Clause& c : cs_) {
    UpdateHPlusFor(c);
  }
}

void Setup::GuaranteeConsistency(int k) {
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
  std::set<Literal> rel = c;
  std::set<Literal> new_rel = rel;
  while (!new_rel.empty()) {
    for (const Literal& l : new_rel) {
      for (const Clause& c : cs_) {
        for (const Literal& ll : c.Rel(hplus_, l)) {
          const auto p = rel.insert(ll);
          if (p.second) {
            new_rel.insert(ll);
          }
        }
      }
    }
    new_rel.clear();
  }
  return rel;
}

std::set<Atom> Setup::Pel(const SimpleClause& c) const {
  const std::set<Literal> rel = Rel(c);
  std::set<Atom> pel;
  for (auto it = rel.begin(); it != rel.end(); ++it) {
    const Literal& l = *it;
    if (!l.sign() || l.z().empty()) {
      continue;
    }
    const auto first = rel.lower_bound(l.LowerBound());
    const auto last = rel.upper_bound(l.UpperBound());
    for (auto jt = first; jt != last; ++jt) {
      const Literal& ll = *jt;
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

size_t Setup::MinimizeWrt(const Clause& c) {
  if (ContainsEmptyClause()) {
    size_t n = cs_.size();
    cs_.clear();
    cs_.insert(Clause::EMPTY);
    incons_.clear();
    incons_[0] = true;
    return n - 1;
  }
  size_t n = 0;
  for (auto it = cs_.upper_bound(c); it != cs_.end(); ++it) {
    const Clause& d = *it;
    if (d.literals().size() > c.literals().size() && c.Subsumes(d)) {
      cs_.erase(it);
      ++n;
    }
  }
  return n;
}

size_t Setup::Minimize() {
  size_t n = 0;
  for (const Clause& c : cs_) {
    n += MinimizeWrt(c);
  }
  return n;
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
#else
  size_t n;
  do {
    n = 0;
    const auto first = cs_.lower_bound(Clause::MIN_UNIT);
    const auto last = cs_.upper_bound(Clause::MAX_UNIT);
    for (const Clause& c : cs_) {
      for (auto it = first; it != last; ++it) {
        const Clause& unit = *it;
        assert(unit.is_unit());
        n += c.ResolveWithUnit(unit, &cs_);
      }
    }
    if (n > 0) {
      Minimize();
    }
  } while (n > 0);
#endif
}

bool Setup::Subsumes(const Clause& c) const {
  for (const Clause& d : cs_) {
    if (d.Subsumes(c)) {
      return true;
    }
  }
  return false;
}

bool Setup::SplitRelevant(const Atom& a, const Clause& c, int k) const {
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
                               int k) const {
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
    s1.cs_.insert(c1);
    s2.cs_.insert(c1);
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

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  os << "Setup:" << std::endl;
  for (const Clause& c : s.clauses()) {
    os << "    " << c << std::endl;
  }
  return os;
}

}  // namespace esbl

