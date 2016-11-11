// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Symbols are the non-logical symbols of the language: variables, standard
// names, and function symbols, which are sorted. Symbols are immutable.
//
// Sorts can be assumed to be small integers, which makes them suitable to be
// used as Key in IntMaps. Sorts are immutable.
//
// Terms can be built from symbols as usual. Terms are immutable.
//
// The implementation aims to keep terms as lightweight as possible to
// facilitate extremely fast copying and comparison. Internally, a term is
// represented by a memory address where its structure is stored. Creating a
// second term of the same structure yields the same memory address.

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include "./compar.h"
#include "./maybe.h"

namespace lela {

class Symbol {
 public:
  typedef int Id;
  typedef int8_t Sort;
  typedef int8_t Arity;
  struct Comparator;

  class Factory {
   public:
    static Symbol CreateName(Id id, Sort sort) {
      assert(id > 0);
      return Symbol(id, sort, 0);
    }

    static Symbol CreateVariable(Id id, Sort sort) {
      assert(id > 0);
      id = -1 * (2 * id);
      return Symbol(id, sort, 0);
    }

    static Symbol CreateFunction(Id id, Sort sort, Arity arity) {
      assert(id > 0);
      id = -1 * (2 * id + 1);
      return Symbol(id, sort, arity);
    }

    Factory() = default;
    Factory(const Factory&) = delete;
    Factory(Factory&&) = delete;
    Factory& operator=(const Factory&) = delete;

    Sort   CreateSort()                           { return last_sort_++; }
    Symbol CreateName(Sort sort)                  { return CreateName(++last_name_, sort); }
    Symbol CreateVariable(Sort sort)              { return CreateVariable(++last_variable_, sort); }
    Symbol CreateFunction(Sort sort, Arity arity) { return CreateFunction(++last_function_, sort, arity); }

   private:
    Sort last_sort_ = 0;
    Id last_function_ = 0;
    Id last_name_ = 0;
    Id last_variable_ = 0;
  };

  bool name()     const { return id_ > 0; }
  bool variable() const { return id_ < 0 && ((-id_) % 2) == 0; }
  bool function() const { return id_ < 0 && ((-id_) % 2) != 0; }

  Id id() const {
    assert(function() || name() || variable());
    if (name())           return id_;
    else if (variable())  return (-1 * id_) / 2;
    else if (function())  return (-1 * id_ - 1) / 2;
    else                  return 0;
  }

  Sort sort() const { return sort_; }

  Arity arity() const { return arity_; }

 private:
  Symbol(Id id, Sort sort, Arity arity) : id_(id), sort_(sort), arity_(arity) {
    assert(sort >= 0);
    assert(arity >= 0);
    assert(!function() || !variable() || arity == 0);
  }

  const Id id_;
  const Sort sort_;
  const Arity arity_;
};

struct Symbol::Comparator {
  typedef Symbol value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a.id_, a.sort_, a.arity_,
                b.id_, b.sort_, b.arity_);
  }

 private:
  LexicographicComparator<LessComparator<Id>,
                          LessComparator<Sort>,
                          LessComparator<int>> comp;
};

class Term {
 public:
  struct Comparator;
  typedef std::vector<Term> Vector;  // using Vector within Term will be legal in C++17, but seems to be illegal before
  typedef std::set<Term, Comparator> Set;

  class Factory;

  Term() = default;

  bool operator==(Term t) const { return data_ == t.data_; }
  bool operator!=(Term t) const { return data_ != t.data_; }
  bool operator<=(Term t) const { return data_ <= t.data_; }
  bool operator>=(Term t) const { return data_ >= t.data_; }
  bool operator<(Term t)  const { return data_ < t.data_; }
  bool operator>(Term t)  const { return data_ > t.data_; }

  template<typename UnaryFunction>
  Term Substitute(UnaryFunction theta, Factory* tf) const;

  Symbol symbol()      const { return data_->symbol_; }
  const Vector& args() const { return data_->args_; }

