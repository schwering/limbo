// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Overloads the stream operator (<<) for Literal, Clause, and so on. For
// Sort and Symbol objects, a human-readable name can be registered.

#ifndef LELA_FORMAT_OUTPUT_H_
#define LELA_FORMAT_OUTPUT_H_

#include <algorithm>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

#include <lela/formula.h>
#include <lela/literal.h>
#include <lela/setup.h>
#include <lela/term.h>
#include <lela/internal/maybe.h>

#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)

namespace lela {
namespace format {
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

inline void UnregisterAll() {
  sort_map()->clear();
  symbol_map()->clear();
}

inline void RegisterSort(Symbol::Sort s, const std::string& n) {
  (*sort_map())[s] = n;
}

inline void RegisterSymbol(Symbol s, const std::string& n) {
  (*symbol_map())[s] = n;
}

inline internal::Maybe<std::string> LookupSort(Symbol::Sort s) {
  auto it = sort_map()->find(s);
  if (it != sort_map()->end()) return internal::Just(it->second);
  else                         return internal::Nothing;
}

inline internal::Maybe<std::string> LookupSymbol(Symbol s) {
  auto it = symbol_map()->find(s);
  if (it != symbol_map()->end()) return internal::Just(it->second);
  else                           return internal::Nothing;
}

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2> p);

template<typename InputIt>
std::ostream& print_sequence(std::ostream& os,
                             InputIt begin,
                             InputIt end,
                             const char* pre = "[",
                             const char* post = "]",
                             const char* sep = ", ");

template<typename Range>
std::ostream& print_range(std::ostream& os, const Range& r,
                          const char* pre = "[",
                          const char* post = "]",
                          const char* sep = ", ");

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& list);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& set);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::multiset<T>& set);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& set);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_multiset<T>& set);

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::map<K, T>& map);

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::multimap<K, T>& map);

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, T, H, E>& map);

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_multimap<K, T, H, E>& map);

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const internal::Maybe<T>& m);

std::ostream& operator<<(std::ostream& os, const Symbol s) {
  internal::Maybe<std::string> sort_name = LookupSort(s.sort());
  internal::Maybe<std::string> symbol_name = LookupSymbol(s);
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
    internal::Maybe<std::string> n1 = LookupSymbol(s1);
    internal::Maybe<std::string> n2 = LookupSymbol(s2);
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
  internal::LexicographicComparator<
      PrintSymbolComparator,
      internal::LessComparator<Symbol::Arity>,
      internal::LexicographicContainerComparator<Term::Vector, PrintTermComparator>> comp;
  return comp(t1.symbol(), t1.arity(), t1.args(),
              t2.symbol(), t2.arity(), t2.args());
}

struct PrintLiteralComparator {
  typedef Literal value_type;

  bool operator()(Literal l1, Literal l2) const {
    return comp(l1.lhs(), l1.rhs(), l1.pos(),
                l2.lhs(), l2.rhs(), l2.pos());
  }

  internal::LexicographicComparator<
      PrintTermComparator,
      PrintTermComparator,
      internal::LessComparator<bool>> comp;
};

struct PrintClauseComparator {
  typedef Clause value_type;

  bool operator()(const Clause& c1, const Clause& c2) const {
    return comp(c1, c2);
  }

  internal::LexicographicContainerComparator<Clause, PrintLiteralComparator> comp;
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

std::ostream& operator<<(std::ostream& os, const std::vector<Literal>& c) {
  std::vector<Literal> vec(c.begin(), c.end());
  std::sort(vec.begin(), vec.end(), PrintLiteralComparator());
  print_range(os, vec, "[", "]", " \u2228 ");
  return os;
}

std::ostream& operator<<(std::ostream& os, const Setup& s) {
  struct Get {
    explicit Get(const Setup* owner) : owner_(owner) {}
    std::vector<Literal> operator()(const Setup::ClauseIndex i) const {
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

    internal::LexicographicComparator<
        internal::LessComparator<size_t>,
        internal::LexicographicContainerComparator<std::vector<Literal>, PrintLiteralComparator>> comp;
  };
  auto cs = s.clauses();
  auto r = internal::transform_range(cs.begin(), cs.end(), Get(&s));
  std::vector<std::vector<Literal>> vec(r.begin(), r.end());
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

template<typename InputIt>
std::ostream& print_sequence(std::ostream& os,
                             InputIt begin,
                             InputIt end,
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

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::pair<T1, T2> p) {
  os << "(" << p.first << ", " << p.second << ")";
  return os;
}

template<typename Range>
std::ostream& print_range(std::ostream& os, const Range& r,
                          const char* pre,
                          const char* post,
                          const char* sep) {
  return print_sequence(os, r.begin(), r.end(), pre, post, sep);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  print_sequence(os, vec.begin(), vec.end(), "[", "]", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& list) {
  os << '[';
  print_sequence(os, list.begin(), list.end(), "[", "]", ", ");
  os << '[';
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& set) {
  print_sequence(os, set.begin(), set.end(), "{", "}", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::multiset<T>& set) {
  print_sequence(os, set.begin(), set.end(), "m{", "}m", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& set) {
  print_sequence(os, set.begin(), set.end(), "{", "}", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_multiset<T>& set) {
  print_sequence(os, set.begin(), set.end(), "m{", "}m", ", ");
  return os;
}

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::map<K, T>& map) {
  print_sequence(os, map.begin(), map.end(), "{", "}", ", ");
  return os;
}

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::multimap<K, T>& map) {
  print_sequence(os, map.begin(), map.end(), "m{", "}m", ", ");
  return os;
}

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, T, H, E>& map) {
  print_sequence(os, map.begin(), map.end(), "{", "}", ", ");
  return os;
}

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_multimap<K, T, H, E>& map) {
  print_sequence(os, map.begin(), map.end(), "m{", "}m", ", ");
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const internal::Maybe<T>& m) {
  if (m) {
    os << "Just(" << m.val << ")";
  } else {
    os << "Nothing";
  }
  return os;
}

}  // namespace output
}  // namespace format
}  // namespace lela

#endif  // LELA_FORMAT_OUTPUT_H_

