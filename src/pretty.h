// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Two namespaces, input and output, provide some procedures to pretty-print
// and to create formulas and related structures.

#ifndef SRC_PRETTY_H_
#define SRC_PRETTY_H_

#include <algorithm>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
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

typedef std::map<Symbol::Sort, std::string> SortMap;
typedef std::map<Symbol, std::string, Symbol::Comparator> SymbolMap;

inline SortMap* sort_map() {
  static SortMap map;
  return &map;
}

inline SymbolMap* symbol_map() {
  static SymbolMap map;
  return &map;
}

inline void RegisterSort(Symbol::Sort s, const std::string& n) {
  (*sort_map())[s] = n;
}

inline void RegisterSymbol(Symbol s, const std::string& n) {
  (*symbol_map())[s] = n;
}

inline Maybe<std::string> LookupSort(Symbol::Sort s) {
  auto it = sort_map()->find(s);
  if (it != sort_map()->end()) return Just(it->second);
  else                         return Nothing;
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
std::ostream& print_range(std::ostream& os, const Range& r,
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
  Maybe<std::string> sort_name = LookupSort(s.sort());
  Maybe<std::string> symbol_name = LookupSymbol(s);
  if (sort_name) {
    os << sort_name.val << (sort_name.val.length() > 0 ? "." : "");
  } else {
    os << static_cast<int>(s.sort()) << ".";
  }
  if (symbol_name) {
    os << symbol_name.val;
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

struct PrintSymbolComparator {
  typedef Symbol value_type;

  bool operator()(Symbol s1, Symbol s2) const {
    Maybe<std::string> n1 = LookupSymbol(s1);
    Maybe<std::string> n2 = LookupSymbol(s2);
    return n1 && n2  ? n1.val < n2.val :
           n1 && !n2 ? true :
           !n1 && n2 ? false :
                       comp(s1, s2);
  }

  Symbol::Comparator comp;
};

struct PrintTermComparator {
  typedef Term value_type;

  bool operator()(Term t1, Term t2) const;
};

inline bool PrintTermComparator::operator()(Term t1, Term t2) const {
  LexicographicComparator<PrintSymbolComparator,
                          LessComparator<Symbol::Arity>,
                          LexicographicContainerComparator<Term::Vector, PrintTermComparator>> comp;
  return comp(t1.symbol(), t1.arity(), t1.args(),
              t2.symbol(), t2.arity(), t2.args());
}

struct PrintLiteralComparator {
  typedef Literal value_type;

  bool operator()(Literal l1, Literal l2) const {
    return comp(l1.lhs(), l1.rhs(), l1.pos(),
                l2.lhs(), l2.rhs(), l2.pos());
  }

  LexicographicComparator<PrintTermComparator,
                          PrintTermComparator,
                          LessComparator<bool>> comp;
};

struct PrintClauseComparator {
  typedef Clause value_type;

  bool operator()(const Clause& c1, const Clause& c2) const {
    return comp(c1, c2);
  }

  LexicographicContainerComparator<Clause, PrintLiteralComparator> comp;
};

std::ostream& operator<<(std::ostream& os, const Literal a) {
  os << a.lhs() << ' ' << (a.pos() ? "\u003D" : "\u2260") << ' ' << a.rhs();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  std::vector<Literal> vec(c.begin(), c.end());
  std::sort(vec.begin(), vec.end(), PrintLiteralComparator());
  print_range(os, vec, "[", "]", " \u2228 ");
  return os;
}

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  struct Get {
    explicit Get(const Setup* owner) : owner_(owner) {}
    std::vector<Literal> operator()(const Setup::Index i) const {
      // Sort here already so that the sorting of the clauses operators on
      // sorted clauses. Do not use Clause here, because that class will change
      // the ordering.
      const Clause& c = owner_->clause(i);
      std::vector<Literal> vec(c.begin(), c.end());
      std::sort(vec.begin(), vec.end(), PrintLiteralComparator());
      return vec;
    }
   private:
    const Setup* owner_;
  };
  struct Comp {
    bool operator()(const std::vector<Literal>& vec1, const std::vector<Literal>& vec2) const {
      return comp(vec1.size(), vec1, vec2.size(), vec2);
    }

    LexicographicComparator<LessComparator<size_t>,
                            LexicographicContainerComparator<std::vector<Literal>, PrintLiteralComparator>> comp;
  };
  Setup::Clauses cs = s.clauses();
  auto begin = transform_iterator<Get, Setup::Clauses::clause_iterator>(Get(&s), cs.begin());
  auto end = transform_iterator<Get, Setup::Clauses::clause_iterator>(Get(&s), cs.end());
  std::vector<std::vector<Literal>> vec(begin, end);
  std::sort(vec.begin(), vec.end(), Comp());
  print_range(os, vec, "{ ", "\n}", "\n, ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Formula::Reader<T>& phi) {
  switch (phi.head().type()) {
    case Formula::Element::kClause:
      os << phi.head().clause().val;
      break;
    case Formula::Element::kNot:
#ifdef PRINT_ABBREVIATIONS
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
#else
      os << "\u00AC" << phi.arg();
#endif
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

class HiTerm : public Term {
 public:
  explicit HiTerm(Term t) : Term(t) {}
};

class HiLiteral : public Literal {
 public:
  explicit HiLiteral(Literal t) : Literal(t) {}
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

}  // namespace input

}  // namespace lela

#endif  // SRC_PRETTY_H_

