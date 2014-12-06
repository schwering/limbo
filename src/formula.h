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
  class Obj;
  struct Ptr;
  struct ObjPtr;

  typedef Setup::split_level split_level;

  Formula() = default;
  Formula(const Formula&) = delete;
  Formula& operator=(const Formula&) = delete;
  virtual ~Formula() {}

  static ObjPtr True();
  static ObjPtr False();
  static ObjPtr Eq(const Term& t1, const Term& t2);
  static ObjPtr Neq(const Term& t1, const Term& t2);
  static ObjPtr Lit(const Literal& l);
  static ObjPtr Or(ObjPtr phi1, ObjPtr phi2);
  static Ptr Or(Ptr phi1, Ptr phi2);
  static ObjPtr And(ObjPtr phi1, ObjPtr phi2);
  static Ptr And(Ptr phi1, Ptr phi2);
  static ObjPtr OnlyIf(ObjPtr phi1, ObjPtr phi2);
  static Ptr OnlyIf(Ptr phi1, Ptr phi2);
  static ObjPtr If(ObjPtr phi1, ObjPtr phi2);
  static Ptr If(Ptr phi1, Ptr phi2);
  static ObjPtr Iff(ObjPtr phi1, ObjPtr phi2);
  static Ptr Iff(Ptr phi1, Ptr phi2);
  static ObjPtr Neg(ObjPtr phi);
  static Ptr Neg(Ptr phi);
  static ObjPtr Act(const Term& t, ObjPtr phi);
  static Ptr Act(const Term& t, Ptr phi);
  static ObjPtr Act(const TermSeq& z, ObjPtr phi);
  static Ptr Act(const TermSeq& z, Ptr phi);
  static ObjPtr Exists(const Variable& x, ObjPtr phi);
  static Ptr Exists(const Variable& x, Ptr phi);
  static ObjPtr Forall(const Variable& x, ObjPtr phi);
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

  bool Eval(const Setup& setup) const;
  bool Eval(const Setups& setups) const;

 private:
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
  virtual ObjPtr Reduce(const Setup& setup,
                        const StdName::SortedSet& kb_and_query_ns) const = 0;
  virtual ObjPtr Reduce(const Setups& setups,
                        const StdName::SortedSet& kb_and_query_ns) const = 0;
  virtual void Print(std::ostream* os) const = 0;
};

class Formula::Obj : public Formula {
 public:
  virtual Ptr Copy() const override;
  virtual ObjPtr ObjCopy() const = 0;

  virtual Ptr Regress(Term::Factory* tf,
                      const DynamicAxioms& axioms) const override;
  virtual ObjPtr ObjRegress(Term::Factory* tf,
                            const DynamicAxioms& axioms) const = 0;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

 private:
  friend class Formula;

  struct Equal;
  struct Lit;
  struct Junction;
  struct Quantifier;

  virtual std::pair<Truth, ObjPtr> Simplify() const = 0;
  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns) const;
  virtual Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
                      StdName::SortedSet* placeholders) const = 0;
};

struct Formula::Ptr {
  typedef std::unique_ptr<Formula> unique_ptr;

  Ptr() : ptr_() {}
  Ptr(Formula* ptr) : ptr_(ptr) {}
  Ptr(unique_ptr&& ptr) : ptr_(std::forward<unique_ptr>(ptr)) {}  // NOLINT
  Ptr(Ptr&& ptr) : ptr_(std::forward<unique_ptr>(ptr.ptr_)) {}  // NOLINT
  virtual ~Ptr() {}

  Ptr& operator=(Ptr&& ptr) { ptr_ = std::forward<unique_ptr>(ptr.ptr_); return *this; }
  Ptr& operator=(std::nullptr_t np) { ptr_ = np; return *this; }

  virtual Formula* get() const { return ptr_.get(); }
  Formula* operator->() const { return ptr_.operator->(); }
  Formula& operator*() const { return ptr_.operator*(); }
  operator bool() const { return ptr_.operator bool(); }

 protected:
  unique_ptr ptr_;
};

struct Formula::ObjPtr : public Formula::Ptr {
  typedef std::unique_ptr<Formula::Obj> unique_ptr;

  ObjPtr() : Ptr() {}
  ObjPtr(Formula::Obj* ptr) : Ptr(ptr) {}
  ObjPtr(unique_ptr&& ptr) : Ptr(std::move(ptr)) {}  // NOLINT
  ObjPtr(ObjPtr&& ptr) : Ptr(std::move(ptr.ptr_)) {}  // NOLINT
  virtual ~ObjPtr() {}

  ObjPtr& operator=(ObjPtr&& ptr) { ptr_ = std::forward<Ptr::unique_ptr>(ptr.ptr_); return *this; }
  ObjPtr& operator=(std::nullptr_t np) { ptr_ = np; return *this; }

  virtual Formula::Obj* get() const { return static_cast<Formula::Obj*>(ptr_.get()); }
  Formula::Obj* operator->() const { return static_cast<Formula::Obj*>(ptr_.operator->()); }
  Formula::Obj& operator*() const { return static_cast<Formula::Obj&>(ptr_.operator*()); }
  operator bool() const { return ptr_.operator bool(); }
};

class DynamicAxioms {
 public:
  virtual Maybe<Formula::ObjPtr> RegressOneStep(Term::Factory* tf,
                                                const Atom& a) const = 0;
};

std::ostream& operator<<(std::ostream& os, const Formula& phi);

}  // namespace esbl

#endif  // SRC_FORMULA_H_

