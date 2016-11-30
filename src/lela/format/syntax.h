// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Overloads some operators to provide a higher-level syntax for formulas.

#ifndef LELA_FORMAT_SYNTAX_H_
#define LELA_FORMAT_SYNTAX_H_

#include <lela/formula.h>
#include <lela/clause.h>
#include <lela/literal.h>
#include <lela/term.h>

#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)

namespace lela {
namespace format {

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

class HiLiteral : public Literal {
 public:
  explicit HiLiteral(Literal t) : Literal(t) {}
};

class HiFormula : public Formula {
 public:
  HiFormula(HiLiteral a) : HiFormula(Clause({a})) {}  // NOLINT
  HiFormula(const lela::Clause& c) : HiFormula(Formula::Clause(c)) {}  // NOLINT
  HiFormula(const Formula& phi) : Formula(phi) {}  // NOLINT
};

class Context {
 public:
  Context(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}

  Symbol::Sort NewSort() { return sf()->CreateSort(); }
  HiTerm NewName(Symbol::Sort sort) { return HiTerm(tf()->CreateTerm(sf()->CreateName(sort))); }
  HiTerm NewVar(Symbol::Sort sort) { return HiTerm(tf()->CreateTerm(sf()->CreateVariable(sort))); }
  HiSymbol NewFun(Symbol::Sort sort, Symbol::Arity arity) { return HiSymbol(tf(), sf()->CreateFunction(sort, arity)); }

 private:
  Symbol::Factory* sf() { return sf_; }
  Term::Factory* tf() { return tf_; }

  Symbol::Factory* const sf_;
  Term::Factory* const tf_;
};

inline HiLiteral operator==(HiTerm t1, HiTerm t2) { return HiLiteral(Literal::Eq(t1, t2)); }
inline HiLiteral operator!=(HiTerm t1, HiTerm t2) { return HiLiteral(Literal::Neq(t1, t2)); }

inline HiFormula operator~(const HiFormula& phi) { return Formula::Not(phi); }
inline HiFormula operator!(const HiFormula& phi) { return ~phi; }
inline HiFormula operator||(const HiFormula& phi, const HiFormula& psi) { return Formula::Or(phi, psi); }
inline HiFormula operator&&(const HiFormula& phi, const HiFormula& psi) { return ~(~phi || ~psi); }

inline HiFormula operator>>(const HiFormula& phi, const HiFormula& psi) { return (~phi || psi); }
inline HiFormula operator<<(const HiFormula& phi, const HiFormula& psi) { return (phi || ~psi); }

inline HiFormula operator==(const HiFormula& phi, const HiFormula& psi) { return (phi >> psi) && (phi << psi); }

inline HiFormula Ex(HiTerm x, const HiFormula& phi) { return Formula::Exists(x, phi); }
inline HiFormula Fa(HiTerm x, const HiFormula& phi) { return ~Ex(x, ~phi); }

}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_SYNTAX_H_

