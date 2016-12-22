// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Overloads some operators to provide a higher-level syntax for formulas.

#ifndef LELA_FORMAT_CPP_SYNTAX_H_
#define LELA_FORMAT_CPP_SYNTAX_H_

#include <lela/formula.h>
#include <lela/clause.h>
#include <lela/literal.h>
#include <lela/solver.h>
#include <lela/term.h>

#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)

namespace lela {
namespace format {
namespace cpp {

class HiTerm : public Term {
 public:
  HiTerm() = default;
  explicit HiTerm(Term t) : Term(t) {}
};

class HiSymbol : public Symbol {
 public:
  HiSymbol(Term::Factory* tf, Symbol s) : Symbol(s), tf_(tf) {}

  template<typename... Args>
  HiTerm operator()(Args... args) const { return HiTerm(tf_->CreateTerm(*this, Term::Vector({args...}))); }

 private:
  Term::Factory* const tf_;
};

class HiFormula;

class HiFormula {
 public:
  HiFormula(const Literal& a) : HiFormula(Clause({a})) {}
  HiFormula(const lela::Clause& c) : HiFormula(Formula::Factory::Atomic(c)) {}
  HiFormula(const Formula::Ref& phi) : HiFormula(*phi) {}
  HiFormula(const Formula& phi) : phi_(phi.Clone()) {}

  operator       Formula::Ref&()       { return phi_; }
  operator const Formula::Ref&() const { return phi_; }

  operator       Formula&()       { return phi(); }
  operator const Formula&() const { return phi(); }

        Formula& phi()       { return *phi_; }
  const Formula& phi() const { return *phi_; }

        Formula::Ref& operator->()       { return phi_; }
  const Formula::Ref& operator->() const { return phi_; }

  Formula::Ref operator*() { return std::move(phi_); }

  Clause as_clause() const { return AsClause(*phi_); }


 private:
  static Clause AsClause(const Formula& phi) {
    size_t nots = 0;
    const Formula* phi_ptr = &phi;
    for (;;) {
      switch (phi_ptr->type()) {
        case Formula::kAtomic: {
          if (nots % 2 != 0) {
            return Clause({});
          }
          const Clause c = phi_ptr->as_atomic().arg();
          if (!std::all_of(c.begin(), c.end(), [](Literal a) {
                           return a.quasiprimitive() || (!a.lhs().function() && !a.rhs().function()); })) {
            return Clause({});
          }
          return c;
        }
        case Formula::kNot: {
          ++nots;
          phi_ptr = &phi_ptr->as_not().arg();
          break;
        }
        case Formula::kExists: {
          if (nots % 2 == 0) {
            return Clause({});
          }
          phi_ptr = &phi_ptr->as_exists().arg();
          break;
        }
        case Formula::kOr: {
          const Clause c1 = HiFormula(phi_ptr->as_or().lhs()).as_clause();
          const Clause c2 = HiFormula(phi_ptr->as_or().rhs()).as_clause();
          const auto r = internal::join_ranges(c1.begin(), c1.end(), c2.begin(), c2.end());
          return Clause(r.begin(), r.end());
        }
        case Formula::kKnow:
        case Formula::kCons:
        case Formula::kBel:
          return Clause({});
      }
    }
  }

  Formula::Ref phi_;
};

class Context {
 public:
  Context() : solver_(&sf_, &tf_) {}

  Symbol::Sort CreateSort() {
    return sf()->CreateSort();
  }
  HiTerm CreateName(Symbol::Sort sort) {
    return HiTerm(tf()->CreateTerm(sf()->CreateName(sort)));
  }
  HiTerm CreateVariable(Symbol::Sort sort) {
    return HiTerm(tf()->CreateTerm(sf()->CreateVariable(sort)));
  }
  HiSymbol CreateFunction(Symbol::Sort sort, Symbol::Arity arity) {
    return HiSymbol(tf(), sf()->CreateFunction(sort, arity));
  }

  void AddClause(const Clause& c) {
    solver_.AddClause(c);
  }

  Solver* solver() { return &solver_; }
  const Solver& solver() const { return solver_; }

  Symbol::Factory* sf() { return &sf_; }
  Term::Factory* tf() { return &tf_; }

 private:
  Term::Factory tf_;
  Symbol::Factory sf_;
  Solver solver_;
};

inline HiFormula operator==(HiTerm t1, HiTerm t2) { return HiFormula(Literal::Eq(t1, t2)); }
inline HiFormula operator!=(HiTerm t1, HiTerm t2) { return HiFormula(Literal::Neq(t1, t2)); }

inline HiFormula operator~(const HiFormula& phi) { return Formula::Factory::Not(phi.phi().Clone()); }
inline HiFormula operator!(const HiFormula& phi) { return ~phi; }
inline HiFormula operator||(const HiFormula& phi, const HiFormula& psi) { return Formula::Factory::Or(phi.phi().Clone(), psi.phi().Clone()); }
inline HiFormula operator&&(const HiFormula& phi, const HiFormula& psi) { return ~(~phi || ~psi); }

inline HiFormula operator>>(const HiFormula& phi, const HiFormula& psi) { return (~phi || psi); }
inline HiFormula operator<<(const HiFormula& phi, const HiFormula& psi) { return (phi || ~psi); }

inline HiFormula operator==(const HiFormula& phi, const HiFormula& psi) { return (phi >> psi) && (phi << psi); }

inline HiFormula Ex(HiTerm x, const HiFormula& phi) { return Formula::Factory::Exists(x, phi.phi().Clone()); }
inline HiFormula Fa(HiTerm x, const HiFormula& phi) { return ~Ex(x, ~phi); }

}  // namespace cpp
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_CPP_SYNTAX_H_

