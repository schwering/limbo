// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Solver implements objective limited belief implications. The key methods
// are Entails(), Determines(), and Consistent(), which determine whether the
// knowledge base consisting of the clauses added with AddClause() entails a
// query, determines a terms's denotation or is consistent with it,
// respectively. They are are sound but incomplete: if they return true, this
// answer is correct with respect to classical logic; if they return false,
// this may not be correct and should be rather interpreted as "don't know."
// The method EntailsComplete() uses Consistent() to implement a complete but
// unsound entailment relation. It is safe to call AddClause() between
// evaluating queries with Entails(), Determines(), EntailsComplete(), or
// Consistent().
//
// Splitting and assigning is done at a deterministic point, namely after
// reducing the outermost logical operators with conjunctive meaning (negated
// disjunction, double negation, negated existential).
//
// In the special case that the set of clauses can be shown to be inconsistent
// after the splits, Determines() returns the null term to indicate that [t=n]
// is entailed by the clauses for arbitrary n.

#ifndef LIMBO_SOLVER_H_
#define LIMBO_SOLVER_H_

#include <cassert>

#include <iterator>
#include <list>

#include <limbo/formula.h>
#include <limbo/grounder.h>
#include <limbo/literal.h>
#include <limbo/setup.h>
#include <limbo/term.h>

#include <limbo/internal/ints.h>
#include <limbo/internal/maybe.h>

namespace limbo {

class Solver {
 public:
  static constexpr bool kConsistencyGuarantee = true;
  static constexpr bool kNoConsistencyGuarantee = false;

  Solver(Symbol::Factory* sf, Term::Factory* tf) : tf_(tf), grounder_(sf, tf) {}
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;
  Solver(Solver&&) = default;
  Solver& operator=(Solver&&) = default;

  Setup::Result AddClause(const Clause& c) { return grounder_.AddClause(c); }

  Grounder& grounder() { return grounder_; }
  const Grounder& grounder() const { return grounder_; }

  const Setup& setup() const { return grounder_.setup(); }

  bool Entails(Formula::belief_level k, const Formula& phi, bool assume_consistent = false) {
    assert(phi.objective());
    assert(phi.free_vars().all_empty());
    Grounder::Undo undo1;
    if (assume_consistent) {
      grounder_.GuaranteeConsistency(phi, &undo1);
    }
    Grounder::Undo undo2;
    grounder_.PrepareForQuery(phi, &undo2);
    const bool entailed = setup().Subsumes(Clause{}) || phi.trivially_valid() ||
        Split(k, [this, &phi]() { return Reduce(phi); }, [](bool r1, bool r2) { return r1 && r2; }, true, false);
    return entailed;
  }

  internal::Maybe<Term> Determines(Formula::belief_level k, Term lhs, bool assume_consistent = false) {
    assert(lhs.primitive());
    Grounder::Undo undo1;
    if (assume_consistent) {
      grounder_.GuaranteeConsistency(lhs, &undo1);
    }
    Grounder::Undo undo2;
    grounder_.PrepareForQuery(lhs, &undo2);
    internal::Maybe<Term> inconsistent_result = internal::Just(Term());
    internal::Maybe<Term> unsuccessful_result = internal::Nothing;
    internal::Maybe<Term> t = Split(k,
                 [this, lhs]() { return setup().Determines(lhs); },
                 [](internal::Maybe<Term> r1, internal::Maybe<Term> r2) {
                   return r1 && r2 && r1.val == r2.val ? r1 :
                          r1 && r2 && r1.val.null()    ? r2 :
                          r1 && r2 && r2.val.null()    ? r1 :
                                                         internal::Nothing;
                 },
                 inconsistent_result, unsuccessful_result);
    return t;
  }

  bool EntailsComplete(int k, const Formula& phi, bool assume_consistent = false) {
    assert(phi.objective());
    assert(phi.free_vars().all_empty());
    Formula::Ref psi = Formula::Factory::Not(phi.Clone());
    return !Consistent(k, *psi, assume_consistent);
  }

