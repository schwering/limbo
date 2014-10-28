// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./setup.h"

namespace esbl {

std::set<Literal> Setup::Rel(const StdName::SortedSet& hplus,
                             const SimpleClause& c) const {
  std::set<Literal> rel = c;
  std::set<Literal> new_rel = rel;
  while (!new_rel.empty()) {
    for (const Literal& l : new_rel) {
      for (const Clause& c : cs_) {
        for (const Literal& ll : c.Rel(hplus, l)) {
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

std::set<Atom> Setup::Pel(const StdName::SortedSet& hplus,
                          const SimpleClause& c) const {
  const std::set<Literal> rel = Rel(hplus, c);
  std::set<Atom> pel;
  for (auto it = rel.begin(); it != rel.end(); ++it) {
    const Literal& l = *it;
    if (!l.sign()) {
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
  return os;
}

}  // namespace esbl

