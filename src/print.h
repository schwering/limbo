// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_PRINT_H_
#define SRC_PRINT_H_

#include <ostream>
#include "clause.h"
#include "literal.h"
#include "maybe.h"
#include "term.h"

#define MARK (std::cout << __FILE__ << ":" << __LINE__ << std::endl)             

namespace lela {

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
      os << '(';
      bool first = true;
      for (const Term arg : t.args()) {
        if (!first) {
          os << ',';
        }
        first = false;
        os << arg;
      }
      os << ')';
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

std::ostream& operator<<(std::ostream& os, const Clause c) {
  os << '[';
  bool first = true;
  for (const Literal a : c) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << a;
  }
  os << ']';
  return os;
}

}  // namespace lela

#endif  // SRC_PRINT_H_

