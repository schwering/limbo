// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#include "./literal.h"

namespace lela {

std::ostream& operator<<(std::ostream& os, const Literal& l) {
  if (!l.sign()) {
    os << '~';
  }
  os << static_cast<const Atom&>(l);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Literal::Set& ls) {
  os << "{ ";
  for (auto it = ls.begin(); it != ls.end(); ++it) {
    if (it != ls.begin()) {
      os << ", ";
    }
    os << *it;
  }
  os << " }";
  return os;
}

}  // namespace lela

