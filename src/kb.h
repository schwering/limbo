// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A Grounder determines how many standard names need to be substituted for
// variables in a proper+ knowledge base and in queries.

#ifndef SRC_KB_H_
#define SRC_KB_H_

#include <cassert>
#include "./grounder.h"
#include "./setup.h"
#include "./term.h"

namespace lela {

class KB {
 public:
  void AddClause(const Clause& c) { g_.AddClause(c); }

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

  template<typename T>
  bool Satisfies(int k, const Formula::Reader<T>& phi) const {
    g_.PrepareFor(k, phi);
    Setup s = g_.Ground();
    Grounder::TermSet split_terms = g_.SplitTerms();
    Grounder::SortedNames split_names = g_.SplitNames();
    return Satisfies(s, split_terms, split_names, k, phi);
  }

 private:
  template<typename T>
  bool satisfies(const Setup& s,
                 const Grounder::TermSet& split_terms,
                 const Grounder::SortedNames& split_names,
                 int k,
                 const Formula::Reader<T>& phi) {
    if (s.Subsumes(Clause{})) {
      return true;
    }
    for (Term t : split_terms) {
      bool r = false;
      for (Term n : split_names[t.symbol().sort()]) {
      }
    }
  }

  Symbol::Factory sf_;
  Term::Factory tf_;
  Grounder g_ = Grounder(&sf_, &tf_);
};

}  // namespace lela

#endif  // SRC_KB_H_

