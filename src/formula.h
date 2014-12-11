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

class Bat;

class Formula {
 public:
  typedef Setup::split_level split_level;
  class Obj;
  typedef std::unique_ptr<Formula> Ptr;
  typedef std::unique_ptr<Obj> ObjPtr;

  Formula() = default;
  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  virtual ~Formula() {}

  static ObjPtr True();
  static ObjPtr False();
  static ObjPtr Value(bool b);
  static ObjPtr Eq(const Term& t1, const Term& t2);
  static ObjPtr Neq(const Term& t1, const Term& t2);
  static ObjPtr Lit(const Literal& l);
  static ObjPtr Or(ObjPtr phi1, ObjPtr phi2);
  static    Ptr Or(   Ptr phi1,    Ptr phi2);
  static ObjPtr And(ObjPtr phi1, ObjPtr phi2);
  static    Ptr And(   Ptr phi1,    Ptr phi2);
  static ObjPtr OnlyIf(ObjPtr phi1, ObjPtr phi2);
  static    Ptr OnlyIf(   Ptr phi1,    Ptr phi2);
  static ObjPtr If(ObjPtr phi1, ObjPtr phi2);
  static    Ptr If(   Ptr phi1,    Ptr phi2);
  static ObjPtr Iff(ObjPtr phi1, ObjPtr phi2);
  static    Ptr Iff(   Ptr phi1,    Ptr phi2);
  static ObjPtr Neg(ObjPtr phi);
  static    Ptr Neg(   Ptr phi);
  static ObjPtr Act(const Term& t, ObjPtr phi);
  static    Ptr Act(const Term& t,    Ptr phi);
  static ObjPtr Act(const TermSeq& z, ObjPtr phi);
  static    Ptr Act(const TermSeq& z,    Ptr phi);
  static ObjPtr Exists(const Variable& x, ObjPtr phi);
  static    Ptr Exists(const Variable& x,    Ptr phi);
  static ObjPtr Forall(const Variable& x, ObjPtr phi);
  static    Ptr Forall(const Variable& x,    Ptr phi);
  static    Ptr Know(split_level k, Ptr phi);
  static    Ptr Believe(split_level k, Ptr psi);
  static    Ptr Believe(split_level k, Ptr phi, Ptr psi);

  virtual Ptr Copy() const = 0;
  virtual void PrependActions(const TermSeq& z) = 0;
  virtual void SubstituteInPlace(const Unifier& theta) = 0;
  virtual void GroundInPlace(const Assignment& theta) = 0;

 private:
  friend class Bat;
  friend std::ostream& operator<<(std::ostream& os, const Formula& phi);

  template<class BaseFormula> struct BaseJunction;
  template<class BaseFormula> struct BaseQuantifier;
  struct Junction;
  struct Quantifier;
  struct Knowledge;
  struct Belief;
  enum Truth { TRIVIALLY_TRUE, TRIVIALLY_FALSE, NONTRIVIAL };
  class Cnf;

  virtual void Negate() = 0;
  virtual void CollectFreeVariables(Variable::Set* vs) const = 0;
  virtual void CollectNames(StdName::SortedSet* ns) const = 0;

  virtual Ptr Regress(Bat* bat, bool is_obj_level) const = 0;

  virtual Ptr GroundObjectiveSensings(
      Bat* bat, const StdName::SortedSet& kb_and_query_ns) const = 0;

  virtual std::pair<Truth, Ptr> Simplify() const = 0;

  virtual ObjPtr Reduce(const Bat& bat,
                        const StdName::SortedSet& kb_and_query_ns) const = 0;

  virtual void Print(std::ostream* os) const = 0;
};

class Formula::Obj : public Formula {
 public:
  Ptr Copy() const override;
  virtual ObjPtr ObjCopy() const = 0;

  Ptr Regress(Bat* bat, bool is_obj_level) const override;
  virtual ObjPtr ObjRegress(Bat* bat, bool is_obj_level) const = 0;

  Ptr GroundObjectiveSensings(
      Bat* bat, const StdName::SortedSet& kb_and_query_ns) const override;
  virtual ObjPtr ObjGroundObjectiveSensings(
      Bat* bat, const StdName::SortedSet& kb_and_query_ns) const = 0;

  std::pair<Truth, Ptr> Simplify() const override;
  virtual std::pair<Truth, ObjPtr> ObjSimplify() const = 0;

 private:
  friend class Bat;
  friend class Formula;

  struct Equal;
  struct Lit;
  struct Eval;
  struct Junction;
  struct Quantifier;

  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns) const;
  virtual Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
                      StdName::SortedSet* placeholders) const = 0;
};

class Bat {
 public:
  typedef Setup::split_level split_level;
  typedef Setups::belief_level belief_level;

  Bat() = default;
  Bat(const Bat&) = delete;
  Bat& operator=(const Bat&) = delete;
  virtual ~Bat() = default;

  virtual Maybe<Formula::ObjPtr> RegressOneStep(const Atom& a) = 0;

  virtual void GuaranteeConsistency(split_level k) = 0;
  virtual void AddClause(const Clause& c) = 0;
  void Add(Formula::ObjPtr phi);

  bool Entails(Formula::Ptr phi);

  virtual bool InconsistentAt(belief_level p,
                              split_level k) const = 0;
  virtual bool EntailsClauseAt(belief_level p,
                               const SimpleClause& c,
                               split_level k) const = 0;

  virtual bool Inconsistent(split_level k) const {
    return InconsistentAt(n_levels() - 1, k);
  }
  virtual bool EntailsClause(const SimpleClause& c, split_level k) const {
    return EntailsClauseAt(n_levels() - 1, c, k);
  }

  virtual size_t n_levels() const = 0;
  virtual const StdName::SortedSet& names() const = 0;

  const Term::Factory& tf() const { return tf_; }
  Term::Factory* mutable_tf() { return &tf_; }

  void set_regression(bool r) { regression_ = r; }
  bool regression() const { return regression_; }

 private:
  friend Formula::Obj::Lit;

  bool Sensed(const Literal& l);
  bool Sensed(Formula::ObjPtr phi,
              const StdName::SortedSet& kb_and_query_ns);
  bool Entails(Formula::ObjPtr phi,
               const StdName::SortedSet& kb_and_query_ns) const;

  Term::Factory tf_;
  bool regression_ = false;
};

std::ostream& operator<<(std::ostream& os, const Formula& phi);

}  // namespace esbl

#endif  // SRC_FORMULA_H_

