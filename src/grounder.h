// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Substitutes standard names for variables. Corresponds to the gnd() operator.

#ifndef SRC_GROUNDER_H_
#define SRC_GROUNDER_H_

#include <cassert>
#include <algorithm>
#include <map>
#include <vector>
#include <utility>
#include "./clause.h"
#include "./formula.h"
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;

  template<typename ClauseRange, typename T>
  static SortedNames Names(const ClauseRange& cs, const Formula::Reader<T>& phi, Term::Factory* tf) {
    SortedNames names = MentionedNames(cs, phi);
    AddPlusNames(PlusNames(cs, phi), tf, &names);
    return names;
  }

  template<typename ClauseRange, typename T>
  static Setup Ground(const ClauseRange& cs, const SortedNames& names, Term::Factory* tf) {
    Setup s;
    for (const Clause& c : cs) {
      const VariableSet vars = MentionedVariables(c);
      for (VariableMapping mapping(&names, vars); mapping.has_next(); ++mapping) {
        s.AddClause(c.Substitute(mapping, tf));
      }
    }
    return s;
  }

  Grounder() = delete;

 private:
  typedef std::vector<Term> VariableSet;
  typedef IntMap<Symbol::Sort, size_t, 0> PlusMap;

  template<typename ClauseRange, typename T>
  static SortedNames MentionedNames(const ClauseRange& cs, const Formula::Reader<T>& phi) {
    SortedNames names;
    for (const Clause& c : cs) {
      c.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    }
    phi.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    return names;
  }

  static VariableSet MentionedVariables(const Clause& c) {
    VariableSet vars;
    c.Traverse([&vars](Term t) { if (t.variable()) { vars.push_back(t); } return true; });
    std::sort(vars.begin(), vars.end(), Term::Comparator());
    vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    return vars;
  }

  template<typename ClauseRange, typename T>
  static PlusMap PlusNames(const ClauseRange& cs, const Formula::Reader<T>& phi) {
    return PlusMap::Zip(PlusMap(cs), PlusMap(phi), [](size_t p1, size_t p2) { return p1 + p2; });
  }

  template<typename ClauseRange>
  static PlusMap PlusNames(const ClauseRange& cs) {
    PlusMap plus;
    for (const Clause& c : cs) {
      for (const auto p : PlusNames(c)) {
        plus[p.first] = std::max(plus[p.first], p.second);
      }
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    PlusMap plus;
    const VariableSet vars = MentionedVariables(c);
    for (const Term var : vars) {
      ++plus[var.symbol().sort()];
    }
    return plus;
  }

  template<typename T>
  static PlusMap PlusNames(const Formula::Reader<T>& phi) {
    // Roughly, we need to add one name for each quantifier. More precisely,
    // it suffices to check for every sort which is the maximal number of
    // different variables occurring freely in any subformula of phi. We do
    // so from the inside to the outside, determining the number of free
    // variables of any sort in cur, and the maximum in max.
    PlusMap max;
    PlusMap cur;
    PlusNames(phi, &cur, &max);
    return max;
  }

  template<typename T>
  static void PlusNames(const Formula::Reader<T>& phi, PlusMap* cur, PlusMap* max) {
    switch (phi.head().type()) {
      case Formula::Element::kClause:
        *cur = PlusNames(phi.head().clause().val);
        *max = *cur;
        break;
      case Formula::Element::kNot:
        PlusNames(phi.arg(), cur, max);
        break;
      case Formula::Element::kOr: {
        PlusMap lcur, lmax;
        PlusMap rcur, rmax;
        PlusNames(phi.left(), &lcur, &lmax);
        PlusNames(phi.right(), &rcur, &rmax);
        *cur = PlusMap::Zip(lcur, rcur, [](size_t lp, size_t rp) { return lp + rp; });
        *max = PlusMap::Zip(lmax, rmax, [](size_t lp, size_t rp) { return std::max(lp, rp); });
        *max = PlusMap::Zip(*max, *cur, [](size_t mp, size_t cp) { return std::max(mp, cp); });
        break;
      }
      case Formula::Element::kExists:
        PlusNames(phi.arg(), cur, max);
        Symbol::Sort sort = phi.head().var().val.symbol().sort();
        if ((*cur)[sort] > 0) {
          --(*cur)[sort];
        }
        break;
    }
  }

  template<typename ClauseRange>
  static void AddPlusNames(const PlusMap& plus, Term::Factory* tf, SortedNames* names) {
    for (auto p : plus) {
      const Symbol::Sort sort = p.first;
      const size_t plus = p.second;
      auto first_name = names->lower_bound(sort);
      auto last_name  = names->upper_bound(sort);
      auto max_name = std::max_element(first_name, last_name, [](SortedNames::value_type p1, SortedNames::value_type p2) { return p1.second.symbol().id() < p2.second.symbol().id(); });
      Symbol::Id next_id = max_name != last_name ? max_name->second.symbol().id() + 1 : 1;
      for (Symbol::Id new_id = next_id; new_id < next_id + static_cast<Symbol::Id>(plus); ++new_id) {
        // By specifying new_id, we avoid that the additional names are registered in the symbol factory.
        // Registering these names would be a waste of IDs, but otherwise not too bad.
        names->insert(std::make_pair(sort, tf->CreateTerm(Symbol::Factory::CreateName(new_id, sort))));
      }
    }
  }

  class VariableMapping {
   public:
    typedef SortedNames::const_iterator name_iterator;
    typedef std::pair<name_iterator, name_iterator> name_range;
    struct Get {
      std::pair<Term, Term> operator()(const std::pair<Term, name_range> p) const {
        const Term& var = p.first;
        const name_range& r = p.second;
        assert(r.first != r.second);
        assert(var.symbol().sort() == r.first->first);
        assert(var.symbol().sort() == r.first->second.symbol().sort());
        const Term& name = r.first->second;
        assert(name.name());
        return std::make_pair(var, name);
      }
    };
    typedef transform_iterator<Get, std::map<Term, name_range>::const_iterator> iterator;

    VariableMapping(const SortedNames* names, const VariableSet& vars) : names_(names) {
      for (const Term var : vars) {
        assert(var.symbol().variable());
        const name_range r = names_->equal_range(var.symbol().sort());
        assert(r.first != r.second);
        assert(var.symbol().sort() == r.first->first);
        assert(var.symbol().sort() == r.first->second.symbol().sort());
        assignment_[var] = r;
      }
      meta_iter_ = assignment_.begin();
    }

    Maybe<Term> operator()(Term v) const {
      auto it = assignment_.find(v);
      if (it != assignment_.end()) {
        auto r = it->second;
        assert(r.first != r.second);
        const Term& name = r.first->second;
        assert(name.name());
        return Just(name);
      } else {
        return Nothing;
      }
    }

    VariableMapping& operator++() {
      assert(meta_iter_ != assignment_.end());
      for (meta_iter_ = assignment_.begin(); meta_iter_ != assignment_.end(); ++meta_iter_) {
        const Term var = meta_iter_->first;
        name_range& r = meta_iter_->second;
        assert(var.symbol().variable());
        assert(r.first != r.second);
        ++r.first;
        if (r.first != r.second) {
          break;
        } else {
          r.first = names_->lower_bound(var.symbol().sort());
          assert(r.first != r.second);
          assert(var.symbol().sort() == r.first->first);
          assert(var.symbol().sort() == r.first->second.symbol().sort());
        }
      }
      return *this;
    }

    bool has_next() const { return meta_iter_ != assignment_.end(); }

    iterator begin() const { return iterator(Get(), assignment_.begin()); }
    iterator end()   const { return iterator(Get(), assignment_.end()); }

   private:
    const SortedNames* names_;
    std::map<Term, name_range> assignment_;
    std::map<Term, name_range>::iterator meta_iter_;
  };
};

}  // namespace lela

#endif  // SRC_GROUNDER_H_

