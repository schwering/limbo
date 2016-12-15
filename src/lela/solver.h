// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Solver implements limited belief implications. The key methods are Entails()
// and Satisfies(), which determine whether the knowledge base consistent of
// the clauses added with AddClause() entail a query or are consistent with it,
// respectively. Entails() and Satisfies() both are sound but incomplete: if
// they return true, this answer is correct with respect to classical logic;
// if they return false, this may not be correct and should be rather
// interpreted as "don't know." The method EntailsComplete() uses Consistent()
// to implement a complete but unsound entailment relation. It is safe to call
// AddClause() between evaluating queries with Entails(), EntailsComplete(), or
// Consistent().
//
// Splitting and assigning is done at a deterministic point, namely after
// reducing the outermost logical operators with conjunctive meaning (negated
// disjunction, double negation, negated existential). This is opposed to the
// original semantics from the KR-2016 paper where splitting can be done at any
// point during the reduction.
//
// In the original semantics, when a split sets (t = n), we also substitute n
// for t in the query to deal with nested terms. But since we often split before
// reducing quantifiers, t might occur later in the query only after quantifiers
// are reduced. Substituting n for t at splitting time is hence not sufficient.
// For that reason, we defer that substitution until the query is reduced to a
// clause for which subsumption is to be checked. Then we check for any nested
// term t in that clause whether its denotation is defined by a unit clause
// (t = n) in the setup, in which case we substitute n for t in the clause.
// Note that the unit clause does not need to come from a split. Hence we
// may even save some trivial splits, e.g., from [f(n) = n], [g(n) = n]
// we infer without split that [f(g(n)) = n].
//
// While the ECAI-2016 paper complements the sound but incomplete entailment
// relation with a complete but unsound entailment relation, Solver provides
// a sound but incomplete consistency check. It is easy to see that this is
// equivalent: Consistent(k, phi) == !EntailsComplete(k, Not(phi)) and
// EntailsComplete(k, phi)) == !Consistent(k, Not(phi)). The advantage of
// the Consistent() method is that it is perhaps less confusing and less prone
// to typos and shares some code with the sound Entails().

#ifndef LELA_SOLVER_H_
#define LELA_SOLVER_H_

#include <cassert>

#include <list>

#include <lela/grounder.h>
#include <lela/setup.h>
#include <lela/term.h>

namespace lela {

class Solver {
 public:
  typedef unsigned int split_level;

  Solver() : grounder_(&sf_, &tf_) {}
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;
  Solver(Solver&&) = default;
  Solver& operator=(Solver&&) = default;

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

  void AddClause(const Clause& c) { grounder_.AddClause(c); }

  const Setup& setup() const { return grounder_.Ground(); }

  template<typename T>
  bool Entails(int k, const Formula::Reader<T>& phi, bool assume_consistent = true) {
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    TermSet split_terms =
      k == 0            ? TermSet() :
      assume_consistent ? grounder_.RelevantSplitTerms(k, phi) :
                          grounder_.SplitTerms();
    SortedTermSet names = grounder_.Names();
    return s.Subsumes(Clause{}) || ReduceConjunctions(s, split_terms, names, k, phi);
  }

  template<typename T>
  bool EntailsComplete(int k, const Formula::Reader<T>& phi) {
    return !Consistent(k, Formula::Not(phi.Build()).reader());
  }

