// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./clause.h"
#include <algorithm>
#include <numeric>
#include <tuple>

namespace esbl {

const SimpleClause SimpleClause::EMPTY({});
const Clause Clause::EMPTY(false, Ewff::TRUE, {});

SimpleClause SimpleClause::ResolveWrt(const SimpleClause& c1,
                                      const SimpleClause& c2,
                                      const Literal& l1,
                                      const Literal& l2) {
  assert(c1.find(l1) != c1.end());
  assert(c2.find(l2) != c2.end());
  if (c1.size() == 1 && c2.size() == 1) {
    return EMPTY;
  } else if (c1.size() == 1) {
    SimpleClause c = c2;
    c.erase(l2);
    return c;
  } else if (c2.size() == 1) {
    SimpleClause c = c1;
    c.erase(l1);
    return c;
  } else {
    SimpleClause r1 = c1;
    r1.erase(l1);
    SimpleClause r2 = c2;
    r1.erase(l2);
    BySizeComparator<SimpleClause> compar;
    SimpleClause r = std::max(r1, r2, compar);
    const SimpleClause& add = std::min(r1, r2, compar);
    r.insert(add.begin(), add.end());
    return r;
  }
}

SimpleClause SimpleClause::PrependActions(const TermSeq& z) const {
  SimpleClause c;
  for (const Literal& l : *this) {
    c.insert(c.end(), l.PrependActions(z));
  }
  return c;
}

SimpleClause SimpleClause::Substitute(const Unifier& theta) const {
  SimpleClause c;
  for (const Literal& l : *this) {
    c.insert(c.end(), l.Substitute(theta));
  }
  return c;
}

SimpleClause SimpleClause::Ground(const Assignment& theta) const {
  SimpleClause c;
  for (const Literal& l : *this) {
    c.insert(c.end(), l.Ground(theta));
  }
  return c;
}

void SimpleClause::SubsumedBy(const const_iterator first,
                              const const_iterator last,
                              const Unifier& theta,
                              std::list<Unifier>* thetas) const {
  if (first == last) {
    thetas->push_back(theta);
    return;
  }
  const Literal l = first->Substitute(theta);
  for (const Literal& ll : range(l.sign(), l.pred())) {
    assert(l.pred() == ll.pred());
    assert(l.sign() == ll.sign());
    Unifier theta2 = theta;
    const bool succ = ll.Matches(l, &theta2);
    if (succ && ll == l.Substitute(theta2)) {
      SubsumedBy(std::next(first), last, theta2, thetas);
    }
  }
}

std::list<Unifier> SimpleClause::Subsumes(const SimpleClause& c) const {
  std::list<Unifier> thetas;
  c.SubsumedBy(begin(), end(), Unifier(), &thetas);
  return thetas;
}

void SimpleClause::GenerateInstances(const Variable::Set::const_iterator first,
                                     const Variable::Set::const_iterator last,
                                     const StdName::SortedSet& hplus,
                                     Assignment* theta,
                                     std::list<SimpleClause>* instances) const {
  if (first != last) {
    const Variable& x = *first;
    StdName::SortedSet::const_iterator ns_it = hplus.find(x.sort());
    assert(ns_it != hplus.end());
    if (ns_it == hplus.end()) {
      return;
    }
    const StdName::Set& ns = ns_it->second;
    for (const StdName& n : ns) {
      (*theta)[x] = n;
      GenerateInstances(std::next(first), last, hplus, theta, instances);
    }
    theta->erase(x);
  } else {
    const SimpleClause c = Ground(*theta);
    assert(c.ground());
    instances->push_back(c);
  }
}

std::list<SimpleClause> SimpleClause::Instances(
    const StdName::SortedSet& hplus) const {
  std::list<SimpleClause> instances;
  Assignment theta;
  const Variable::Set vs = Variables();
  GenerateInstances(vs.begin(), vs.end(), hplus, &theta, &instances);
  return instances;
}

Atom::Set SimpleClause::Sensings() const {
  Atom::Set sfs;
  for (const Literal& l : *this) {
    const TermSeq& z = l.z();
    for (auto it = z.begin(); it != z.end(); ++it) {
      const TermSeq zz(z.begin(), it);
      const Term& t = *it;
      sfs.insert(Atom(zz, Atom::SF, {t}));
    }
  }
  return sfs;
}

Variable::Set SimpleClause::Variables() const {
  Variable::Set vs;
  for (const Literal& l : *this) {
    l.CollectVariables(&vs);
  }
  return vs;
}

void SimpleClause::CollectVariables(Variable::SortedSet* vs) const {
  for (const Literal& l : *this) {
    l.CollectVariables(vs);
  }
}

void SimpleClause::CollectNames(StdName::SortedSet* ns) const {
  for (const Literal& l : *this) {
    l.CollectNames(ns);
  }
}

bool SimpleClause::ground() const {
  for (const Literal& l : *this) {
    if (!l.ground()) {
      return false;
    }
  }
  return true;
}

Clause Clause::InstantiateBox(const TermSeq& z) const {
  assert(box_);
  return Clause(false, e_, ls_.PrependActions(z));
}

Maybe<Clause> Clause::Substitute(const Unifier& theta) const {
  const Maybe<Ewff> e = e_.Substitute(theta);
  if (!e) {
    return Nothing;
  }
  const SimpleClause ls = ls_.Substitute(theta);
  return Just(Clause(box_, e.val, ls));
}

bool Clause::Subsumes(const Clause& c) const {
  if (ls_.empty()) {
    return true;
  }
  if (!box_ && c.box_) {
    return false;
  }
  if ((box_ && !std::includes(c.ls_.begin(), c.ls_.end(),
                              ls_.begin(), ls_.end(),
                              Literal::BySignAndPredOnlyComparator())) ||
      (!box_ && !std::includes(c.ls_.begin(), c.ls_.end(),
                               ls_.begin(), ls_.end(),
                               Literal::BySizeOnlyCompatator()))) {
    return false;
  }
  if (box_) {
    const Literal& l = *ls_.begin();
    for (const Literal& ll : c.ls_.range(l.sign(), l.pred())) {
      assert(l.pred() == ll.pred());
      assert(l.sign() == ll.sign());
      const Maybe<TermSeq> z = ll.z().WithoutLast(l.z().size());
      if (z && InstantiateBox(z.val).Subsumes(c)) {
        return true;
      }
    }
    return false;
  } else {
    for (const Unifier& theta : ls_.Subsumes(c.ls_)) {
      const Maybe<Ewff> e = e_.Substitute(theta);
      if (e && c.e_.Subsumes(e.val)) {
        return true;
      }
    }
    return false;
  }
}

void Clause::Rel(const StdName::SortedSet& hplus,
                 const Literal& goal_lit,
                 std::deque<Literal>* rel) const {
  const size_t max_z = std::accumulate(ls_.begin(), ls_.end(), 0,
      [](size_t n, const Literal& l) { return std::max(n, l.z().size()); });
  const Clause goal = Clause(Ewff::TRUE, {goal_lit.Flip()});
  // Given a clause l and a clause e->c such that for some l' in c with
  // l = l'.theta for some theta and |= e.theta.theta' for some theta', for all
  // clauses l'' c.theta.theta', its negation ~l'' is relevant to l.
  // Relevance means that splitting ~l'' may help to prove l.
  // In case c is a box clause, it must be unified as well.
  // Due to the constrained form of BATs, only l' with maximum action length in
  // e->c must be considered. Intuitively, in an SSA Box([a]F <-> ...) this is
  // [a]F, and in Box(SF(a) <-> ...) it is every literal of the fluent.
  for (const Literal& l : ls_.range(goal_lit.sign(), goal_lit.pred())) {
    assert(goal_lit.sign() == l.sign());
    assert(goal_lit.pred() == l.pred());
    if (l.z().size() < max_z) {
      continue;
    }
    Maybe<Clause> c = ResolveWrt(*this, goal, l, *goal.ls_.begin());
    if (!c) {
      continue;
    }
    for (const Assignment& theta : c.val.e_.Models(hplus)) {
      for (const Literal& ll : c.val.ls_) {
        rel->insert(rel->end(), ll.Ground(theta).Flip());
      }
    }
  }
}

bool Clause::SplitRelevant(const Atom& a, const Clause& c, int k) const {
  assert(k >= 0);
  const SimpleClause::const_iterator it1 = ls_.find(Literal(true, a));
  const SimpleClause::const_iterator it2 = ls_.find(Literal(false, a));
  // ls_.size() - c.size() is a (bad) approximation of (d \ c).size() where d is
  // a unification of ls_ and c
  const int ls_size = static_cast<int>(ls_.size());
  const int c_size = static_cast<int>(c.ls_.size());
  return (it1 != ls_.end() || it2 != ls_.end()) &&
         (ls_size <= k + 1 || ls_size - c_size <= k);
}

Maybe<Clause> Clause::ResolveWrt(const Clause& c1, const Clause& c2,
                                 const Literal& l1, const Literal& l2) {
  assert(l1.sign() != l2.sign());
  assert(c1.ls_.find(l1) != c1.ls_.end());
  assert(c2.ls_.find(l2) != c2.ls_.end());
  if (c1.box_) {
    const Maybe<TermSeq> z = l2.z().WithoutLast(l1.z().size());
    if (!z) {
      return Nothing;
    }
    const Literal ll1 = l1.PrependActions(z.val);
    const Clause cc1 = c1.InstantiateBox(z.val);
    Maybe<Clause> d = ResolveWrt(cc1, c2, ll1, l2);
    if (d) {
      d.val.box_ = c2.box_;
    }
    return d;
  } else if (c2.box_) {
    return ResolveWrt(c2, c1, l2, l1);
  } else {
    const Maybe<Unifier> theta = Atom::Unify(l1, l2);
    if (!theta) {
      return Nothing;
    }
    const Maybe<Ewff> e1 = c1.e_.Substitute(theta.val);
    if (!e1) {
      return Nothing;
    }
    const Maybe<Ewff> e2 = c2.e_.Substitute(theta.val);
    if (!e2) {
      return Nothing;
    }
    const SimpleClause ls =
        SimpleClause::ResolveWrt(c1.ls_, c2.ls_, l1, l2).Substitute(theta.val);
    Ewff e = Ewff::And(e1.val, e2.val);
    e.RestrictVariable(ls.Variables());
    return Just(Clause(false, e, ls));
  }
}

size_t Clause::ResolveWrt(const Clause& c1, const Clause& c2, const Atom& a,
                          Clause::Set* rs) {
  size_t n = 0;
  for (const Literal& l1 : c1.ls_.range(a.pred())) {
    for (const Literal& l2 : c2.ls_.range(!l1.sign(), l1.pred())) {
      assert(l1.pred() == l2.pred());
      assert(l1.sign() != l2.sign());
      Maybe<Clause> c = ResolveWrt(c1, c2, l1, l2);
      if (c) {
        const auto p = rs->insert(c.val);
        if (p.second) {
          ++n;
        }
      }
    }
  }
  return n;
}

void Clause::CollectVariables(Variable::SortedSet* vs) const {
  e_.CollectVariables(vs);
  ls_.CollectVariables(vs);
}

void Clause::CollectNames(StdName::SortedSet* ns) const {
  e_.CollectNames(ns);
  ls_.CollectNames(ns);
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
  if (c.box()) {
    os << "box(";
  }
  os << c.ewff() << " -> " << c.literals();
  if (c.box()) {
    os << ")";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause::Set& cs) {
  os << "{" << std::endl;
  for (const Clause& c : cs) {
    os << ", " << c << std::endl;
  }
  os << "}" << std::endl;
  return os;
}

}  // namespace esbl

