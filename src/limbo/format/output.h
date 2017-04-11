// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Overloads the stream operator (<<) for Literal, Clause, and so on. These
// operators are only activated when the corresponding header is included.
// For Sort and Symbol objects, a human-readable name can be registered.

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

#include <limbo/term.h>

#include <limbo/internal/compar.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>

#ifndef MARK  // NOLINT
#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)
#endif

namespace limbo {
namespace format {

#ifndef LIMBO_FORMAT_OUTPUT_H_
typedef std::unordered_map<Symbol::Sort, std::string> SortMap;
typedef std::unordered_map<Symbol, std::string> SymbolMap;

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

template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T, H, E>& set);

template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_multiset<T, H, E>& set);

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::map<K, T>& map);

template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const std::multimap<K, T>& map);

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, T, H, E>& map);

template<typename K, typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_multimap<K, T, H, E>& map);
#endif  // LIMBO_FORMAT_OUTPUT_H_

#ifdef LIMBO_INTERNAL_HASHSET_H_
template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const internal::HashSet<T, H, E>& set);
#endif  // LIMBO_INTERNAL_HASHSET_H_

#ifdef LIMBO_INTERNAL_MAYBE_H_
template<typename K, typename T>
std::ostream& operator<<(std::ostream& os, const internal::Maybe<T>& m);
#endif  // LIMBO_INTERNAL_MAYBE_H_

#ifdef LIMBO_TERM_H_
#ifndef LIMBO_TERM_OUTPUT
#define LIMBO_TERM_OUTPUT
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
#endif  // LIMBO_TERM_OUTPUT
#endif  // LIMBO_TERM_H_

#ifdef LIMBO_LITERAL_H_
#ifndef LIMBO_LITERAL_OUTPUT
#define LIMBO_LITERAL_OUTPUT
std::ostream& operator<<(std::ostream& os, const Literal a) {
  os << a.lhs() << ' ' << (a.pos() ? "\u003D" : "\u2260") << ' ' << a.rhs();
  return os;
}
#endif  // LIMBO_LITERAL_OUTPUT_
#endif  // LIMBO_LITERAL_H_

#ifdef LIMBO_CLAUSE_H_
#ifndef LIMBO_CLAUSE_OUTPUT
#define LIMBO_CLAUSE_OUTPUT
std::ostream& operator<<(std::ostream& os, const Clause& c) {
  struct PrintLiteralComparator {
    struct PrintTermComparator {
      struct PrintSymbolComparator {
        typedef Symbol value_type;

        bool operator()(Symbol s1, Symbol s2) const {
          internal::Maybe<std::string> n1 = LookupSymbol(s1);
          internal::Maybe<std::string> n2 = LookupSymbol(s2);
          return n1 && n2  ? n1.val < n2.val :
                 n1 && !n2 ? true :
                 !n1 && n2 ? false :
                             s1.hash() < s2.hash();
        }
      };

      typedef Term value_type;

      inline bool operator()(Term t1, Term t2) const {
        internal::LexicographicComparator<
            PrintSymbolComparator,
            internal::LessComparator<Symbol::Arity>,
            internal::LexicographicContainerComparator<Term::Vector, PrintTermComparator>> comp;
        return comp(t1.symbol(), t1.arity(), t1.args(),
                    t2.symbol(), t2.arity(), t2.args());
      }
    };

    bool operator()(Literal l1, Literal l2) const {
      return comp(l1.lhs(), l1.rhs(), l1.pos(),
                  l2.lhs(), l2.rhs(), l2.pos());
    }

    internal::LexicographicComparator<
        PrintTermComparator,
        PrintTermComparator,
        internal::LessComparator<bool>> comp;
  };
  std::vector<Literal> vec(c.begin(), c.end());
  std::sort(vec.begin(), vec.end(), PrintLiteralComparator());
  return print_range(os, vec, "[", "]", " \u2228 ");
}
#endif  // LIMBO_OUTPUT_CLAUSE
#endif  // LIMBO_CLAUSE_H_

#ifdef LIMBO_SETUP_H_
#ifndef LIMBO_SETUP_OUTPUT
#define LIMBO_SETUP_OUTPUT
std::ostream& operator<<(std::ostream& os, const Setup& s) {
  auto is = s.clauses();
  auto cs = internal::transform_range(is.begin(), is.end(), [&s](size_t i) { return s.clause(i); });
  return print_range(os, cs, "{ ", "\n}", "\n, ");
}
#endif  // LIMBO_SETUP_OUTPUT
#endif  // LIMBO_SETUP_H_

