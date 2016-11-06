// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

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
  os << a.lhs();
  if (a.pos()) {
    os << " \u003D ";
  } else {
    os << " \u2260 ";
  }
  os << a.rhs();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  print_sequence(os, c.begin(), c.end(), "[", "]", ", ");
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
    case Formula::Element::kClause: os << phi.head().clause(); break;
    case Formula::Element::kNot:    os << '~' << phi.arg(); break;
    case Formula::Element::kOr:     os << '(' << phi.left() << " v " << phi.right() << ')'; break;
    case Formula::Element::kExists: os << 'E' << phi.head().var() << ' ' << phi.arg(); break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula& phi) {
  os << phi.reader();
  return os;
}

}  // namespace lela

#endif  // SRC_PRINT_H_

