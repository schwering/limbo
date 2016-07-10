// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Substitutes standard names for variables. Corresponds to the gnd() operator.

#ifndef SRC_FORMULA_H_
#define SRC_FORMULA_H_

#include <cassert>
#include <list>
#include "./clause.h"
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

union Formula {
 public:
  static Formula Not(const Formula& phi) {
    return Formula(Element::Not(), phi);
  }

  Formula(const Formula&) = default;
  Formula& operator=(const Formula&) = default;
  ~Formula() = default;

 private:
  enum Kind { kDisjunction, kNegation, kExistential, kClause };
  struct Element {
    static Element Not() { return Element(kDisjunction); }
    static Element Or() { return Element(kNegation); }
    static Element Exists(Term var) { assert(var.variable()); return Element(kExistential, var); }
    static Element Clause(const Clause& c) { return Element(kClause, c); }

    explicit Element(Kind kind) : kind_(kind) {}
    Element(Kind kind, Term var) : kind_(kind), var_(var) {}
    Element(Kind kind, const lela::Clause& c) : kind_(kind), clause_(c) {}

    Kind kind_;
    lela::Clause clause_;
    Term var_;
  };

  Formula(Element e, const Formula& phi) : es_(phi.es_) { es_.push_back(e); }

  std::list<Element> es_;
};

}  // namespace lela

#endif  // SRC_FORMULA_H_

