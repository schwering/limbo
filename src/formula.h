// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// Formulas are represented in ENNF (extended negation normal form), which means
// that negation and actions are pushed inwards. For this transformation to be
// equivalent,
//
// Formulas shall be rectified, that is, no variable should be bound by two
// quantifiers. Otherwise an action variable might be moved within the scope of
// another quantifier, which would change the formula's meaning.
//
// The conversion to CNF is necessary because, to make the decision procedure
// deterministic, literals are only split in setup_entails(), that is, only
// clause-wise. However, splitting literals across conjunctions or quantifiers
// is sometimes necessary:
// Example 1: Consider the tautology (p v q v (~p ^ ~q)). It can be shown in
// ESL * for k = 2 by splitting p and q and then decomposing the query (if both
// split literals are negative, [~p] and [~q] hold and otherwise [p,q] holds).
// The implementation computes the CNF ((p v q v ~p) ^ (p v q v ~q)) and then
// both clauses can be shown even for k = 1.
// Example 2: Given a KB (P(#1) v P(#2)), in ESL (E x . P(x)) follows because
// we can first split P(#1) and then P(#2) and then proceed with the semantics
// of the existential, which requires to show P(#1) or to show P(#2). The
// implementation obtains (P(#1) v P(#2)) after grounding the query and thus
// proves the query even for k = 0.
//
// On the scope of ns in Reduce() and MakeCnf(): the sorted set ns should
// contain the non-placeholder names from the setup and all names from the
// query. Currently, it is not the full query but the sub-formula within K, B or
// a quantifier, respectively. This leads to more incompleteness, as for example
// the material implication (P(#1) -> E x. P(x)) expands to (P(#1) -> P(#-123)),
// because #1 does not occur in the scope of the quantifier. At least in the
// case of quantifers, the formula can be converted to prenex form
// E x. (P(#1) -> P(x)), which expands to (P(#1) -> P(#1) v P(#-123)). So in a
// way, the current implementation allows th user to control the grounding and
// thus a bit complexity.

#ifndef SRC_FORMULA_H_
#define SRC_FORMULA_H_

#include <memory>
#include <utility>
#include <vector>
#include "./setup.h"
#include "./term.h"

namespace esbl {

class DynamicAxioms;

class Formula {
 public:
  typedef std::unique_ptr<Formula> Ptr;
  typedef Setup::split_level split_level;

  Formula() = default;
  Formula(const Formula&) = delete;
  Formula& operator=(const Formula&) = delete;
  virtual ~Formula() {}

  static Ptr True();
  static Ptr False();
  static Ptr Eq(const Term& t1, const Term& t2);
  static Ptr Neq(const Term& t1, const Term& t2);
  static Ptr Lit(const Literal& l);
  static Ptr Or(Ptr phi1, Ptr phi2);
  static Ptr And(Ptr phi1, Ptr phi2);
  static Ptr OnlyIf(Ptr phi1, Ptr phi2);
  static Ptr If(Ptr phi1, Ptr phi2);
  static Ptr Iff(Ptr phi1, Ptr phi2);
  static Ptr Neg(Ptr phi);
  static Ptr Act(const Term& t, Ptr phi);
  static Ptr Act(const TermSeq& z, Ptr phi);
  static Ptr Exists(const Variable& x, Ptr phi);
  static Ptr Forall(const Variable& x, Ptr phi);
  static Ptr Know(split_level k, Ptr phi);
  static Ptr Believe(split_level k, Ptr psi);
  static Ptr Believe(split_level k, Ptr phi, Ptr psi);

  virtual Ptr Copy() const = 0;
  virtual void PrependActions(const TermSeq& z) = 0;
  virtual void SubstituteInPlace(const Unifier& theta) = 0;
  virtual void GroundInPlace(const Assignment& theta) = 0;

  // tf is needed to create variables in order to keep the formula rectified.
  virtual Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const = 0;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

  bool Eval(Setup* setup) const;
  bool Eval(Setups* setups) const;

 private:
  friend std::ostream& operator<<(std::ostream& os, const Formula& phi);

  struct Equal;
  struct Lit;
  struct Junction;
  struct Quantifier;
  struct Knowledge;
  struct Belief;
  enum Truth { TRIVIALLY_TRUE, TRIVIALLY_FALSE, NONTRIVIAL };
  class Cnf;

  virtual void Negate() = 0;
  virtual void CollectFreeVariables(Variable::Set* vs) const = 0;
  virtual void CollectNames(StdName::SortedSet* ns) const = 0;
  virtual Ptr Reduce(Setup* setup,
                     const StdName::SortedSet& kb_and_query_ns) const = 0;
  virtual Ptr Reduce(Setups* setups,
                     const StdName::SortedSet& kb_and_query_ns) const = 0;
  virtual std::pair<Truth, Ptr> Simplify() const = 0;
  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns) const;
  virtual Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
                      StdName::SortedSet* placeholders) const = 0;
  virtual void Print(std::ostream* os) const = 0;
};

class DynamicAxioms {
 public:
  virtual Maybe<Formula::Ptr> RegressOneStep(Term::Factory* tf,
                                             const Atom& a) const = 0;
};

std::ostream& operator<<(std::ostream& os, const Formula& phi);

}  // namespace esbl

#endif  // SRC_FORMULA_H_

