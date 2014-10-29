// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./clause.h"
#include <algorithm>
#include <numeric>

namespace esbl {

bool Clause::operator==(const Clause& c) const {
  return box_ == c.box_ && e_ == c.e_ && ls_ == c.ls_;
}

bool Clause::operator!=(const Clause& c) const {
  return !operator==(c);
}

bool Clause::operator<(const Clause& c) const {
  return box_ < c.box_ ||
      (box_ == c.box_ && e_ < c.e_) ||
      (box_ == c.box_ && e_ == c.e_ && ls_ < c.ls_);
}

Clause Clause::PrependActions(const TermSeq& z) const {
  SimpleClause ls;
  for (const Literal& l : ls_) {
    ls.insert(l.PrependActions(z));
  }
  return Clause(box_, e_, ls);
}

std::tuple<bool, Clause> Clause::Substitute(const Unifier& theta) const {
  bool succ;
  Ewff e;
  std::tie(succ, e) = e_.Substitute(theta);
  if (!succ) {
    return failed<Clause>();
  }
  SimpleClause ls;
  for (const Literal& l : ls_) {
    ls.insert(l.Substitute(theta));
  }
  return std::make_pair(true, Clause(box_, e, ls));
}

namespace {
std::tuple<bool, TermSeq, Unifier> BoxUnify(const Atom& cl_a,
                                            const Atom& ext_a) {
  if (ext_a.z().size() < cl_a.z().size()) {
    return failed<TermSeq, Unifier>();
  }
  const size_t split = ext_a.z().size() - cl_a.z().size();
  const Atom a = ext_a.DropActions(split);
  bool succ;
  Unifier theta;
  std::tie(succ, theta) = Atom::Unify(cl_a, a);
  if (!succ) {
    return failed<TermSeq, Unifier>();
  }
  const TermSeq prefix(ext_a.z().begin(), ext_a.z().begin() + split);
  return std::make_tuple(true, prefix, theta);
}
}  // namespace

std::tuple<bool, Unifier, Clause> Clause::Unify(const Atom& cl_a,
                                                const Atom& ext_a) const {
  assert(ls_.count(Literal(true, cl_a)) + ls_.count(Literal(false, cl_a)) > 0);
  if (box_) {
    bool succ;
    TermSeq z;
    Unifier theta;
    std::tie(succ, z, theta) = BoxUnify(cl_a, ext_a);
    if (!succ) {
      return failed<Unifier, Clause>();
    }
    Clause c;
    std::tie(succ, c) = Substitute(theta);
    if (!succ) {
      return failed<Unifier, Clause>();
    }
    Clause d = c.PrependActions(z);
    d.box_ = false;
    return std::make_tuple(true, theta, d);
  } else {
    bool succ;
    Unifier theta;
    std::tie(succ, theta) = Atom::Unify(cl_a, ext_a);
    if (!succ) {
      return failed<Unifier, Clause>();
    }
    Clause c;
    std::tie(succ, c) = Substitute(theta);
    if (!succ) {
      return failed<Unifier, Clause>();
    }
    return std::make_tuple(true, theta, c);
  }
}

std::set<Literal> Clause::Rel(const StdName::SortedSet& hplus,
                              const Literal &ext_l) const {
  const size_t max_z = std::accumulate(ls_.begin(), ls_.end(), 0,
      [](size_t n, const Literal& l) { return std::max(n, l.z().size()); });
  std::set<Literal> rel;
  // Given a clause l and a clause e->c such that for some l' in c with
  // l = l'.theta for some theta and |= e.theta.theta' for some theta', for all
  // clauses l'' c.theta.theta', its negation ~l'' is relevant to l.
  // Relevance means that splitting ~l'' may help to prove l.
  // In case c is a box clause, it must be unified as well.
  // Due to the constrained form of BATs, only l' with maximum action length in
  // e->c must be considered. Intuitively, in an SSA Box([a]F <-> ...) this is
  // [a]F, and in Box(SF(a) <-> ...) it is every literal of the fluent.
  for (const Literal& cl_l : ls_) {
    if (ext_l.sign() != cl_l.sign() ||
        ext_l.pred() != cl_l.pred() ||
        cl_l.z().size() < max_z) {
      continue;
    }
    bool succ;
    Unifier theta;
    Clause c;
    std::tie(succ, theta, c) = Unify(cl_l, ext_l);
    if (!succ) {
      continue;
    }
    size_t n = c.ls_.erase(ext_l.Substitute(theta));
    assert(n >= 1);
    for (const Assignment& theta : c.e_.Models(hplus)) {
      for (const Literal& ll : c.ls_) {
        rel.insert(ll.Ground(theta).Flip());
      }
    }
  }
  return rel;
}

bool Clause::Subsumes2(const SimpleClause& c) const {
  if (ls_.empty()) {
    return true;
  }
  const Literal& cl_l = *ls_.begin();
  const auto first = c.lower_bound(cl_l.LowerBound());
  const auto last = c.upper_bound(cl_l.UpperBound());
  for (auto jt = first; jt != last; ++jt) {
    const Literal& ext_l = *jt;
    assert(ext_l.pred() == cl_l.pred());
    if (ext_l.sign() != cl_l.sign()) {
      continue;
    }
    bool succ;
    Unifier theta;
    Clause d;
    std::tie(succ, theta, d) = Unify(cl_l, ext_l);
    if (!succ) {
      continue;
    }
    size_t n = d.ls_.erase(ext_l.Substitute(theta));
    assert(n >= 1);
    if (d.Subsumes2(c)) {
      return true;
    }
  }
  return false;
}

bool Clause::Subsumes(const SimpleClause& c) const {
  auto cmp_maybe_subsuming = [](const Literal& l1, const Literal& l2) {
    return l1.LowerBound() < l2.LowerBound();
  };
  if (!std::includes(c.begin(), c.end(), ls_.begin(), ls_.end(),
                     cmp_maybe_subsuming)) {
    return false;
  }
  return Subsumes2(c);
}

bool Clause::SplitRelevant(const Atom& a, const SimpleClause c,
                           unsigned int k) const {
  const SimpleClause::const_iterator it1 = ls_.find(Literal(true, a));
  const SimpleClause::const_iterator it2 = ls_.find(Literal(false, a));
  // ls_.size() - c.size() is a (bad) approximation of (d \ c).size() where d is
  // a unification of ls_ and c
  return (it1 != ls_.end() || it2 != ls_.end()) &&
         (ls_.size() <= k + 1 || ls_.size() - c.size() <= k);
}

std::list<Clause> Clause::ResolveWithUnit(const Literal& ext_l) const {
  std::list<Clause> cs;
  for (const Literal& cl_l : ls_) {
    if (ext_l.sign() == cl_l.sign() ||
        ext_l.pred() != cl_l.pred()) {
      continue;
    }
    bool succ;
    Unifier theta;
    Clause d;
    std::tie(succ, theta, d) = Unify(cl_l, ext_l);
    if (!succ) {
      continue;
    }
    d.ls_.erase(cl_l.Substitute(theta));
    cs.push_back(d);
  }
  return cs;
}

std::list<Clause> Clause::ResolveWithUnitClause(const Clause& unit) const {
  assert(unit.is_unit());
  std::list<Clause> cs;
  const Literal& unit_l = *unit.literals().begin();
  for (const Literal& l : ls_) {
    if (unit_l.sign() == l.sign() ||
        unit_l.pred() != l.pred()) {
      continue;
    }
    bool succ;
    Unifier theta;
    Clause d;
    std::tie(succ, theta, d) = Unify(l, unit_l);
    if (!succ) {
      continue;
    }
    Ewff unit_e;
    std::tie(succ, unit_e) = unit.ewff().Substitute(theta);
    if (!succ || unit_e == Ewff::FALSE) {
      continue;
    }
    d.e_ = Ewff::And(d.e_, unit_e);
    d.ls_.erase(l.Substitute(theta));
    cs.push_back(d);
  }
  return cs;
}

std::set<Atom::PredId> Clause::positive_preds() const {
  std::set<Atom::PredId> s;
  for (const Literal& l : ls_) {
    if (l.sign()) {
      s.insert(l.pred());
    }
  }
  return s;
}

std::set<Atom::PredId> Clause::negative_preds() const {
  std::set<Atom::PredId> s;
  for (const Literal& l : ls_) {
    if (!l.sign()) {
      s.insert(l.pred());
    }
  }
  return s;
}

void Clause::CollectVariables(Variable::SortedSet* vs) const {
  e_.CollectVariables(vs);
  for (const Literal& l : ls_) {
    l.CollectVariables(vs);
  }
}

void Clause::CollectNames(StdName::SortedSet* ns) const {
  e_.CollectNames(ns);
  for (const Literal& l : ls_) {
    l.CollectNames(ns);
  }
}

std::ostream& operator<<(std::ostream& os, const SimpleClause& c) {
  os << '(';
  for (auto it = c.begin(); it != c.end(); ++it) {
    if (it != c.begin()) {
      os << " v ";
    }
    os << *it;
  }
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  os << c.ewff() << " -> " << c.literals();
  return os;
}

}  // namespace esbl