#ifdef LIMBO_FORMULA_H_
#ifndef LIMBO_FORMULA_OUTPUT
#define LIMBO_FORMULA_OUTPUT
std::ostream& operator<<(std::ostream& os, const Formula& alpha) {
  switch (alpha.type()) {
    case Formula::kAtomic: {
      const Clause& c = alpha.as_atomic().arg();
#ifdef PRINT_ABBREVIATIONS
      if (c.empty()) {
        os << "\u22A5";
      } else if (c.unit()) {
        os << c.first();
      } else {
        auto as = internal::transform_range(c.begin(), c.end(), [](Literal a) { return a; });
        print_sequence(os, as.begin(), as.end(), "[", "]", " \u2227 ");
      }
#else
      os << c;
#endif
      break;
    }
    case Formula::kNot:
#ifdef PRINT_ABBREVIATIONS
      if (alpha.as_not().arg().type() == Formula::kOr &&
          alpha.as_not().arg().as_or().lhs().type() == Formula::kNot &&
          alpha.as_not().arg().as_or().rhs().type() == Formula::kNot) {
        os << '('
           << alpha.as_not().arg().as_or().lhs().as_not().arg()
           << ' ' << "\u2227" << ' '
           << alpha.as_not().arg().as_or().rhs().as_not().arg()
           << ')';
      } else if (alpha.as_not().arg().type() == Formula::kNot) {
        os << alpha.as_not().arg().as_not().arg();
      } else if (alpha.as_not().arg().type() == Formula::kAtomic) {
        const Clause& c = alpha.as_not().arg().as_atomic().arg();
        if (c.empty()) {
          os << "\u22A4";
        } else if (c.unit()) {
          os << c.first().flip();
        } else {
          auto as = internal::transform_range(c.begin(), c.end(), [](Literal a) { return a.flip(); });
          print_sequence(os, as.begin(), as.end(), "[", "]", " \u2227 ");
        }
      } else if (alpha.as_not().arg().type() == Formula::kExists &&
                 alpha.as_not().arg().as_exists().arg().type() == Formula::kNot) {
        os << "\u2200" << alpha.as_not().arg().as_exists().x() << alpha.as_not().arg().as_exists().arg().as_not().arg();
      } else {
        os << "\u00AC" << alpha.as_not().arg();
      }
#else
      os << "\u00AC" << alpha.as_not().arg();
#endif
      break;
    case Formula::kOr:
      os << '(' << alpha.as_or().lhs() << ' ' << "\u2228" << ' ' << alpha.as_or().rhs() << ')';
      break;
    case Formula::kExists:
      os << "\u2203" << alpha.as_exists().x() << alpha.as_exists().arg();
      break;
    case Formula::kKnow:
      os << "K<" << alpha.as_know().k() << "> " << alpha.as_know().arg();
      break;
    case Formula::kCons:
      os << "M<" << alpha.as_cons().k() << "> " << alpha.as_cons().arg();
      break;
    case Formula::kBel:
      os << "B<" << alpha.as_bel().k() << "," << alpha.as_bel().l() << "> " << alpha.as_bel().antecedent() <<
          " \u27FE  " << alpha.as_bel().consequent();
      break;
    case Formula::kGuarantee:
      os << "G " << alpha.as_guarantee().arg();
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula::Ref& alpha) {
  return os << *alpha;
}
#endif  // LIMBO_FORMULA_OUTPUT
#endif  // LIMBO_FORMULA_H_

#ifndef LIMBO_FORMAT_OUTPUT_H_
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

template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T, H, E>& set) {
  print_sequence(os, set.begin(), set.end(), "{", "}", ", ");
  return os;
}

template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const std::unordered_multiset<T, H, E>& set) {
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
#endif  // LIMBO_FORMAT_OUTPUT_H_

#ifdef LIMBO_INTERNAL_HASHSET_H_
#ifndef LIMBO_INTERNAL_HASHSET_OUTPUT
#define LIMBO_INTERNAL_HASHSET_OUTPUT
template<typename T, typename H, typename E>
std::ostream& operator<<(std::ostream& os, const internal::HashSet<T, H, E>& set) {
  print_sequence(os, set.begin(), set.end(), "{", "}", ", ");
  return os;
}
#endif  // LIMBO_INTERNAL_HASHSET_OUTPUT
#endif  // LIMBO_INTERNAL_HASHSET_H_

#ifdef LIMBO_INTERNAL_MAYBE_H_
#ifndef LIMBO_INTERNAL_MAYBE_OUTPUT
#define LIMBO_INTERNAL_MAYBE_OUTPUT
template<typename T>
std::ostream& operator<<(std::ostream& os, const internal::Maybe<T>& m) {
  if (m) {
    os << "Just(" << m.val << ")";
  } else {
    os << "Nothing";
  }
  return os;
}
#endif  // LIMBO_INTERNAL_MAYBE_OUTPUT
#endif  // LIMBO_INTERNAL_MAYBE_H_

}  // namespace format
}  // namespace limbo

#ifndef LIMBO_FORMAT_OUTPUT_H_
#define LIMBO_FORMAT_OUTPUT_H_
#endif  // LIMBO_FORMAT_OUTPUT_H_

