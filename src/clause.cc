// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./clause.h"
#include <algorithm>
#include <numeric>

Clause Clause::PrependActions(const TermSeq& z) const {
  GroundClause ls;
  for (const Literal& l : ls_) {
    ls.insert(l.PrependActions(z));
  }
  return Clause(box_, e_, ls);
}

std::pair<bool, Clause> Clause::Substitute(const Unifier& theta) const {
  std::pair<bool, Ewff> p = e_.Substitute(theta);
  if (!p.first) {
    return std::make_pair(false, Clause());
  }
  GroundClause ls;
  for (const Literal& l : ls_) {
    ls.insert(l.Substitute(theta));
  }
  return std::make_pair(true, Clause(box_, p.second, ls));
}

namespace {
std::tuple<bool, TermSeq, Unifier> BoxUnify(const Atom& cl_a,
                                            const Atom& ext_a) {
  const size_t split = cl_a.z().size();
  if (split > ext_a.z().size() ||
      !std::equal(cl_a.z().begin(), cl_a.z().end(),
                  ext_a.z().begin() + split)) {
    return std::make_tuple(false, TermSeq(), Unifier());
  }
  const Atom a = ext_a.DropActions(split);
  std::pair<bool, Unifier> p = Atom::Unify(a, cl_a);
  if (!p.first) {
    return std::make_tuple(false, TermSeq(), Unifier());
  }
  const Unifier& theta = p.second;
  const TermSeq prefix(ext_a.z().begin(), ext_a.z().begin() + split);
  return std::make_tuple(true, prefix, theta);
}
}  // namespace

std::pair<bool, Clause> Clause::Unify(const Atom& cl_a,
                                      const Atom& ext_a) const {
  if (box_) {
    bool succ;
    TermSeq z;
    Unifier theta;
    std::tie(succ, z, theta) = BoxUnify(cl_a, ext_a);
    if (!succ) {
      return std::make_pair(false, Clause());
    }
    Clause c;
    std::tie(succ, c) = Substitute(theta);
    if (!succ) {
      return std::make_pair(false, Clause());
    }
    return std::make_pair(true, c.PrependActions(z));
  } else {
    bool succ;
    Unifier theta;
    std::tie(succ, theta) = Atom::Unify(cl_a, ext_a);
    if (!succ) {
      return std::make_pair(false, Clause());
    }
    Clause c;
    std::tie(succ, c) = Substitute(theta);
    if (!succ) {
      return std::make_pair(false, Clause());
    }
    return std::make_pair(true, c);
  }
}

GroundClause Clause::Rel(const StdName::SortedSet& hplus,
                         const Literal &ext_l) const {
  const size_t max_z = std::accumulate(ls_.begin(), ls_.end(), 0,
      [](size_t n, const Literal& l) { return std::max(n, l.z().size()); });
  GroundClause rel;
  // Given a clause l and a clause e->c such that for some l' in c with
  // l = l'.theta for some theta and |= e.theta.theta' for some theta', for all
  // clauses l'' c.theta.theta', its negation ~l'' is relevant to l.
  // Relevance means that splitting ~l'' may help to prove l.
  // In case c is a box clause, it must be unified as well.
  // Due to the constrained form of BATs, only l' with maximum action length in
  // e->c must be considered. Intuitively, in an SSA Box([a]F <-> ...) this is
  // [a]F, and in Box(SF(a) <-> ...) it is every literal of the fluent.
  for (const Literal& l : ls_) {
    if (ext_l.sign() != l.sign() ||
        ext_l.pred() != l.pred() ||
        l.z().size() < max_z) {
      continue;
    }
    bool succ;
    Clause c;
    std::tie(succ, c) = Unify(l, ext_l);
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

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  os << c.ewff() << " -> (";
  for (auto it = c.literals().begin(); it != c.literals().end(); ++it) {
    if (it != c.literals().begin()) {
      os << " v ";
    }
    os << *it;
  }
  os << ")";
  return os;
}

