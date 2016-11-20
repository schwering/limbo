// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Solver implements limited belief implications. Splitting is done at a
// deterministic point, namely after reducing the outermost logical operators
// with conjunctive meaning (negated disjunction, double negation, negated
// existential). This is opposed to the original semantics where splitting can
// be done at any point during the reduction.

#ifndef LELA_SOLVER_H_
#define LELA_SOLVER_H_

#include <cassert>

#include <lela/grounder.h>
#include <lela/setup.h>
#include <lela/term.h>

namespace lela {

class Solver {
 public:
  typedef unsigned int split_level;

  Solver() : grounder_(&sf_, &tf_) {}
  Solver(const Solver&) = delete;
  Solver(const Solver&&) = delete;
  Solver& operator=(const Solver&) = delete;

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

  void AddClause(const Clause& c) { grounder_.AddClause(c); }

  const Setup& setup() const { return grounder_.Ground(); }

  template<typename T>
  bool EntailsSound(int k, const Formula::Reader<T>& phi, bool assume_consistent = true) {
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    TermSet split_terms =
      k == 0            ? TermSet() :
      assume_consistent ? grounder_.RelevantSplitTerms(k, phi) :
                          grounder_.SplitTerms();
    SortedTermSet names = grounder_.Names();
    return ReduceConjunctions(s, split_terms, names, k, phi);
  }

  template<typename T>
  bool EntailsComplete(int k, const Formula::Reader<T>& phi, bool assume_consistent = true) {
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    TermSet assign_terms =
      k == 0            ? TermSet() :
      assume_consistent ? grounder_.RelevantSplitTerms(k, phi) :
                          grounder_.SplitTerms();
    SortedTermSet names = grounder_.Names();
    return ReduceDisjunctions(s, assign_terms, names, k, phi, assume_consistent);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(SolverTest, EntailsSound);
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
      default: {
        return Split(s, split_terms, names, k, phi);
      }
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
    if (k > 0) {
      assert(!split_terms.empty());
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
    } else {
      return ReduceSound(s, names, phi);
    }
  }

