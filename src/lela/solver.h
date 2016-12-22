// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Solver implements limited belief implications. The key methods are Entails()
// and Consistent(), which determine whether the knowledge base consisting of
// the clauses added with AddClause() entails a query or is consistent with it,
// respectively. Entails() and Consistent() both are sound but incomplete: if
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
  typedef Formula::split_level split_level;

  Solver(Symbol::Factory* sf, Term::Factory* tf) : tf_(tf), grounder_(sf, tf) {}
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;
  Solver(Solver&&) = default;
  Solver& operator=(Solver&&) = default;

  void AddClause(const Clause& c) { grounder_.AddClause(c); }

  const Setup& setup() const { return grounder_.Ground(); }

  const Grounder::SortedTermSet& names() { return grounder_.Names(); }

  bool Entails(int k, const Formula& phi, bool assume_consistent = true) {
    assert(phi.objective());
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    TermSet split_terms =
      k == 0            ? TermSet() :
      assume_consistent ? grounder_.RelevantSplitTerms(k, phi) :
                          grounder_.SplitTerms();
    const SortedTermSet& names = grounder_.Names();
    return s.Subsumes(Clause{}) || ReduceConjunctions(s, split_terms, names, k, phi);
  }

  bool EntailsComplete(int k, const Formula& phi) {
    assert(phi.objective());
    Formula::Ref psi = Formula::Factory::Not(phi.Clone());
    return !Consistent(k, *psi);
  }

  bool Consistent(int k, const Formula& phi) {
    assert(phi.objective());
    grounder_.PrepareForQuery(k, phi);
    const Setup& s = grounder_.Ground();
    std::list<LiteralSet> assign_lits = k == 0 ? std::list<LiteralSet>() : grounder_.AssignLiterals();
    const SortedTermSet& names = grounder_.Names();
    return !s.Subsumes(Clause{}) && ReduceDisjunctions(s, assign_lits, names, k, phi);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(SolverTest, Constants);
#endif

  typedef Grounder::TermSet TermSet;
  typedef Grounder::LiteralSet LiteralSet;
  typedef Grounder::SortedTermSet SortedTermSet;

  bool ReduceConjunctions(const Setup& s,
                          const TermSet& split_terms,
                          const SortedTermSet& names,
                          int k,
                          const Formula& phi) {
    assert(phi.objective());
    switch (phi.type()) {
      case Formula::kNot: {
        switch (phi.as_not().arg().type()) {
          case Formula::kAtomic: {
            const Clause c = phi.as_not().arg().as_atomic().arg();
            return std::all_of(c.begin(), c.end(), [this, &s, &split_terms, &names, k](Literal a) {
              a = a.flip();
              Formula::Ref psi = Formula::Factory::Atomic(Clause{a});
              return ReduceConjunctions(s, split_terms, names, k, *psi);
            });
          }
          case Formula::kNot: {
            return ReduceConjunctions(s, split_terms, names, k, phi.as_not().arg().as_not().arg());
          }
          case Formula::kOr: {
            Formula::Ref left = Formula::Factory::Not(phi.as_not().arg().as_or().lhs().Clone());
            Formula::Ref right = Formula::Factory::Not(phi.as_not().arg().as_or().rhs().Clone());
            return ReduceConjunctions(s, split_terms, names, k, *left) &&
                   ReduceConjunctions(s, split_terms, names, k, *right);
          }
          case Formula::kExists: {
            const Term x = phi.as_not().arg().as_exists().x();
            const Formula& psi = phi.as_not().arg().as_exists().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &split_terms, &names, k, &psi, x](const Term n) {
              Formula::Ref xi = Formula::Factory::Not(psi.Clone());
              xi->SubstituteFree(Term::SingleSubstitution(x, n), tf_);
              return ReduceConjunctions(s, split_terms, names, k, *xi);
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

  bool Split(const Setup& s,
             const TermSet& split_terms,
             const SortedTermSet& names,
             int k,
             const Formula& phi) {
    assert(phi.objective());
    if (s.Subsumes(Clause{}) || phi.trivially_valid()) {
      return true;
    } else if (k > 0) {
      if (split_terms.empty()) {
        assert(phi.trivially_invalid());
        return phi.trivially_valid();
      }
      assert(!split_terms.empty());
      return std::any_of(split_terms.begin(), split_terms.end(), [this, &s, &split_terms, &names, k, &phi](Term t) {
        const TermSet& ns = names[t.sort()];
        assert(!ns.empty());
        return std::all_of(ns.begin(), ns.end(), [this, &s, &split_terms, &names, k, &phi, t](Term n) {
          Setup ss = s.Spawn();
          ss.AddClause(Clause{Literal::Eq(t, n)});
          return Split(ss, split_terms, names, k-1, phi);
        });
      });
    } else {
      return Reduce(s, names, phi);
    }
  }

  bool ReduceDisjunctions(const Setup& s,
                          const std::list<LiteralSet>& assign_lits,
                          const SortedTermSet& names,
                          int k,
                          const Formula& phi) {
    assert(phi.objective());
    switch (phi.type()) {
      case Formula::kAtomic: {
        return Assign(s, assign_lits, names, k, phi);
      }
      case Formula::kOr: {
        const Formula& left = phi.as_or().lhs();
        const Formula& right = phi.as_or().rhs();
        return ReduceDisjunctions(s, assign_lits, names, k, left) ||
               ReduceDisjunctions(s, assign_lits, names, k, right);
      }
      case Formula::kExists: {
        const Term x = phi.as_exists().x();
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &assign_lits, &names, k, &phi, x](const Term n) {
          Formula::Ref psi = phi.as_exists().arg().Clone();
          psi->SubstituteFree(Term::SingleSubstitution(x, n), tf_);
          return ReduceDisjunctions(s, assign_lits, names, k, *psi);
        });
      }
      case Formula::kNot: {
        switch (phi.as_not().arg().type()) {
          case Formula::kNot:
            return ReduceDisjunctions(s, assign_lits, names, k, phi.as_not().arg().as_not().arg());
          default:
            return !phi.trivially_invalid() && Assign(s, assign_lits, names, k, phi);
        }
      }
      case Formula::kKnow:
      case Formula::kCons:
      case Formula::kBel:
        assert(false);
        return false;
    }
  }

  bool Assign(const Setup& s,
              const std::list<LiteralSet>& assign_lits,
              const SortedTermSet& names,
              int k,
              const Formula& phi) {
    assert(phi.objective());
    if (s.Subsumes(Clause{}) || phi.trivially_invalid()) {
      return false;
    } else if (k > 0) {
      if (assign_lits.empty()) {
        assert(s.Consistent() && (phi.trivially_valid() || phi.trivially_invalid()));
        return phi.trivially_valid();
      }
      assert(!assign_lits.empty());
      return std::any_of(assign_lits.begin(), assign_lits.end(),
                         [this, &s, &assign_lits, &names, k, &phi](const LiteralSet& lits) {
        assert(!lits.empty());
        Setup ss = s.Spawn();
        for (Literal a : lits) {
          Clause c{a};
          if (!ss.Subsumes(c)) {
            ss.AddClause(c);
          }
        }
        return Assign(ss, assign_lits, names, k-1, phi);
      });
    } else {
      return s.Consistent() && Reduce(s, names, phi);
    }
  }

  bool Reduce(const Setup& s, const SortedTermSet& names, const Formula& phi) {
    assert(phi.objective());
    switch (phi.type()) {
      case Formula::kAtomic: {
        const Clause c = phi.as_atomic().arg();
        return c.valid() || (c.primitive() && s.Subsumes(c));
      }
      case Formula::kNot: {
        switch (phi.as_not().arg().type()) {
          case Formula::kAtomic: {
            const Clause c = phi.as_not().arg().as_atomic().arg();
            return std::all_of(c.begin(), c.end(), [this, &s, &names](Literal a) {
              Formula::Ref psi = Formula::Factory::Atomic(Clause{a.flip()});
              return Reduce(s, names, *psi);
            });
          }
          case Formula::kNot: {
            return Reduce(s, names, phi.as_not().arg().as_not().arg());
          }
          case Formula::kOr: {
            Formula::Ref left = Formula::Factory::Not(phi.as_not().arg().as_or().lhs().Clone());
            Formula::Ref right = Formula::Factory::Not(phi.as_not().arg().as_or().rhs().Clone());
            return Reduce(s, names, *left) &&
                   Reduce(s, names, *right);
          }
          case Formula::kExists: {
            const Term x = phi.as_not().arg().as_exists().x();
            const Formula& psi = phi.as_not().arg().as_exists().arg();
            const TermSet& ns = names[x.sort()];
            return std::all_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
              Formula::Ref xi = Formula::Factory::Not(psi.Clone());
              xi->SubstituteFree(Term::SingleSubstitution(x, n), tf_);
              return Reduce(s, names, *xi);
            });
          }
          case Formula::kKnow:
          case Formula::kCons:
          case Formula::kBel:
            assert(false);
            break;
        }
      }
      case Formula::kOr: {
        const Formula& left = phi.as_or().lhs();
        const Formula& right = phi.as_or().rhs();
        return Reduce(s, names, left) ||
               Reduce(s, names, right);
      }
      case Formula::kExists: {
        const Term x = phi.as_exists().x();
        const Formula& psi = phi.as_exists().arg();
        const TermSet& ns = names[x.sort()];
        return std::any_of(ns.begin(), ns.end(), [this, &s, &names, &psi, x](const Term n) {
          Formula::Ref xi = psi.Clone();
          xi->SubstituteFree(Term::SingleSubstitution(x, n), tf_);
          return Reduce(s, names, *xi);
        });
      }
      case Formula::kKnow:
      case Formula::kCons:
      case Formula::kBel:
        assert(false);
        return false;
    }
  }

  Term::Factory* tf_;
  Grounder grounder_;
};

}  // namespace lela

#endif  // LELA_SOLVER_H_

