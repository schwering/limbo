// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#include "./term.h"

namespace lela {

std::set<Term::Data*, Term::Data::PtrComparator> Term::memory_;

Term Term::Create(Symbol symbol) {
  return Create(symbol, {});
}

Term Term::Create(Symbol symbol, const Vector& args) {
  Data* d = new Data(symbol, args);
  auto p = memory_.insert(d);
  if (p.second) {
    assert(d == *p.first);
    return Term(d);
  } else {
    assert(d != *p.first);
    delete d;
    return Term(*p.first);
  }
}

}  // namespace lela