  bool name()           const { return data_->symbol_.name(); }
  bool variable()       const { return data_->symbol_.variable(); }
  bool function()       const { return data_->symbol_.function(); }
  Symbol::Arity arity() const { return data_->symbol_.arity(); }

  bool null()           const { return data_ == nullptr; }
  bool ground()         const { return name() || (function() && all_args([](Term t) { return t.ground(); })); }
  bool primitive()      const { return function() && all_args([](Term t) { return t.name(); }); }
  bool quasiprimitive() const { return function() && all_args([](Term t) { return t.name() || t.variable(); }); }

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const;

  uint64_t hash() const {
    // 64bit FNV-1a hash
    const uint64_t magic_prime = 0x00000100000001b3;
    const uint64_t b = reinterpret_cast<const uint64_t>(data_);
    assert(sizeof(data_) == sizeof(b));
    return
        ((((((((((((((((
          0xcbf29ce484222325
          ^ ((b >>  0) & 0xFF)) * magic_prime)
          ^ ((b >>  8) & 0xFF)) * magic_prime)
          ^ ((b >> 16) & 0xFF)) * magic_prime)
          ^ ((b >> 24) & 0xFF)) * magic_prime)
          ^ ((b >> 32) & 0xFF)) * magic_prime)
          ^ ((b >> 40) & 0xFF)) * magic_prime)
          ^ ((b >> 48) & 0xFF)) * magic_prime)
          ^ ((b >> 56) & 0xFF)) * magic_prime);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Term, hash);
#endif

  struct Data {
    struct DeepComparator;
    Data(Symbol symbol, const Vector& args) : symbol_(symbol), args_(args) {}

    Symbol symbol_;
    Vector args_;
  };

  explicit Term(Data* data) : data_(data) {}

  template<typename UnaryPredicate>
  bool all_args(UnaryPredicate p) const { return std::all_of(data_->args_.begin(), data_->args_.end(), p); }

  const Data* data_;
};

struct Term::Comparator {
  typedef Term value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a.data_, b.data_);
  }

 private:
  LessComparator<const Data*> comp;
};

struct Term::Data::DeepComparator {
  typedef const Term::Data* value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a->symbol_, a->args_,
                b->symbol_, b->args_);
  }

 private:
  LexicographicComparator<Symbol::Comparator,
                          LexicographicContainerComparator<Vector, Term::Comparator>> comp;
};

class Term::Factory {
 public:
  Factory() = default;
  Factory(const Factory&) = delete;
  Factory(Factory&&) = delete;
  Factory& operator=(const Factory&) = delete;

  Term CreateTerm(Symbol symbol) {
    return CreateTerm(symbol, {});
  }

  Term CreateTerm(Symbol symbol, const Vector& args) {
    assert(symbol.arity() == static_cast<Symbol::Arity>(args.size()));
    const size_t mem_index = symbol.sort();
    if (mem_index >= memory_.size()) {
      memory_.resize(mem_index + 1);
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

 private:
  std::vector<std::set<Data*, Data::DeepComparator>> memory_;
};

template<typename UnaryFunction>
Term Term::Substitute(UnaryFunction theta, Factory* tf) const {
  Maybe<Term> t = theta(*this);
  if (t) {
    return t.val;
  } else if (arity() > 0) {
    Vector args;
    args.reserve(data_->args_.size());
    for (Term arg : data_->args_) {
      args.push_back(arg.Substitute(theta, tf));
    }
    if (args != data_->args_) {
      return tf->CreateTerm(data_->symbol_, args);
    } else {
      return *this;
    }
  } else {
    return *this;
  }
}

template<typename UnaryFunction>
void Term::Traverse(UnaryFunction f) const {
  if (f(*this)) {
    for (Term arg : args()) {
      arg.Traverse(f);
    }
  }
}

}  // namespace lela

#endif  // SRC_TERM_H_

