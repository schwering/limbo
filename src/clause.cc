// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./clause.h"
#include <algorithm>
#include <numeric>

namespace esbl {

Clause Clause::PrependActions(const TermSeq& z) const {
  GroundClause ls;
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
  GroundClause ls;
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
  assert(ext_l.is_ground());
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
    Clause c;
    std::tie(succ, std::ignore, c) = Unify(cl_l, ext_l);
    if (!succ) {
      continue;
    }
    for (const Assignment& theta : c.e_.Models(hplus)) {
      for (const Literal& ll : c.ls_) {
        rel.insert(ll.Ground(theta).Flip());
      }
    }
  }
  return rel;
}

bool Clause::Subsumes(const GroundClause& c) const {
  if (ls_.empty()) {
    return true;
  }
  const GroundClause::const_iterator cl_l_it = ls_.begin();
  const Literal& cl_l = *cl_l_it;
  for (const Literal& ext_l : c) {
    assert(ext_l.is_ground());
    if (ext_l.sign() != cl_l.sign() ||
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
    size_t n = d.ls_.erase(ext_l.Substitute(theta));
    assert(n >= 1);
    if (d.Subsumes(c)) {
      return true;
    }
  }
  return false;
}

bool Clause::SplitRelevant(const Atom& a, const GroundClause c,
                           unsigned int k) const {
  const GroundClause::const_iterator it1 = ls_.find(Literal(true, a));
  const GroundClause::const_iterator it2 = ls_.find(Literal(false, a));
  // ls_.size() - c.size() is a (bad) approximation of (d \ c).size() where d is
  // a unification of ls_ and c
  return (it1 != ls_.end() || it2 != ls_.end()) &&
         (ls_.size() <= k + 1 || ls_.size() - c.size() <= k);
}

std::list<Clause> Clause::PropagateUnit(const Literal& ext_l) const {
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

std::ostream& operator<<(std::ostream& os, const GroundClause& c) {
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

}

