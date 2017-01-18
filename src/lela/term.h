// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering
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

#ifndef LELA_TERM_H_
#define LELA_TERM_H_

#include <cassert>

#include <algorithm>
#include <functional>
#include <unordered_set>
#include <vector>

#include <lela/internal/compar.h>
#include <lela/internal/hash.h>
#include <lela/internal/intmap.h>
#include <lela/internal/maybe.h>

namespace lela {

class Symbol {
 public:
  typedef int Id;
  typedef std::int8_t Sort;
  typedef std::int8_t Arity;
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
    Factory& operator=(const Factory&) = delete;
    Factory(Factory&&) = default;
    Factory& operator=(Factory&&) = default;

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

  bool operator==(Symbol s) const {
    assert(id_ != s.id_ || (sort_ == s.sort_ && arity_ == s.arity_));
    return id_ == s.id_;
  }
  bool operator!=(Symbol s) const { return !(*this == s); }

  internal::hash_t hash() const { return internal::fnv1a_hash(id_); }

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
  internal::LexicographicComparator<internal::LessComparator<Id>,
                                    internal::LessComparator<Sort>,
                                    internal::LessComparator<int>> comp;
};

class Term {
 public:
  struct Comparator;
  typedef std::vector<Term> Vector;  // using Vector within Term will be legal in C++17, but seems to be illegal before

  class Factory;
  struct SingleSubstitution;

  Term() = default;

  bool operator==(Term t) const { return data_ == t.data_; }
  bool operator!=(Term t) const { return data_ != t.data_; }
  bool operator<=(Term t) const { return data_ <= t.data_; }
  bool operator>=(Term t) const { return data_ >= t.data_; }
  bool operator<(Term t)  const { return data_ < t.data_; }
  bool operator>(Term t)  const { return data_ > t.data_; }

  internal::hash_t hash() const { return internal::fnv1a_hash(reinterpret_cast<std::uintptr_t>(data_)); }

  template<typename UnaryFunction>
  Term Substitute(UnaryFunction theta, Factory* tf) const;

  Symbol symbol()      const { return data_->symbol_; }
  const Vector& args() const { return data_->args_; }

  Symbol::Sort sort()   const { return data_->symbol_.sort(); }
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

 private:
  struct Data {
    struct DeepComparator;
    Data(Symbol symbol, const Vector& args) : symbol_(symbol), args_(args) {}

    bool operator==(const Data& d) const { return symbol_ == d.symbol_ && args_ == d.args_; }
    bool operator!=(const Data& s) const { return !(*this == s); }

    internal::hash_t hash() const {
      internal::hash_t h = symbol_.hash();
      for (const lela::Term t : args_) {
        h ^= t.hash();
      }
      return h;
    }

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
  internal::LessComparator<const Data*> comp;
};

struct Term::Data::DeepComparator {
  typedef const Term::Data* value_type;

  bool operator()(value_type a, value_type b) const {
    return comp(a->symbol_, a->args_,
                b->symbol_, b->args_);
  }

 private:
  internal::LexicographicComparator<Symbol::Comparator,
                                    internal::LexicographicContainerComparator<Vector, Term::Comparator>> comp;
};

class Term::Factory {
 public:
  Factory() = default;
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;
  Factory(Factory&&) = default;
  Factory& operator=(Factory&&) = default;

  ~Factory() {
    for (const DataPtrSet& set : memory_.values()) {
      for (Data* data : set) {
        delete data;
      }
    }
  }

  Term CreateTerm(Symbol symbol) {
    return CreateTerm(symbol, {});
  }

  Term CreateTerm(Symbol symbol, const Vector& args) {
    assert(symbol.arity() == static_cast<Symbol::Arity>(args.size()));
    Data* d = new Data(symbol, args);
    DataPtrSet* s = &memory_[symbol.sort()];
    auto p = s->insert(d);
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
  struct DataPtrHash   { std::size_t operator()(const Term::Data* d) const { return d->hash(); } };
  struct DataPtrEquals { std::size_t operator()(const Term::Data* a, const Term::Data* b) const { return *a == *b; } };

  typedef std::unordered_set<Data*, DataPtrHash, DataPtrEquals> DataPtrSet;
  internal::IntMap<Symbol::Sort, DataPtrSet> memory_;
};

struct Term::SingleSubstitution {
  SingleSubstitution(Term old, Term rev) : old_(old), rev_(rev) {}

  internal::Maybe<Term> operator()(const Term t) const {
    return t == old_ ? internal::Just(rev_) : internal::Nothing;
  }

 private:
  const Term old_;
  const Term rev_;
};

template<typename UnaryFunction>
Term Term::Substitute(UnaryFunction theta, Factory* tf) const {
  internal::Maybe<Term> t = theta(*this);
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
  if (f(*this) && arity() > 0) {
    for (Term arg : args()) {
      arg.Traverse(f);
    }
  }
}

}  // namespace lela


namespace std {

template<>
struct hash<lela::Symbol> {
  std::size_t operator()(const lela::Symbol s) const { return s.hash(); }
};

template<>
struct equal_to<lela::Symbol> {
  bool operator()(const lela::Symbol a, const lela::Symbol b) const { return a == b; }
};

template<>
struct less<lela::Symbol> {
  bool operator()(const lela::Symbol a, const lela::Symbol b) const { return comp_(a, b); }
 private:
  lela::Symbol::Comparator comp_;
};

template<>
struct hash<lela::Term> {
  std::size_t operator()(const lela::Term t) const { return t.hash(); }
};

template<>
struct equal_to<lela::Term> {
  bool operator()(const lela::Term a, const lela::Term b) const { return a == b; }
};

template<>
struct less<lela::Term> {
  bool operator()(const lela::Term a, const lela::Term b) const { return comp_(a, b); }
 private:
  lela::Term::Comparator comp_;
};

}  // namespace std

#endif  // LELA_TERM_H_

