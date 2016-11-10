// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Two namespaces, input and output, provide some procedures to pretty-print
// and to create formulas and related structures.

#ifndef SRC_PRINT_H_
#define SRC_PRINT_H_

#include <list>
#include <map>
#include <ostream>
#include <set>
#include <utility>
#include <vector>
#include "./clause.h"
#include "./formula.h"
#include "./literal.h"
#include "./iter.h"
#include "./maybe.h"
#include "./term.h"

#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)

namespace lela {

namespace output {

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2> p) {
  os << "(" << p.first << ", " << p.second << ")";
  return os;
}

template<typename Iter>
std::ostream& print_sequence(std::ostream& os, Iter begin, Iter end, const char* pre = "[", const char* post = "]", const char* sep = ", ") { // NOLINT
  bool first = true;
  os << pre;
  for (auto it = begin; it != end; ++it) {
    if (!first) {
      os << sep;
    }
    os << *it;
    first = false;
  }
  os << post;
  return os;
}

template<typename Range>
std::ostream& print_range(std::ostream& os, Range r, const char* pre = "[", const char* post = "]", const char* sep = ", ") { // NOLINT
  return print_sequence(os, r.begin(), r.end(), pre, post, sep);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T> vec) {
  print_sequence(os, vec.begin(), vec.end(), "[", "]", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T> list) {
  os << '[';
  print_sequence(os, list.begin(), list.end(), "[", "]", ", ");
  os << '[';
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T> set) {
  print_sequence(os, set.begin(), set.end(), "{", "}", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::multiset<T> set) {
  print_sequence(os, set.begin(), set.end(), "m{", "}m", ", ");
  return os;
}

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::map<K, T> map) {
  print_sequence(os, map.begin(), map.end(), "{", "}", ", ");
  return os;
}

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::multimap<K, T> map) {
  print_sequence(os, map.begin(), map.end(), "m{", "}m", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Maybe<T>& m) {
  if (m) {
    os << "Just(" << m.val << ")";
  } else {
    os << "Nothing";
  }
  return os;
}

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const Maybe<T1, T2>& m) {
  if (m) {
    os << "Just(" << m.val1 << ", " << m.val2 << ")";
  } else {
    os << "Nothing";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Symbol s) {
  if (s.function()) {
    os << 'f';
  } else if (s.name()) {
    os << '#';
  } else if (s.variable()) {
    os << 'x';
  }
  os << s.id();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Term t) {
  if (t.null()) {
    os << "nullterm";
  } else {
    os << t.symbol();
    if (t.arity() > 0) {
      auto seq = t.args();
      print_sequence(os, seq.begin(), seq.end(), "(", ")", ",");
    }
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Literal a) {
  os << a.lhs() << ' ' << (a.pos() ? "\u003D" : "\u2260") << ' ' << a.rhs();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  print_sequence(os, c.begin(), c.end(), "[", "]", " \u2228 ");
  return os;
}

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  struct Get {
    explicit Get(const Setup* owner) : owner_(owner) {}
    std::pair<int, Clause> operator()(const Setup::Index i) const { return std::make_pair(i, owner_->clause(i)); }
   private:
    const Setup* owner_;
  };
  Setup::Clauses cs = s.clauses();
  auto begin = transform_iterator<Get, Setup::Clauses::clause_iterator>(Get(&s), cs.begin());
  auto end = transform_iterator<Get, Setup::Clauses::clause_iterator>(Get(&s), cs.end());
  print_sequence(os, begin, end, "{ ", "\n}", "\n, ");
  return os;
}

template<typename UnaryFunction>
std::ostream& operator<<(std::ostream& os, const Formula::Reader<UnaryFunction>& phi) {
  switch (phi.head().type()) {
    case Formula::Element::kClause:
      os << phi.head().clause().val;
      break;
    case Formula::Element::kNot:
      if (phi.arg().head().type() == Formula::Element::kOr &&
          phi.arg().left().head().type() == Formula::Element::kNot &&
          phi.arg().right().head().type() == Formula::Element::kNot) {
        os << '(' << phi.arg().left().arg() << ' ' << "\u2227" << ' ' << phi.arg().right().arg() << ')';
      } else if (phi.arg().head().type() == Formula::Element::kClause) {
        const Clause& c = phi.arg().head().clause().val;
        print_sequence(os, c.begin(), c.end(), "[", "]", " \u2227 ");
      } else if (phi.arg().head().type() == Formula::Element::kExists &&
                 phi.arg().arg().head().type() == Formula::Element::kNot) {
        os << "\u2200" << phi.arg().head().var().val << phi.arg().arg().arg();
      } else {
        os << "\u00AC" << phi.arg();
      }
      break;
    case Formula::Element::kOr:
      os << '(' << phi.left() << ' ' << "\u2228" << ' ' << phi.right() << ')';
      break;
    case Formula::Element::kExists:
      os << "\u2203" << phi.head().var().val << phi.arg();
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula& phi) {
  os << phi.reader();
  return os;
}

}  // namespace output


namespace input {

class Context {
 public:
  class Term;
  class Formula;

  class Symbol : public lela::Symbol {
   public:
    Symbol(Context* ctx, lela::Symbol s) : lela::Symbol(s), ctx_(ctx) {}

    template<typename... Args>
    Term operator()(Args... args) const { return Term(ctx_->tf()->CreateTerm(*this, lela::Term::Vector({args...}))); }

   private:
    Context* const ctx_;
  };

  class Term : public lela::Term {
   public:
    explicit Term(lela::Term t) : lela::Term(t) {}
  };

  class Formula : public lela::Formula {
   public:
    Formula(lela::Literal l) : Formula(lela::Clause({l})) {}
    Formula(const lela::Clause& c) : Formula(lela::Formula::Clause(c)) {}
    Formula(const lela::Formula& phi) : lela::Formula(phi) {}
  };

  Symbol::Sort NewSort() { return sf()->CreateSort(); }
  Term NewName(Symbol::Sort sort) { return Term(tf()->CreateTerm(Symbol(this, sf()->CreateName(sort)))); }
  Term NewVar(Symbol::Sort sort) { return Term(tf()->CreateTerm(Symbol(this, sf()->CreateVariable(sort)))); }
  Symbol NewFun(Symbol::Sort sort, Symbol::Arity arity) { return Symbol(this, sf()->CreateFunction(sort, arity)); }

 private:
  lela::Symbol::Factory* sf() { return &sf_; }
  lela::Term::Factory* tf() { return &tf_; }

  lela::Symbol::Factory sf_;
  lela::Term::Factory tf_;
};

inline Context::Formula operator==(Context::Term t1, Context::Term t2) { return Context::Formula(Literal::Eq(t1, t2)); }
inline Context::Formula operator!=(Context::Term t1, Context::Term t2) { return Context::Formula(Literal::Neq(t1, t2)); }

inline Context::Formula operator~(const Context::Formula& phi) { return lela::Formula::Not(phi); }
inline Context::Formula operator!(const Context::Formula& phi) { return ~phi; }
inline Context::Formula operator||(const Context::Formula& phi, const Context::Formula& psi) { return lela::Formula::Or(phi, psi); }
inline Context::Formula operator&&(const Context::Formula& phi, const Context::Formula& psi) { return ~(~phi || ~psi); }

inline Context::Formula operator>>(const Context::Formula& phi, const Context::Formula& psi) { return (~phi || psi); }
inline Context::Formula operator<<(const Context::Formula& phi, const Context::Formula& psi) { return (phi || ~psi); }

inline Context::Formula operator==(const Context::Formula& phi, const Context::Formula& psi) { return (phi >> psi) && (phi << psi); }

inline Context::Formula Ex(Term x, const Context::Formula& phi) { return lela::Formula::Exists(x, phi); }
inline Context::Formula Fa(Term x, const Context::Formula& phi) { return ~Ex(x, ~phi); }

}  // namespace input

}  // namespace lela

#endif  // SRC_PRINT_H_

