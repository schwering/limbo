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
  bool Entails(int k, const Formula::Reader<T>& phi) {
    g_.PrepareFor(k, phi);
    Setup s = g_.Ground();
    TermSet split_terms = g_.SplitTerms();
    SortedTermSet names = g_.Names();
    return ReduceConjunctions(s, split_terms, names, k, phi);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(KB, general);
#endif

  typedef Grounder::SortedTermSet SortedTermSet;
  typedef Grounder::TermSet TermSet;

  template<typename T>
  bool ReduceConjunctions(const Setup& s,
                          const TermSet& split_terms,
                          const SortedTermSet& names,
                          int k,
                          const Formula::Reader<T>& phi) {
    if (s.Subsumes(Clause{})) {
      return true;
    }
    switch (phi.head().type()) {
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kClause: {
            const Clause c = phi.arg().head().clause().val;
            return std::all_of(c.begin(), c.end(), [this, &s, &split_terms, &names, k](Literal a) {
              a = a.flip();
              Formula psi = Formula::Clause(Clause{a});
              return a.valid() || ReduceConjunctions(s, split_terms, names, k, psi.reader());
            });
          }
          case Formula::Element::kOr: {
            Formula left = Formula::Not(phi.arg().left().Build());
            Formula right = Formula::Not(phi.arg().right().Build());
            return ReduceConjunctions(s, split_terms, names, k, left.reader()) &&
                   ReduceConjunctions(s, split_terms, names, k, right.reader());
          }
          case Formula::Element::kNot: {
            return ReduceConjunctions(s, split_terms, names, k, phi.arg().arg());
          }
          case Formula::Element::kExists: {
            const Term x = phi.arg().head().var().val;
            const Formula::Reader<T> psi = phi.arg().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &split_terms, &names, k, &psi, x](const Term n) {
              Formula xi = Formula::Not(psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build());
              return ReduceConjunctions(s, split_terms, names, k, xi.reader());
            });
          }
          default:
            break;
        }
      }
      default:
        return Split(s, split_terms, names, k, phi);
    }
  }

  template<typename T>
  bool Split(const Setup& s,
             const TermSet& split_terms,
             const SortedTermSet& names,
             int k,
             const Formula::Reader<T>& phi) {
    if (s.Subsumes(Clause{})) {
      return true;
    }
    if (k == 0 || split_terms.empty()) {
      return Reduce(s, names, phi);
    }
    return std::any_of(split_terms.begin(), split_terms.end(), [this, &s, &split_terms, &names, k, &phi](Term t) {
      const TermSet& ns = names[t.sort()];
      assert(!ns.empty());
      return std::all_of(ns.begin(), ns.end(), [this, &s, &split_terms, &names, k, &phi, t](Term n) {
        Setup ss(&s);
        ss.AddClause(Clause{Literal::Eq(t, n)});
        ss.Init();
        Formula psi = phi.Substitute(Term::SingleSubstitution(t, n), tf()).Build();
        return Split(ss, split_terms, names, k-1, psi.reader());
      });
    });
  }

  template<typename T>
  bool Reduce(const Setup& s,
              const SortedTermSet& names,
              const Formula::Reader<T>& phi) {
    if (s.Subsumes(Clause{})) {
      return true;
    }
    switch (phi.head().type()) {
      case Formula::Element::kClause: {
        const Clause& c = phi.head().clause().val;
        return c.valid() || s.Subsumes(c);
      }
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kClause: {
            const Clause c = phi.arg().head().clause().val;
            return std::all_of(c.begin(), c.end(), [&s](Literal a) {
              a = a.flip();
              return a.valid() || s.Subsumes(Clause{a});
            });
          }
          case Formula::Element::kOr: {
            Formula left = Formula::Not(phi.arg().left().Build());
            Formula right = Formula::Not(phi.arg().right().Build());
            return Reduce(s, names, left.reader()) &&
                   Reduce(s, names, right.reader());
          }
          case Formula::Element::kNot: {
            return Reduce(s, names, phi.arg().arg());
          }
          case Formula::Element::kExists: {
            const Term x = phi.arg().head().var().val;
            const Formula::Reader<T> psi = phi.arg().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
              Formula xi = Formula::Not(psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build());
              return Reduce(s, names, xi.reader());
            });
          }
        }
      }
      case Formula::Element::kOr: {
        Formula left = Formula::Not(phi.left().Build());
        Formula right = Formula::Not(phi.right().Build());
        return Reduce(s, names, left.reader()) ||
               Reduce(s, names, right.reader());
      }
      case Formula::Element::kExists: {
        const Term x = phi.head().var().val;
        const Formula::Reader<T> psi = phi.arg();
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
          Formula xi = psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build();
          return Reduce(s, names, xi.reader());
        });
      }
    }
  }

  Symbol::Factory sf_;
  Term::Factory tf_;
  Grounder g_ = Grounder(&sf_, &tf_);
};

}  // namespace lela

#endif  // SRC_KB_H_

