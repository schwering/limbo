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

typedef std::map<Symbol, std::string, Symbol::Comparator> SymbolMap;

inline SymbolMap* symbol_map() {
  static SymbolMap map;
  return &map;
}

inline void RegisterSymbol(Symbol s, const std::string& n) {
  (*symbol_map())[s] = n;
}

inline Maybe<std::string> LookupSymbol(Symbol s) {
  auto it = symbol_map()->find(s);
  if (it != symbol_map()->end()) return Just(it->second);
  else                           return Nothing;
}

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2> p) {
  os << "(" << p.first << ", " << p.second << ")";
  return os;
}

template<typename Iter>
std::ostream& print_sequence(std::ostream& os,
                             Iter begin,
                             Iter end,
                             const char* pre = "[",
                             const char* post = "]",
                             const char* sep = ", ");

template<typename Range>
std::ostream& print_range(std::ostream& os, Range r,
                          const char* pre = "[",
                          const char* post = "]",
                          const char* sep = ", ") {
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
  Maybe<std::string> n = LookupSymbol(s);
  if (n) {
    os << n.val;
  } else {
    if (s.function()) {
      os << 'f';
    } else if (s.name()) {
      os << '#';
    } else if (s.variable()) {
      os << 'x';
    }
    os << s.id();
  }
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
    Clause operator()(const Setup::Index i) const { return owner_->clause(i); }
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
      //if (phi.arg().head().type() == Formula::Element::kOr &&
      //    phi.arg().left().head().type() == Formula::Element::kNot &&
      //    phi.arg().right().head().type() == Formula::Element::kNot) {
      //  os << '(' << phi.arg().left().arg() << ' ' << "\u2227" << ' ' << phi.arg().right().arg() << ')';
      //} else if (phi.arg().head().type() == Formula::Element::kClause) {
      //  const Clause& c = phi.arg().head().clause().val;
      //  print_sequence(os, c.begin(), c.end(), "[", "]", " \u2227 ");
      //} else if (phi.arg().head().type() == Formula::Element::kExists &&
      //           phi.arg().arg().head().type() == Formula::Element::kNot) {
      //  os << "\u2200" << phi.arg().head().var().val << phi.arg().arg().arg();
      //} else {
        os << "\u00AC" << phi.arg();
      //}
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

template<typename Iter>
std::ostream& print_sequence(std::ostream& os,
                             Iter begin,
                             Iter end,
                             const char* pre,
                             const char* post,
                             const char* sep) {
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
}  // namespace output


namespace input {

class HiSymbol;
class HiTerm;
class Context;

class HiTerm : public Term {
 public:
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

class HiFormula : public Formula {
 public:
  HiFormula(Literal l) : HiFormula(Clause({l})) {}
  HiFormula(const lela::Clause& c) : HiFormula(Formula::Clause(c)) {}
  HiFormula(const Formula& phi) : Formula(phi) {}
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

inline HiFormula operator==(HiTerm t1, HiTerm t2) { return HiFormula(Literal::Eq(t1, t2)); }
inline HiFormula operator!=(HiTerm t1, HiTerm t2) { return HiFormula(Literal::Neq(t1, t2)); }

inline HiFormula operator~(const HiFormula& phi) { return Formula::Not(phi); }
inline HiFormula operator!(const HiFormula& phi) { return ~phi; }
inline HiFormula operator||(const HiFormula& phi, const HiFormula& psi) { return Formula::Or(phi, psi); }
inline HiFormula operator&&(const HiFormula& phi, const HiFormula& psi) { return ~(~phi || ~psi); }

inline HiFormula operator>>(const HiFormula& phi, const HiFormula& psi) { return (~phi || psi); }
inline HiFormula operator<<(const HiFormula& phi, const HiFormula& psi) { return (phi || ~psi); }

inline HiFormula operator==(const HiFormula& phi, const HiFormula& psi) { return (phi >> psi) && (phi << psi); }

inline HiFormula Ex(HiTerm x, const HiFormula& phi) { return Formula::Exists(x, phi); }
inline HiFormula Fa(HiTerm x, const HiFormula& phi) { return ~Ex(x, ~phi); }

}  // namespace input

}  // namespace lela

#endif  // SRC_PRINT_H_

