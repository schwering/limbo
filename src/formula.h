// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Basic first-order formulas without any syntactic sugar. The atomic entities
// here are clauses, and the connectives are negation, disjunction, and
// existential quantifier. Formulas are immutable.
//
// Formulas can be accessed through Readers, which in gives access to Element
// objects, which is either a Clause or a logical operator, which in case of an
// existential operator is parameterized with a (variable) Term. Readers and
// Elements are immutable.
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
  class Element {
   public:
    enum Type { kClause, kNot, kOr, kExists };

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
    Maybe<lela::Clause> clause() const { return clause_; }
    Maybe<Term> var() const { return var_; }

   private:
    explicit Element(Type type) : type_(type) {}
    Element(Type type, Term var) : type_(type), var_(Just(var)) {}
    Element(Type type, const lela::Clause& c) : type_(type), clause_(Just(c)) {}

    Type type_;
    Maybe<lela::Clause> clause_ = Nothing;
    Maybe<Term> var_ = Nothing;
  };

  template<typename Iter = std::list<Element>::const_iterator>
  class Reader {
   public:
    Iter begin() const { return begin_; }

    Iter end() const {
      if (!end_) {
        Iter it = begin_;
        for (int n = 1; n > 0; --n, ++it) {
          switch (it->type()) {
            case Element::kClause: n += 0; break;
            case Element::kNot:    n += 1; break;
            case Element::kOr:     n += 2; break;
            case Element::kExists: n += 1; break;
          }
        }
        end_ = Just(it);
      }
      return end_.val;
    }

    const Element& head() const { return *begin(); }

    Reader arg() const {
      assert(head().type() == Element::kNot || head().type() == Element::kExists);
      return Reader(std::next(begin()));
    }

    Reader left() const {
      assert(head().type() == Element::kOr);
      return Reader(std::next(begin()));
    }

    Reader right() const {
      assert(head().type() == Element::kOr);
      return Reader(left().end());
    }

    Formula Build() const { return Formula(*this); }

    template<typename UnaryFunction>
    struct SubstituteElement {
      SubstituteElement(UnaryFunction theta, Term::Factory* tf) : theta_(theta), tf_(tf) {}

      Element operator()(const Element& e) const {
        switch (e.type()) {
          case Element::kClause: return Element::Clause(e.clause().val.Substitute(theta_, tf_)); break;
          case Element::kNot:    return Element::Not(); break;
          case Element::kOr:     return Element::Or(); break;
          case Element::kExists: return Element::Exists(e.var().val.Substitute(theta_, tf_)); break;
        }
      }

     private:
      UnaryFunction theta_;
      Term::Factory* tf_;
    };

    template<typename UnaryFunction>
    Reader<transform_iterator<SubstituteElement<UnaryFunction>, Iter>> Substitute(UnaryFunction theta) {
      return Reader(transform_iterator<SubstituteElement<UnaryFunction>, Iter>(theta, begin()));
    }

    template<typename UnaryFunction>
    struct TraverseElement {
      TraverseElement(UnaryFunction f) : f_(f) {}

      Element operator()(const Element& e) const {
        switch (e.type()) {
          case Element::kClause: return Element::Clause(e.clause().val.Traverse(f_)); break;
          case Element::kNot:    return Element::Not(); break;
          case Element::kOr:     return Element::Or(); break;
          case Element::kExists: return Element::Exists(e.var().val.Traverse(f_)); break;
        }
      }

     private:
      UnaryFunction f_;
    };

    template<typename UnaryFunction>
    Reader<transform_iterator<TraverseElement<UnaryFunction>, Iter>> Substitute(UnaryFunction theta) {
      return Reader(transform_iterator<TraverseElement<UnaryFunction>, Iter>(theta, begin()));
    }

   private:
    friend class Formula;

    explicit Reader(Iter begin) : begin_(begin) {}

    Iter begin_;
    mutable Maybe<Iter> end_ = Nothing;
  };

  static Formula Clause(const Clause& c) { return Atomic(Element::Clause(c)); }
  static Formula Not(const Formula& phi) { return Unary(Element::Not(), phi); }
  static Formula Or(const Formula& phi, const Formula& psi) { return Binary(Element::Or(), phi, psi); }
  static Formula Exists(Term var, const Formula& phi) { return Unary(Element::Exists(var), phi); }

  template<typename Iter>
  explicit Formula(const Reader<Iter>& r) : es_(r.begin(), r.end()) {}
  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  ~Formula() = default;

  bool operator==(const Formula& phi) const { return es_ == phi.es_; }
  bool operator!=(const Formula& phi) const { return !(*this == phi); }

  struct identity {
    template<typename T>
    constexpr auto operator()(T&& x) const noexcept -> decltype(std::forward<T>(x)) {
      return std::forward<T>(x);
    }
  };

  Reader<> reader() const { return Reader<>(es_.begin()); }

 private:
  static Formula Atomic(Element op) {
    assert(op.type() == Element::kClause);
    Formula r;
    r.es_.push_front(op);
    return r;
  }

  static Formula Unary(Element op, Formula s) {
    assert(op.type() == Element::kNot || op.type() == Element::kExists);
    s.es_.push_front(op);
    return s;
  }

  static Formula Binary(Element op, Formula s, Formula r) {
    assert(op.type() == Element::kOr);
    s.es_.splice(s.es_.end(), r.es_);
    s.es_.push_front(op);
    return s;
  }

  Formula() = default;

  std::list<Element> es_;
};

}  // namespace lela

#endif  // SRC_FORMULA_H_

