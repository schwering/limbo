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
// the owning Formula is alive. NF() implements a normal form similar to
// negation normal form which however preserves clauses (and it turns
// clauses that are not explicitly represented as such into ones).
//
// Internally it's stored in Polish notation as a list of Element objects.
// Element would be a union if C++ would allow non-trivial types in unions.

#ifndef LELA_FORMULA_H_
#define LELA_FORMULA_H_

#include <cassert>

#include <list>

#include <lela/clause.h>
#include <lela/setup.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>

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
    const internal::Maybe<lela::Clause>& clause() const { return clause_; }
    internal::Maybe<Term> var() const { return var_; }

   private:
    explicit Element(Type type) : type_(type) {}
    Element(Type type, Term var) : type_(type), var_(internal::Just(var)) {}
    Element(Type type, const lela::Clause& c) : type_(type), clause_(internal::Just(c)) {}

    Type type_;
    internal::Maybe<lela::Clause> clause_ = internal::Nothing;
    internal::Maybe<Term> var_ = internal::Nothing;
  };

  template<typename Iter = std::list<Element>::const_iterator>
  class Reader {
   public:
    Iter begin() const { return begin_; }
    Iter end() const { return end_; }

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

    Formula NF() const {
      switch (head().type()) {
        case Element::kClause: {
          return Formula(*this);
        }
        case Element::kOr: {
          Formula phi = left().NF();
          Formula psi = right().NF();
          auto phi_q = phi.reader().quantifiers();
          auto psi_q = psi.reader().quantifiers();
          Reader phi_r = Reader(phi_q.end());
          Reader psi_r = Reader(psi_q.end());
          if (phi_r.head().type() == Element::kClause && (phi_q.even() || phi_r.head().clause().val.unit()) &&
              psi_r.head().type() == Element::kClause && (psi_q.even() || psi_r.head().clause().val.unit())) {
            std::list<Element> phi_p(phi_q.begin(), phi_q.end());
            std::list<Element> psi_p(psi_q.begin(), psi_q.end());
            lela::Clause phi_c = phi_r.head().clause().val;
            lela::Clause psi_c = psi_r.head().clause().val;
            if (!phi_q.even()) {
              assert(phi_c.unit());
              phi_p.push_back(Element::Not());
              phi_c = lela::Clause({phi_c.begin()->flip()});
            }
            if (!psi_q.even()) {
              assert(psi_c.unit());
              psi_p.push_back(Element::Not());
              psi_c = lela::Clause({psi_c.begin()->flip()});
            }
            auto ls = internal::join_ranges(phi_c.begin(), phi_c.end(), psi_c.begin(), psi_c.end());
            const lela::Clause c(ls.begin(), ls.end());
            Formula r;
            r.es_.push_front(Element::Clause(c));
            r.es_.splice(r.es_.begin(), psi_p);
            r.es_.splice(r.es_.begin(), phi_p);
            return r;
          } else {
            return Or(phi, psi);
          }
        }
        case Element::kExists: {
          return Exists(head().var().val, arg().NF());
        }
        case Element::kNot: {
          switch (arg().head().type()) {
            case Element::kClause: {
              const lela::Clause c = arg().head().clause().val;
              if (c.unit()) {
                return Clause(lela::Clause({c.begin()->flip()}));
              } else {
                return Clause(c);
              }
            }
            case Element::kOr:
              return Not(arg().NF());
            case Element::kNot:
              return arg().arg().NF();
            case Element::kExists:
              return Not(Exists(arg().head().var().val, arg().arg().NF()));
          }
        }
      }
    }

    template<typename UnaryFunction>
    struct SubstituteElement {
      SubstituteElement() = default;
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
    Reader<internal::transform_iterator<Iter, SubstituteElement<UnaryFunction>>>
    Substitute(UnaryFunction theta, Term::Factory* tf) const {
      typedef internal::transform_iterator<Iter, SubstituteElement<UnaryFunction>> iterator;
      iterator it = iterator(begin(), SubstituteElement<UnaryFunction>(theta, tf));
      return Reader<iterator>(it);
    }

    template<typename UnaryFunction>
    void Traverse(UnaryFunction f) const {
      for (const Element& e : *this) {
        switch (e.type()) {
          case Element::kClause: e.clause().val.Traverse(f); break;
          case Element::kNot:    break;
          case Element::kOr:     break;
          case Element::kExists: /*e.var().val.Traverse(f);*/ break;
        }
      }
    }

    struct QuantifierPrefix {
      QuantifierPrefix(Iter begin, Iter end) : begin_(begin), end_(end) {}

      Iter begin() const { return begin_; }
      Iter end()   const { return end_; }

      bool even() const {
        return std::count_if(begin(), end(), [](const Element& e) { return e.type() == Element::kNot; }) % 2 == 0;
      }

     private:
      Iter begin_;
      Iter end_;
    };

    QuantifierPrefix quantifiers() const {
      auto last = std::find_if_not(begin(), end(), [](const Element& e) {
        return e.type() == Element::kNot || e.type() == Element::kExists;
      });
      return QuantifierPrefix(begin(), last);
    }

   private:
    friend class Formula;

    explicit Reader(Iter begin) : begin_(begin), end_(begin) {
      for (int n = 1; n > 0; --n, ++end_) {
        switch ((*end_).type()) {
          case Element::kClause: n += 0; break;
          case Element::kNot:    n += 1; break;
          case Element::kOr:     n += 2; break;
          case Element::kExists: n += 1; break;
        }
      }
    }

    Iter begin_;
    Iter end_;
  };

  static Formula Clause(const lela::Clause& c) { return Atomic(Element::Clause(c)); }
  static Formula Not(const Formula& phi) { return Unary(Element::Not(), phi); }
  static Formula Or(const Formula& phi, const Formula& psi) { return Binary(Element::Or(), phi, psi); }
  static Formula Exists(Term var, const Formula& phi) { return Unary(Element::Exists(var), phi); }

  template<typename Iter>
  explicit Formula(const Reader<Iter>& r) : es_(r.begin(), r.end()) {}

  bool operator==(const Formula& phi) const { return es_ == phi.es_; }
  bool operator!=(const Formula& phi) const { return !(*this == phi); }

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
    r.es_.splice(r.es_.begin(), s.es_);
    r.es_.push_front(op);
    return r;
  }

  Formula() = default;

  std::list<Element> es_;
};

}  // namespace lela

#endif  // LELA_FORMULA_H_

