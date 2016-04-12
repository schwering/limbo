// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_GROUNDER_H_
#define SRC_GROUNDER_H_

#include <algorithm>
#include <map>
#include <vector>
#include <utility>
#include "./clause.h"
#include "./iter.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::vector<Term> VarSet;
  typedef std::map<Symbol::Sort, size_t> PlusMap;

  Grounder() = delete;

  template<typename Range>
  static Setup Ground(const Range& range, const PlusMap& plus) {
    Setup s;
    SortedNames names = Names(range, plus);
    VarSet vars = Variables(range);
    return s;
  }

 private:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;

  template<typename Range>
  static SortedNames Names(const Range& range, const PlusMap& plus) {
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
        names.insert(std::make_pair(sort, Term::Create(Symbol::CreateName(new_id, sort))));
      }
    }
    return names;
  }

  template<typename Range>
  static VarSet Variables(const Range& range) {
    VarSet vars;
    for (const Clause& c : range) {
      c.Traverse([&vars](Term t) { if (t.variable()) { vars.push_back(t); } return true; });
    }
    std::sort(vars.begin(), vars.end(), Term::Comparator());
    vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    return vars;
  }
};

}  // namespace lela

#endif  // SRC_GROUNDER_H_

