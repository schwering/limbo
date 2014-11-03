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

bool Setup::ContainsEmptyClause() const {
  const auto it = cs_.begin();
  return it != cs_.end() && it->literals().size() == 0;
}

std::set<Clause> Setup::UnitClauses() const {
  std::set<Clause> units;
  const auto first = cs_.lower_bound(Clause::MIN_UNIT);
  const auto last = cs_.upper_bound(Clause::MAX_UNIT);
  for (auto it = first; it != last; ++it) {
    assert(it->is_unit());
    units.insert(units.end(), *it);
  }
  return units;
}

void Setup::PropagateUnits() {
  if (ContainsEmptyClause()) {
    return;
  }
  std::set<Clause> ucs = UnitClauses();
  std::set<Clause> new_ucs;
  while (!ucs.empty()) {
    new_ucs.clear();
    for (auto it = cs_.begin(); it != cs_.end(); ) {
      const Clause& c = *it;
      bool remove = false;
      //for (const Clause& d : c.ResolveWithUnitClauses(ucs)) {
      //  remove |= c.ewff() == d.ewff();
      //}
      if (remove) {
        it = cs_.erase(it);
      } else {
        ++it;
      }
    }
    std::swap(ucs, new_ucs);
  }

  /*
    while (splitset_size(units_ptr) > 0) {
        splitset_clear(new_units_ptr);
        for (EACH(clauses, &setup->clauses, i)) {
            const clause_t *c = i.val;
            const clause_t *d = clause_resolve(c, units_ptr);
            if (!clause_eq(c, d)) {
                const int j = clauses_iter_replace(&i, d);
                if (j >= 0) {
                    setup_minimize_wrt(setup, d, &i);
                    if (setup_contains_empty_clause(setup)) {
                        return;
                    }
                    if (clause_is_unit(d)) {
                        const literal_t *l = clause_unit(d);
                        if (!splitset_contains(units_ptr, l)) {
                            splitset_add(new_units_ptr, l);
                        }
                    }
                }
            }
        }
        SWAP(units_ptr, new_units_ptr);
    }
    */
}

void Setup::Minimize() {
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

