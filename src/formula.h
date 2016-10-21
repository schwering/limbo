// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Basic first-order formulas without any syntactic sugar. The atomic entities
// here are clauses, and the connectives are negation, disjunction, and
// existential quantifier.
//
// Internally it's stored in Polish notation as a list of Element objects.
// Element is a public class here so that the user can access the clauses and
// variables. Element would be a union if C++ unions could consist of
// non-trivial types. It's the most pragmatic representation of formulas I
// found.

#ifndef SRC_FORMULA_H_
#define SRC_FORMULA_H_

#include <cassert>
#include <list>
#include "./clause.h"
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

class Formula {
 public:
  enum Type { kClause, kNot, kOr, kExists };

  struct Element {
    static Element Clause(const lela::Clause& c) { return Element(kClause, c); }
    static Element Not() { return Element(kNot); }
    static Element Or() { return Element(kOr); }
    static Element Exists(Term var) { assert(var.variable()); return Element(kExists, var); }

    explicit Element(Type type) : type_(type) {}
    Element(Type type, Term var) : type_(type), var_(var) {}
    Element(Type type, const lela::Clause& c) : type_(type), clause_(c) {}

    Type type() const { return type_; }
    const lela::Clause clause() const { assert(type() == kClause); return clause_; }
    const Term var() const { assert(type() == kExists); return var_; }

   private:
    Type type_;
    lela::Clause clause_;
    Term var_;
  };

  static Formula Clause(const Clause& c) { return Atomic(Element::Clause(c)); }
  static Formula Not(const Formula& phi) { return Unary(Element::Not(), phi); }
  static Formula Or(const Formula& phi, const Formula& psi) { return Binary(Element::Or(), phi, psi); }
  static Formula Exists(Term var, const Formula& phi) { return Unary(Element::Exists(var), phi); }

  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  ~Formula() = default;

  Element head() const { return es_.front(); }
  Formula arg() const { return arg_sub().Pull(); }
  Formula left() const { return left_sub().Pull(); }
  Formula right() const { return right_sub().Pull(); }

 private:
  struct Subformula {
    typedef std::list<Element>::const_iterator const_iterator;

    Subformula(const_iterator begin) : begin_(begin), end_(begin) {
      for (int n = 1; n > 0; --n, ++end_) {
        switch (end_->type()) {
          case kClause: n += 0; break;
          case kNot:    n += 1; break;
          case kOr:     n += 3; break;
          case kExists: n += 1; break;
        }
      }
    }

    const_iterator begin() const { return begin_; }
    const_iterator end() const { return end_; }

    Formula Pull() const {
      Formula s;
      std::copy(begin(), end(), std::front_inserter(s.es_));
      return s;
    }

   private:
    const_iterator begin_;
    const_iterator end_;
  };

  static Formula Atomic(Element op) {
    assert(op.type() == kClause);
    Formula r;
    r.es_.push_front(op);
    return r;
  }

  static Formula Unary(Element op, Formula s) {
    assert(op.type() == kNot || op.type() == kExists);
    s.es_.push_front(op);
    return s;
  }

  static Formula Binary(Element op, const Formula& s, Formula r) {
    assert(op.type() == kOr);
    std::move(s.es_.begin(), s.es_.end(), std::front_inserter(r.es_));
    r.es_.push_front(op);
    return r;
  }

  Formula() = default;

  Subformula arg_sub() const {
    assert(head().type() == kNot || head().type() == kExists);
    return Subformula(std::next(es_.begin()));
  }

  Subformula left_sub() const {
    assert(head().type() == kOr);
    return Subformula(std::next(es_.begin()));
  }

  Subformula right_sub() const {
    assert(head().type() == kOr);
    return Subformula(std::next(left_sub().end()));
  }

  std::list<Element> es_;
};

}  // namespace lela

#endif  // SRC_FORMULA_H_

