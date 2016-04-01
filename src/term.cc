// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 Christoph Schwering

#include "./term.h"

namespace lela {

std::vector<std::set<Term::Data*, Term::Data::PtrComparator>> Term::memory_;

Term Term::Create(Symbol symbol) {
  return Create(symbol, {});
}

Term Term::Create(Symbol symbol, const Vector& args) {
  const size_t mem_index = symbol.sort();
  if (mem_index >= memory_.size()) {
    memory_.resize(mem_index);
  }
  Data* d = new Data(symbol, args);
  auto p = memory_[mem_index].insert(d);
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