  template<typename T>
  bool ReduceSound(const Setup& s,
                   const SortedTermSet& names,
                   const Formula::Reader<T>& phi) {
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
            return ReduceSound(s, names, left.reader()) &&
                   ReduceSound(s, names, right.reader());
          }
          case Formula::Element::kNot: {
            return ReduceSound(s, names, phi.arg().arg());
          }
          case Formula::Element::kExists: {
            const Term x = phi.arg().head().var().val;
            const Formula::Reader<T> psi = phi.arg().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
              Formula xi = Formula::Not(psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build());
              return ReduceSound(s, names, xi.reader());
            });
          }
        }
      }
      case Formula::Element::kOr: {
        Formula left = Formula::Not(phi.left().Build());
        Formula right = Formula::Not(phi.right().Build());
        return ReduceSound(s, names, left.reader()) ||
               ReduceSound(s, names, right.reader());
      }
      case Formula::Element::kExists: {
        const Term x = phi.head().var().val;
        const Formula::Reader<T> psi = phi.arg();
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
          Formula xi = psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build();
          return ReduceSound(s, names, xi.reader());
        });
      }
    }
  }

  template<typename T>
  bool ReduceDisjunctions(const Setup& s,
                          const TermSet& assign_terms,
                          const SortedTermSet& names,
                          int k,
                          const Formula::Reader<T>& phi) {
    if (s.Subsumes(Clause{})) {
      return true;
    }
    switch (phi.head().type()) {
      case Formula::Element::kClause: {
        const Clause c = phi.head().clause().val;
        return std::any_of(c.begin(), c.end(), [this, &s, &assign_terms, &names, k](Literal a) {
          Formula psi = Formula::Clause(Clause{a});
          return a.valid() || ReduceDisjunctions(s, assign_terms, names, k, psi.reader());
        });
      }
      case Formula::Element::kOr: {
        Formula::Reader<T> left = phi.left().Build();
        Formula::Reader<T> right = phi.right().Build();
        return ReduceDisjunctions(s, assign_terms, names, k, left) ||
               ReduceDisjunctions(s, assign_terms, names, k, right);
      }
      case Formula::Element::kExists: {
        const Term x = phi.head().var().val;
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &assign_terms, &names, k, &phi, x](const Term n) {
          Formula::Reader<T> psi = phi.Substitute(Term::SingleSubstitution(x, n), tf());
          return ReduceDisjunctions(s, assign_terms, names, k, psi);
        });
      }
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kNot:
            return ReduceDisjunctions(s, assign_terms, names, k, phi.arg().arg());
          default:
            break;
        }
      }
      default: {
        return Assign(s, assign_terms, names, k, phi);
      }
    }
  }

  template<typename T>
  bool Assign(const Setup& s,
              const TermSet& assign_terms,
              const SortedTermSet& names,
              int k,
              const Formula::Reader<T>& phi) {
    if (!s.Consistent()) {
      return true;
    }
    if (k > 0) {
      assert(!assign_terms.empty());
      return std::any_of(assign_terms.begin(), assign_terms.end(), [this, &s, &assign_terms, &names, k, &phi](Term t) {
        const TermSet& ns = names[t.sort()];
        assert(!ns.empty());
        return std::all_of(ns.begin(), ns.end(), [this, &s, &assign_terms, &names, k, &phi, t](Term n) {
          Setup ss(&s);
          if (t.ground()) {
            ss.AddClause(Clause{Literal::Eq(t, n)});
          } else {
            //ss.AddClause(); // XXX TODO
          }
          ss.Init();
          Formula psi = phi.Substitute(Term::SingleSubstitution(t, n), tf()).Build();
          return Assign(ss, assign_terms, names, k-1, psi.reader());
        });
      });
    } else {
      return ReduceComplete(s, names, phi);
    }
  }

  template<typename T>
  bool ReduceComplete(const Setup& s,
              const SortedTermSet& names,
              const Formula::Reader<T>& phi) {
    switch (phi.head().type()) {
      case Formula::Element::kClause: {
        const Clause c = phi.head().clause().val;
        return std::any_of(c.begin(), c.end(), [this, &s, &names](Literal a) {
          Formula psi = Formula::Clause(Clause{a});
          return a.valid() || ReduceComplete(s, names, psi.reader());
        });
      }
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kClause: {
            const Clause c = phi.arg().head().clause().val;
            return !s.Subsumes(c);
          }
          case Formula::Element::kOr: {
            Formula left = Formula::Not(phi.arg().left().Build());
            Formula right = Formula::Not(phi.arg().right().Build());
            return ReduceComplete(s, names, left.reader()) &&
                   ReduceComplete(s, names, right.reader());
          }
          case Formula::Element::kNot: {
            return ReduceComplete(s, names, phi.arg().arg());
          }
          case Formula::Element::kExists: {
            const Term x = phi.arg().head().var().val;
            const Formula::Reader<T> psi = phi.arg().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
              Formula xi = Formula::Not(psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build());
              return ReduceComplete(s, names, xi.reader());
            });
          }
        }
      }
      case Formula::Element::kOr: {
        Formula left = Formula::Not(phi.left().Build());
        Formula right = Formula::Not(phi.right().Build());
        return ReduceComplete(s, names, left.reader()) ||
               ReduceComplete(s, names, right.reader());
      }
      case Formula::Element::kExists: {
        const Term x = phi.head().var().val;
        const Formula::Reader<T> psi = phi.arg();
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
          Formula xi = psi.Substitute(Term::SingleSubstitution(x, n), tf()).Build();
          return ReduceComplete(s, names, xi.reader());
        });
      }
    }
  }

  Symbol::Factory sf_;
  Term::Factory tf_;
  Grounder grounder_;
};

}  // namespace lela

#endif  // LELA_SOLVER_H_

