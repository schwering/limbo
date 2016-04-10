// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_GROUNDER_H_
#define SRC_GROUNDER_H_

#include <algorithm>
#include <vector>
#include <utility>
#include "./clause.h"
#include "./iter.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::map<Symbol::Sort, size_t> PlusMap;

  Grounder() = delete;

  template<typename Range>
  static Setup Ground(const Range& range, PlusMap& plus) {
    Setup s;
    SortedNames names = Names(range, plus);
    return s;
  }

 private:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;

  template<typename Range>
  static SortedNames Names(const Range& range, const PlusMap& plus) {
    SortedNames names;
    for (const Clause& c : range) {
      c.Collect([](Term t) { return t.name(); }, [](Term t) { return std::make_pair(t.symbol().sort(), t); }, &names);
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
};

}  // namespace lela

#endif  // SRC_GROUNDER_H_

