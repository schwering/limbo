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

#ifndef SRC_FORMULA_H_
#define SRC_FORMULA_H_

#include <memory>
#include <vector>
#include "./setup.h"
#include "./term.h"

namespace esbl {

class Formula {
 public:
  typedef std::unique_ptr<Formula> Ptr;
  typedef Setup::split_level split_level;

  Formula() = default;
  Formula(const Formula&) = delete;
  Formula& operator=(const Formula&) = delete;
  virtual ~Formula() {}

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

  virtual Ptr Copy() const = 0;
  virtual Ptr Substitute(const Unifier& theta) const = 0;

  bool EntailedBy(Term::Factory* tf, Setup* setup, split_level k) const;
  bool EntailedBy(Term::Factory* tf, Setups* setups, split_level k) const;

 private:
  friend std::ostream& operator<<(std::ostream& os, const Formula& phi);

  struct Equal;
  struct Lit;
  struct Junction;
  struct Quantifier;
  struct Knowledge;
  struct Belief;
  struct Cnf;

  virtual void Negate() = 0;
  virtual void PrependActions(const TermSeq& z) = 0;
  virtual void CollectFreeVariables(Variable::SortedSet* vs) const = 0;
  virtual Cnf MakeCnf(StdName::SortedSet* hplus) const = 0;
  virtual void Print(std::ostream* os) const = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Formula& phi) {
  phi.Print(&os);
  return os;
}

}  // namespace esbl

#endif  // SRC_FORMULA_H_

