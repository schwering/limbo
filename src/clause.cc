// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./clause.h"
#include <algorithm>
#include <numeric>

Clause Clause::PrependActions(const TermSeq& z) const {
  std::set<Literal> ls;
  for (const Literal& l : ls_) {
    ls.insert(l.PrependActions(z));
  }
  return Clause(box_, e_, ls);
}

std::pair<bool, Clause> Clause::Substitute(const Unifier& theta) const {
  std::pair<bool, Ewff> p = e_.Substitute(theta);
  if (!p.first) {
    return std::make_pair(false, *this);
  }
  std::set<Literal> ls;
  for (const Literal& l : ls_) {
    ls.insert(l.Substitute(theta));
  }
  return std::make_pair(true, Clause(box_, p.second, ls));
}

namespace {
std::tuple<bool, TermSeq, Unifier> BoxUnify(const Literal &z_lit,
                                            const Literal& box_lit) {
  const size_t split = box_lit.z().size();
  if (split > z_lit.z().size() ||
      !std::equal(box_lit.z().begin(), box_lit.z().end(),
                  z_lit.z().begin() + split)) {
    return std::make_tuple(false, TermSeq(), Unifier());
  }
  const Literal lit = z_lit.DropActions(split);
  std::pair<bool, Unifier> p = Atom::Unify(lit, box_lit);
  if (!p.first) {
    return std::make_tuple(false, TermSeq(), Unifier());
  }
  const Unifier& theta = p.second;
  const TermSeq prefix(z_lit.z().begin(), z_lit.z().begin() + split);
  return std::make_tuple(true, prefix, theta);
}
}  // namespace

std::set<Literal> Clause::Rel(const StdName::SortedSet& hplus,
                              const Literal &rel_lit) const {
  const size_t max_z = std::accumulate(ls_.begin(), ls_.end(), 0,
      [](size_t n, const Literal& l) { return std::max(n, l.z().size()); });
  std::set<Literal> rel;
  for (const Literal& l : ls_) {
    if (rel_lit.sign() != l.sign() ||
        rel_lit.pred() != l.pred() ||
        l.z().size() < max_z) {
      continue;
    }
    bool succ;
    TermSeq z;
    Unifier theta;
    std::tie(succ, z, theta) = BoxUnify(rel_lit, l);
    if (!succ) {
      continue;
    }
    Clause c;
    std::tie(succ, c) = Substitute(theta);
    if (!succ) {
      continue;
    }
    for (const Assignment& theta : c.PrependActions(z).e_.Models(hplus)) {
      for (const Literal& ll : c.ls_) {
        rel.insert(ll.Ground(theta).Negative());
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

