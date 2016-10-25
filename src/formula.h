// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Basic first-order formulas without any syntactic sugar. The atomic entities
// here are clauses, and the connectives are negation, disjunction, and
// existential quantifier. Formulas can be accessed through Readers, which in
// gives access to Element objects, which is either a Clause or a logical
// operator, which in case of an existential operator is parameterized with a
// (variable) Term. Formulas, Readers, and Elements are immutable.
//
// Readers are glorified range objects; their behaviour is only defined while
// the owning Formula is alive.
//
// Internally it's stored in Polish notation as a list of Element objects.
// Element would be a union if C++ would allow non-trivial types in unions.

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

    bool operator==(const Element& e) const {
      return type_ == e.type_ &&
            (type_ != kClause || clause_ == e.clause_) &&
            (type_ != kExists || var_ == e.var_);
    }
    bool operator!=(const Element& e) const { return !(*this == e); }

    Type type() const { return type_; }
    const lela::Clause clause() const { assert(type() == kClause); return clause_; }
    const Term var() const { assert(type() == kExists); return var_; }

   private:
    explicit Element(Type type) : type_(type) {}
    Element(Type type, Term var) : type_(type), var_(var) {}
    Element(Type type, const lela::Clause& c) : type_(type), clause_(c) {}

    Type type_;
    lela::Clause clause_;
    Term var_;
  };

  struct Reader {
    typedef std::list<Element>::const_iterator const_iterator;

    explicit Reader(const Formula& phi) : Reader(phi.es_.begin()) {}

    const_iterator begin() const { return begin_; }

    const_iterator end() const {
      if (!end_) {
        const_iterator it = begin_;
        for (int n = 1; n > 0; --n, ++it) {
          switch (it->type()) {
            case kClause: n += 0; break;
            case kNot:    n += 1; break;
            case kOr:     n += 2; break;
            case kExists: n += 1; break;
          }
        }
        end_ = Just(it);
      }
      return end_.val;
    }

    const Element& head() const { return *begin(); }
    Reader arg() const { assert(head().type() == kNot || head().type() == kExists); return Reader(std::next(begin())); }
    Reader left() const { assert(head().type() == kOr); return Reader(std::next(begin())); }
    Reader right() const { assert(head().type() == kOr); return Reader(left().end()); }

    Formula Build() const { return Formula(*this); }

   private:
    explicit Reader(const_iterator begin) : begin_(begin) {}

    const_iterator begin_;
    mutable Maybe<const_iterator> end_ = Nothing;
  };

  static Formula Clause(const Clause& c) { return Atomic(Element::Clause(c)); }
  static Formula Not(const Formula& phi) { return Unary(Element::Not(), phi); }
  static Formula Or(const Formula& phi, const Formula& psi) { return Binary(Element::Or(), phi, psi); }
  static Formula Exists(Term var, const Formula& phi) { return Unary(Element::Exists(var), phi); }

  explicit Formula(const Reader& r) : es_(r.begin(), r.end()) {}
  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  ~Formula() = default;

  bool operator==(const Formula& phi) const { return es_ == phi.es_; }
  bool operator!=(const Formula& phi) const { return !(*this == phi); }

  Reader reader() const { return Reader(*this); }

 private:
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

  static Formula Binary(Element op, Formula s, Formula r) {
    assert(op.type() == kOr);
    s.es_.splice(s.es_.end(), r.es_);
    s.es_.push_front(op);
    return s;
  }

  Formula() = default;

  std::list<Element> es_;
};

}  // namespace lela

#endif  // SRC_FORMULA_H_

