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
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::vector<Term> VariableSet;
  typedef std::map<Symbol::Sort, size_t> PlusMap;

  template<typename Range>
  static Setup Ground(const Range& range, const PlusMap& plus, Term::Factory* tf) {
    Setup s;
    SortedNames names = Names(range, plus, tf);
    for (const Clause& c : range) {
      const VariableSet vars = Variables(c);
      for (VariableMapping mapping(&names, vars); mapping.has_next(); ++mapping) {
        s.AddClause(c.Substitute(mapping, tf));
      }
    }
    return s;
  }

  Grounder() = delete;

 private:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;

  // The additional names are not registered in the symbol factory.
  template<typename Range>
  static SortedNames Names(const Range& range, const PlusMap& plus, Term::Factory* tf) {
    SortedNames names;
    for (const Clause& c : range) {
      c.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    }
    for (auto p : plus) {
      const Symbol::Sort sort = p.first;
      const Symbol::Id plus = p.second;
      auto first_name = names.lower_bound(sort);
      auto last_name  = names.upper_bound(sort);
      auto max_name = std::max_element(first_name, last_name, [](SortedNames::value_type p1, SortedNames::value_type p2) { return p1.second.symbol().id() < p2.second.symbol().id(); });
      Symbol::Id next_id = max_name != last_name ? max_name->second.symbol().id() + 1 : 1;
      for (Symbol::Id new_id = next_id; new_id < next_id + plus; ++new_id) {
        names.insert(std::make_pair(sort, tf->CreateTerm(Symbol::Factory::CreateName(new_id, sort))));
      }
    }
    return names;
  }

  static VariableSet Variables(const Clause& c) {
    VariableSet vars;
    c.Traverse([&vars](Term t) { if (t.variable()) { vars.push_back(t); } return true; });
    std::sort(vars.begin(), vars.end(), Term::Comparator());
    vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    return vars;
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
        return Just(name);
      }
      return Nothing;
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