  template<typename T>
  bool Consistent(int k, const Formula::Reader<T>& phi) {
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    std::list<LiteralSet> assign_lits = k == 0 ? std::list<LiteralSet>() : grounder_.AssignLiterals();
    SortedTermSet names = grounder_.Names();
    return !s.Subsumes(Clause{}) && ReduceDisjunctions(s, assign_lits, names, k, phi);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(SolverTest, Constants);
#endif

  typedef Grounder::TermSet TermSet;
  typedef Grounder::LiteralSet LiteralSet;
  typedef Grounder::SortedTermSet SortedTermSet;

  template<typename T>
  bool ReduceConjunctions(const Setup& s,
                          const TermSet& split_terms,
                          const SortedTermSet& names,
                          int k,
                          const Formula::Reader<T>& phi) {
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
          case Formula::Element::kNot: {
            return ReduceConjunctions(s, split_terms, names, k, phi.arg().arg());
          }
          case Formula::Element::kOr: {
            Formula left = Formula::Not(phi.arg().left().Build());
            Formula right = Formula::Not(phi.arg().right().Build());
            return ReduceConjunctions(s, split_terms, names, k, left.reader()) &&
                   ReduceConjunctions(s, split_terms, names, k, right.reader());
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
    } else if (k > 0) {
      assert(!split_terms.empty());
      return std::any_of(split_terms.begin(), split_terms.end(), [this, &s, &split_terms, &names, k, &phi](Term t) {
        const TermSet& ns = names[t.sort()];
        assert(!ns.empty());
        return std::all_of(ns.begin(), ns.end(), [this, &s, &split_terms, &names, k, &phi, t](Term n) {
          Setup ss(&s);
          ss.AddClause(Clause{Literal::Eq(t, n)});
#if 0
          ss.Init();
#endif
          return Split(ss, split_terms, names, k-1, phi);
        });
      });
    } else {
      return Reduce(s, names, phi);
    }
  }

  template<typename T>
  bool ReduceDisjunctions(const Setup& s,
                          const std::list<LiteralSet>& assign_lits,
                          const SortedTermSet& names,
                          int k,
                          const Formula::Reader<T>& phi) {
    switch (phi.head().type()) {
      case Formula::Element::kClause: {
        return Assign(s, assign_lits, names, k, phi);
      }
      case Formula::Element::kOr: {
        Formula::Reader<T> left = phi.left();
        Formula::Reader<T> right = phi.right();
        return ReduceDisjunctions(s, assign_lits, names, k, left) ||
               ReduceDisjunctions(s, assign_lits, names, k, right);
      }
      case Formula::Element::kExists: {
        const Term x = phi.head().var().val;
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &assign_lits, &names, k, &phi, x](const Term n) {
          Formula psi = phi.arg().Substitute(Term::SingleSubstitution(x, n), tf()).Build();
          return ReduceDisjunctions(s, assign_lits, names, k, psi.reader());
        });
      }
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kNot:
            return ReduceDisjunctions(s, assign_lits, names, k, phi.arg().arg());
          default:
            return Assign(s, assign_lits, names, k, phi);
        }
      }
    }
  }

  template<typename T>
  bool Assign(const Setup& s,
              const std::list<LiteralSet>& assign_lits,
              const SortedTermSet& names,
              int k,
              const Formula::Reader<T>& phi) {
    if (k > 0) {
      assert(!assign_lits.empty());
      return std::any_of(assign_lits.begin(), assign_lits.end(),
                         [this, &s, &assign_lits, &names, k, &phi](const LiteralSet& lits) {
        assert(!lits.empty());
        Setup ss(&s);
        for (Literal a : lits) {
          Clause c{a};
          if (!ss.Subsumes(c)) {
            ss.AddClause(c);
          }
        }
#if 0
        ss.Init();
#endif
        return Assign(ss, assign_lits, names, k-1, phi);
      });
    } else {
      return s.Consistent() && Reduce(s, names, phi);
    }
  }

  template<typename T>
  bool Reduce(const Setup& s, const SortedTermSet& names, const Formula::Reader<T>& phi) {
    switch (phi.head().type()) {
      case Formula::Element::kClause: {
        const Clause c = ResolveDeterminedTerms(s, phi.head().clause().val);
        return c.valid() || (c.primitive() && s.Subsumes(c));
      }
      case Formula::Element::kNot: {
        switch (phi.arg().head().type()) {
          case Formula::Element::kClause: {
            const Clause c = phi.arg().head().clause().val;
            return std::all_of(c.begin(), c.end(), [this, &s, &names](Literal a) {
              Formula psi = Formula::Clause(Clause{a.flip()});
              return Reduce(s, names, psi.reader());
            });
          }
          case Formula::Element::kNot: {
            return Reduce(s, names, phi.arg().arg());
          }
          case Formula::Element::kOr: {
            Formula left = Formula::Not(phi.arg().left().Build());
            Formula right = Formula::Not(phi.arg().right().Build());
            return Reduce(s, names, left.reader()) &&
                   Reduce(s, names, right.reader());
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
        Formula::Reader<T> left = phi.left();
        Formula::Reader<T> right = phi.right();
        return Reduce(s, names, left) ||
               Reduce(s, names, right);
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

  Clause ResolveDeterminedTerms(const Setup& s, Clause c) {
    assert(c.ground());
    bool changed;
    do {
      changed = false;
      c = c.Substitute([&s, &changed](const Term t) -> internal::Maybe<Term> {
        if (t.primitive()) {
          for (Setup::Index i : s.clauses()) {
            if (s.clause(i).unit()) {
              Literal a = *s.clause(i).begin();
              if (a.pos() && a.lhs() == t) {
                changed = true;
                return internal::Just(a.rhs());
              }
            }
          }
        }
        return internal::Nothing;
      }, &tf_);
    } while (changed);
    return c;
  }

  Symbol::Factory sf_;
  Term::Factory tf_;
  Grounder grounder_;
};

}  // namespace lela

#endif  // LELA_SOLVER_H_