  bool Consistent(int k, const Formula& phi, bool assume_consistent = false) {
    assert(phi.objective());
    assert(phi.free_vars().all_empty());
    Grounder::Undo undo1;
    if (assume_consistent) {
      grounder_.GuaranteeConsistency(phi, &undo1);
    }
    Grounder::Undo undo2;
    grounder_.PrepareForQuery(phi, &undo2);
    return !phi.trivially_invalid() && Fix(k, [this, &phi]() { return Reduce(phi); });
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(SolverTest, Constants);
#endif

  typedef internal::size_t size_t;
  typedef Formula::SortedTermSet SortedTermSet;

  bool Reduce(const Formula& phi) {
    assert(phi.objective());
    switch (phi.type()) {
      case Formula::kAtomic: {
        const Clause c = phi.as_atomic().arg();
        assert(c.ground());
        assert(c.valid() || c.primitive());
        return c.valid() || setup().Subsumes(c);
      }
      case Formula::kNot: {
        switch (phi.as_not().arg().type()) {
          case Formula::kAtomic: {
            const Clause c = phi.as_not().arg().as_atomic().arg();
            return std::all_of(c.begin(), c.end(), [this](Literal a) {
              Formula::Ref psi = Formula::Factory::Atomic(Clause{a.flip()});
              return Reduce(*psi);
            });
          }
          case Formula::kNot: {
            return Reduce(phi.as_not().arg().as_not().arg());
          }
          case Formula::kOr: {
            Formula::Ref left = Formula::Factory::Not(phi.as_not().arg().as_or().lhs().Clone());
            Formula::Ref right = Formula::Factory::Not(phi.as_not().arg().as_or().rhs().Clone());
            return Reduce(*left) && Reduce(*right);
          }
          case Formula::kExists: {
            const Term x = phi.as_not().arg().as_exists().x();
            const Formula& psi = phi.as_not().arg().as_exists().arg();
            const SortedTermSet& xs = psi.free_vars();
            // XXX TODO Check that this works even if we disable the first if-branch, once the name computation is fixed.
            // In test-functions.limbo, the line
            //   Refute: Fa x Know<1> f(x) == x
            // yields an error because the set of names is empty. This error should be fixed by correct name computation.
            if (xs.all_empty()) {
              Formula::Ref xi = Formula::Factory::Not(psi.Clone());
              return Reduce(*xi);
            } else {
              const Grounder::Names ns = grounder_.names(x.sort());
              assert(ns.begin() != ns.end());
              return std::all_of(ns.begin(), ns.end(), [this, &psi, x](const Term n) {
                Formula::Ref xi = Formula::Factory::Not(psi.Clone());
                xi->SubstituteFree(Term::Substitution(x, n), tf_);
                return Reduce(*xi);
              });
            }
          }
          case Formula::kKnow:
          case Formula::kCons:
          case Formula::kBel:
          case Formula::kGuarantee:
            assert(false);
            break;
        }
      }
      case Formula::kOr: {
        const Formula& left = phi.as_or().lhs();
        const Formula& right = phi.as_or().rhs();
        return Reduce(left) || Reduce(right);
      }
      case Formula::kExists: {
        const Term x = phi.as_exists().x();
        const Formula& psi = phi.as_exists().arg();
        const SortedTermSet& xs = psi.free_vars();
        if (xs.all_empty()) {
          return Reduce(psi);
        } else {
          const Grounder::Names ns = grounder_.names(x.sort());
          assert(ns.begin() != ns.end());
          return std::any_of(ns.begin(), ns.end(), [this, &psi, x](const Term n) {
            Formula::Ref xi = psi.Clone();
            xi->SubstituteFree(Term::Substitution(x, n), tf_);
            return Reduce(*xi);
          });
        }
      }
      case Formula::kKnow:
      case Formula::kCons:
      case Formula::kBel:
      case Formula::kGuarantee:
        assert(false);
        return false;
    }
    throw;
  }

  template<typename T, typename GoalPredicate, typename MergeResultPredicate>
  T Split(int k, GoalPredicate goal, MergeResultPredicate merge, T inconsistent_result, T unsuccessful_result) {
    // XXX TODO The statement "the split order [does not matter] for Entails()"
    // is wrong:
    //
    // For Determines(), the split order matters, for Entails() it does not.
    // Suppose we have two split terms t1, t2, t3 and two names n1, n2, and
    // a query term t and two candidate names n, n' for t.
    //
    // Let us assume that for all combinations of t1=N*, t3=N**, the reasoner
    // finds that t=n.
    //
    // The reasoner splits t1 at the first level, and after setting t1=n1 it
    // descends to the next split level, where it successfully splits t2, which
    // obtains t=n' as binding for t. Back at split level one, the reasoner now
    // considers the case t1=n2, again descends to the next level, where it
    // splits, say, t3, and obtains t=n.
    //
    // Back at split level one, the reasoner sees that t=n' and t=n are
    // incompatible and hence proceeds by splitting t2, which does not succeed.
    // In a way, t=n' found with t2 after t1=n1 blocks the real solution.
    //
    // Again at level one, the reasoner splits t3. If the order does not
    // matter, it will not descend to any further split level, since all
    // unordered combinations of t1, t2, t3 have been tested already.
    //
    // If the order does matter, however, it does descend to the next level.
    // There it will split t1: even if it picks t2 before t1, t2 will prove
    // incompatible with t3 (*). And once it splits t1 at level two, it will
    // find t=n, which this time is not blocked by t=n'.
    //
    // (*) This holds under the assumption that the KB is consistent.
    // If the KB is not consistent, could split terms 'block' each other?
    if (setup().contains_empty_clause()) {
      return unsuccessful_result;
    }
    if (k == 0) {
      return goal();
    }
    bool recursed = false;
    for (const Term t : grounder_.lhs_terms()) {
      if (setup().Determines(t)) {
        continue;
      }
      auto merged_result = unsuccessful_result;
      for (const Term n : grounder_.rhs_names(t)) {
        Grounder::Undo undo;
        const Setup::Result add_result = grounder_.AddClause(Clause{Literal::Eq(t, n)}, &undo);
        if (add_result == Setup::kInconsistent) {
          merged_result = !merged_result ? inconsistent_result : merge(merged_result, inconsistent_result);
          if (!merged_result) {
            goto next_term;
          }
          recursed = true;
          goto next_name;
        }
        {
          const T split_result = Split(k-1, goal, merge, inconsistent_result, unsuccessful_result);
          if (!split_result) {
            goto next_term;
          }
          merged_result = !merged_result ? split_result : merge(merged_result, split_result);
        }
        if (!merged_result) {
          goto next_term;
        }
        recursed = true;
next_name:
        {};
      }
      return merged_result;
next_term:
      {}
    }
    return recursed ? unsuccessful_result : goal();
  }

  template<typename GoalPredicate>
  bool Fix(int k, GoalPredicate goal) {
    if (setup().Subsumes(Clause{})) {
      return false;
    }
    if (k > 0) {
      std::unordered_set<Literal> as;
      for (const Term t : grounder_.lhs_terms()) {
        for (const Term n : grounder_.rhs_names(t)) {
          {
            const Literal a = Literal::Eq(t, n);
            Grounder::Undo undo;
            const Setup::Result add_result = grounder_.AddClause(Clause{a}, &undo, true);
            const bool succ = add_result != Setup::kSubsumed && Fix(k-1, goal);
            if (succ) {
              return true;
            }
          }
          {
            const Literal a = grounder_.Variablify(Literal::Eq(t, n));
            if (!as.insert(a).second) {
              Grounder::Undo undo;
              const Setup::Result add_result = grounder_.AddClause(Clause{a}, &undo, true);
              const bool succ = add_result != Setup::kSubsumed && Fix(k-1, goal);
              if (succ) {
                return true;
              }
            }
          }
        }
      }
    }
    return setup().Consistent() && goal();
  }

  Term::Factory* tf_;
  Grounder grounder_;
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

