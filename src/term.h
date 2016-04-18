// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Symbols are the non-logical symbols of the language: variables, standard
// names, and function symbols, which are sorted. Symbols are immutable.
//
// Terms can be built from these symbols as usual. Symbols are immutable.
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

namespace lela {

class Symbol {
 public:
  typedef int Id;
  typedef int8_t Sort;
  typedef int8_t Arity;
  struct Comparator;

  static Symbol CreateFunction(Id id, Sort sort, Arity arity) {
    assert(id > 0);
    id = -1 * (2 * id + 1);
    return Symbol(id, sort, arity);
  }

  static Symbol CreateName(Id id, Sort sort) {
    assert(id > 0);
    return Symbol(id, sort, 0);
  }

  static Symbol CreateVariable(Id id, Sort sort) {
    assert(id > 0);
    id = -1 * (2 * id);
    return Symbol(id, sort, 0);
  }

  bool function() const { return id_ < 0 && ((-id_) % 2) != 0; }
  bool name() const { return id_ > 0; }
  bool variable() const { return id_ < 0 && ((-id_) % 2) == 0; }

  Id id() const {
    assert(function() || name() || variable());
    if (function())       return (-1 * id_ - 1) / 2;
    else if (name())      return id_;
    else if (variable())  return (-1 * id_) / 2;
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
  typedef std::vector<Term> Vector;
  typedef std::map<Term, Term> Substitution;
  typedef std::set<Term, Comparator> Set;

  Term() = default;
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  static Term Create(Symbol symbol);
  static Term Create(Symbol symbol, const Vector& args);

  bool operator==(Term t) const { return data_ == t.data_; }
  bool operator!=(Term t) const { return data_ != t.data_; }
  bool operator<=(Term t) const { return data_ <= t.data_; }
  bool operator>=(Term t) const { return data_ >= t.data_; }
  bool operator<(Term t) const { return data_ < t.data_; }
  bool operator>(Term t) const { return data_ > t.data_; }

  Term Substitute(Term pre, Term post) const {
    if (*this == pre) {
      return post;
    }
    if (arity() > 0) {
      Vector args;
      args.reserve(data_->args_.size());
      for (Term arg : data_->args_) {
        args.push_back(arg.Substitute(pre, post));
      }
      if (args != data_->args_) {
        return Create(data_->symbol_, args);
      }
    }
    return *this;
  }

  Term Ground(const Substitution& theta) const {
    assert(std::all_of(theta.begin(), theta.end(), [](const std::pair<Term, Term>& p) { return p.first.variable() && p.second.name(); }));
    if (variable()) {
      const auto it = theta.find(*this);
      if (it != theta.end()) {
        return it->second;
      } else {
        return *this;
      }
    } else if (arity() > 0) {
      Vector args;
      args.reserve(data_->args_.size());
      for (Term arg : data_->args_) {
        args.push_back(arg.Ground(theta));
      }
      if (args != data_->args_) {
        return Create(data_->symbol_, args);
      } else {
        return *this;
      }
    } else {
      return *this;
    }
  }

  Symbol symbol() const { return data_->symbol_; }
  const Vector& args() const { return data_->args_; }

  bool function() const { return data_->symbol_.function(); }
  bool name() const { return data_->symbol_.name(); }
  bool variable() const { return data_->symbol_.variable(); }
  Symbol::Arity arity() const { return data_->symbol_.arity(); }

  bool null() const { return !data_; }
  bool ground() const {
    return name() ||
        (function() && std::all_of(data_->args_.begin(), data_->args_.end(),
                                   [](Term t) { return t.ground(); }));
  }
  bool primitive() const {
    return function() && std::all_of(data_->args_.begin(), data_->args_.end(),
                                     [](Term t) { return t.name(); });
  }
  bool quasiprimitive() const {
    return function() && std::all_of(data_->args_.begin(), data_->args_.end(),
                                     [](Term t) { return t.name() || t.variable(); });
  }

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
  struct Data {
    struct DeepComparator;
    Data(Symbol symbol, const Vector& args) : symbol_(symbol), args_(args) {}

    Symbol symbol_;
    Vector args_;
  };

  explicit Term(Data* data) : data_(data) {}

  static std::vector<std::set<Data*, Data::DeepComparator>> memory_;

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

