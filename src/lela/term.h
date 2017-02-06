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
//
// Comparison of terms is based on their memory addresses, which makes it
// nondeterministic wrt different program executions.

#ifndef LELA_TERM_H_
#define LELA_TERM_H_

#include <cassert>

#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <lela/internal/compar.h>
#include <lela/internal/hash.h>
#include <lela/internal/intmap.h>
#include <lela/internal/maybe.h>

namespace lela {

template<typename T>
struct Singleton {
  static std::unique_ptr<T> instance_;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance_;

class Symbol {
 public:
  typedef std::int32_t Id;
  typedef std::int8_t Sort;
  typedef std::int8_t Arity;
  struct Comparator;

  class Factory : private Singleton<Factory> {
   public:
    static Factory* Instance() {
      if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Factory>(new Factory());
      }
      return instance_.get();
    }

    static void Reset() { instance_ = nullptr; }

    static Symbol CreateName(Id id, Sort sort) {
      assert(id > 0);
      return Symbol((id << 2) | 0, sort, 0);
    }

    static Symbol CreateVariable(Id id, Sort sort) {
      assert(id > 0);
      return Symbol((id << 2) | 1, sort, 0);
    }

    static Symbol CreateFunction(Id id, Sort sort, Arity arity) {
      assert(id > 0);
      return Symbol((id << 2) | 2, sort, arity);
    }

    Sort   CreateSort()                           { return last_sort_++; }
    Symbol CreateName(Sort sort)                  { return CreateName(++last_name_, sort); }
    Symbol CreateVariable(Sort sort)              { return CreateVariable(++last_variable_, sort); }
    Symbol CreateFunction(Sort sort, Arity arity) { return CreateFunction(++last_function_, sort, arity); }

   private:
    Factory() = default;
    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;
    Factory(Factory&&) = delete;
    Factory& operator=(Factory&&) = delete;

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

  internal::hash32_t hash() const { return internal::jenkins_hash(id_); }

  bool name()     const { return (id_ & (0 | 1 | 2)) == 0; }
  bool variable() const { return (id_ & (0 | 1 | 2)) == 1; }
  bool function() const { return (id_ & (0 | 1 | 2)) == 2; }

  Id id() const { return id_ >> 2; }
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

class Term {
 public:
  class Factory;
  struct Substitution;
  typedef std::vector<Term> Vector;  // using Vector within Term will be legal in C++17, but seems to be illegal before
  typedef std::uint8_t UnificationConfiguration;

  static constexpr UnificationConfiguration kUnifyLeft = (1 << 0);
  static constexpr UnificationConfiguration kUnifyRight = (1 << 1);
  static constexpr UnificationConfiguration kUnifyVars = (1 << 2);
  static constexpr UnificationConfiguration kOccursCheck = (1 << 4);
  static constexpr UnificationConfiguration kUnifyTwoWay = kUnifyLeft | kUnifyRight;
  static constexpr UnificationConfiguration kDefaultConfig = kUnifyTwoWay | kUnifyVars;

  Term() = default;

  bool operator==(Term t) const { return index_ == t.index_; }
  bool operator!=(Term t) const { return index_ != t.index_; }
  bool operator<=(Term t) const { return index_ <= t.index_; }
  bool operator>=(Term t) const { return index_ >= t.index_; }
  bool operator<(Term t)  const { return index_ < t.index_; }
  bool operator>(Term t)  const { return index_ > t.index_; }

  internal::hash32_t hash() const { return internal::jenkins_hash(index_); }

  Symbol symbol()         const { return data()->symbol_; }
  Term arg(std::size_t i) const { return data()->args_[i]; }
  const Vector& args()    const { return data()->args_; }

  Symbol::Sort sort()   const { return data()->symbol_.sort(); }
  bool name()           const { return data()->symbol_.name(); }
  bool variable()       const { return data()->symbol_.variable(); }
  bool function()       const { return data()->symbol_.function(); }
  Symbol::Arity arity() const { return data()->symbol_.arity(); }

  bool null()           const { return index_ == 0; }
  bool ground()         const { return name() || (function() && all_args([](Term t) { return t.ground(); })); }
  bool primitive()      const { return function() && all_args([](Term t) { return t.name(); }); }
  bool quasiprimitive() const { return function() && all_args([](Term t) { return t.name() || t.variable(); }); }

  bool Mentions(Term t) const { return *this == t || any_arg([t](Term tt) { return t == tt; }); }

  template<typename UnaryFunction>
  Term Substitute(UnaryFunction theta, Factory* tf) const;

  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static bool Unify(Term l, Term r, Substitution* sub);
  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static internal::Maybe<Substitution> Unify(Term l, Term r);

  static bool Isomorphic(Term l, Term r, Substitution* sub);
  static internal::Maybe<Substitution> Isomorphic(Term l, Term r);

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const;

 private:
  friend class Literal;

  struct Data {
    Data(Symbol symbol, const Vector& args) : symbol_(symbol), args_(args) {}

    bool operator==(const Data& d) const { return symbol_ == d.symbol_ && args_ == d.args_; }
    bool operator!=(const Data& s) const { return !(*this == s); }

    Symbol symbol_;
    Vector args_;

    internal::hash32_t hash() const {
      internal::hash32_t h = symbol_.hash();
      for (const lela::Term t : args_) {
        h ^= t.hash();
      }
      return h;
    }
  };

  explicit Term(std::uint32_t index) : index_(index) {}

  inline const Data* data() const;

  std::uint32_t index() const { return index_; }

  template<typename UnaryPredicate>
  bool all_args(UnaryPredicate p) const { return std::all_of(data()->args_.begin(), data()->args_.end(), p); }

  template<typename UnaryPredicate>
  bool any_arg(UnaryPredicate p) const { return std::any_of(data()->args_.begin(), data()->args_.end(), p); }

  std::uint32_t index_;
};

class Term::Factory : private Singleton<Factory> {
 public:
  static Factory* Instance() {
    if (instance_ == nullptr) {
      instance_ = std::unique_ptr<Factory>(new Factory());
    }
    return instance_.get();
  }

  static void Reset() { instance_ = nullptr; }

  ~Factory() {
    for (Data* data : heap_) {
      delete data;
    }
  }

  Term CreateTerm(Symbol symbol) {
    return CreateTerm(symbol, {});
  }

  Term CreateTerm(Symbol symbol, const Vector& args) {
    assert(symbol.arity() == static_cast<Symbol::Arity>(args.size()));
    Data* d = new Data(symbol, args);
    DataPtrSet* s = &memory_[symbol.sort()];
    auto it = s->find(d);
    if (it == s->end()) {
      heap_.push_back(d);
      const std::uint32_t index = static_cast<std::uint32_t>(heap_.size());
      s->insert(std::make_pair(d, index));
      return Term(index);
    } else {
      const std::uint32_t index = it->second;
      delete d;
      return Term(index);
    }
  }

  const Data* get(std::uint32_t index) const { return heap_[index - 1]; }

 private:
  struct DataPtrHash   { internal::hash32_t operator()(const Term::Data* d) const { return d->hash(); } };
  struct DataPtrEquals { bool operator()(const Term::Data* a, const Term::Data* b) const { return *a == *b; } };

  Factory() = default;
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;
  Factory(Factory&&) = delete;
  Factory& operator=(Factory&&) = delete;

  typedef std::unordered_map<Data*, std::uint32_t, DataPtrHash, DataPtrEquals> DataPtrSet;
  internal::IntMap<Symbol::Sort, DataPtrSet> memory_;
  std::vector<Data*> heap_;
};

struct Term::Substitution {
  Substitution() = default;
  Substitution(Term old, Term sub) { Add(old, sub); }

  bool Add(Term old, Term sub) {
    internal::Maybe<Term> bef = operator()(old);
    if (!bef) {
      subs_.push_back(std::make_pair(old, sub));
      return true;
    } else {
      return bef.val == sub;
    }
  }

  internal::Maybe<Term> operator()(const Term t) const {
    for (auto p : subs_) {
      if (p.first == t) {
        return internal::Just(p.second);
      }
    }
    return internal::Nothing;
  }

  std::vector<std::pair<Term, Term>> subs_;
};

inline const Term::Data* Term::data() const { return Factory::Instance()->get(index_); }

template<typename UnaryFunction>
Term Term::Substitute(UnaryFunction theta, Factory* tf) const {
  internal::Maybe<Term> t = theta(*this);
  if (t) {
    return t.val;
  } else if (arity() > 0) {
    Vector args;
    args.reserve(data()->args_.size());
    for (Term arg : data()->args_) {
      args.push_back(arg.Substitute(theta, tf));
    }
    if (args != data()->args_) {
      return tf->CreateTerm(data()->symbol_, args);
    } else {
      return *this;
    }
  } else {
    return *this;
  }
}

template<Term::UnificationConfiguration config>
bool Term::Unify(Term l, Term r, Substitution* sub) {
  if (l == r) {
    return true;
  }
  internal::Maybe<Term> u;
  l = (u = (*sub)(l)) ? u.val : l;
  r = (u = (*sub)(r)) ? u.val : r;
  if (l.sort() != r.sort()) {
    return false;
  }
  if (l.symbol() == r.symbol()) {
    for (std::size_t i = 0; i < l.arity(); ++i) {
      if (!Unify<config>(l.arg(i), r.arg(i), sub)) {
        return false;
      }
    }
    return true;
  } else if (l.variable() && (config & kUnifyLeft) != 0 != 0 && sub->Add(l, r)) {
    return (config & kOccursCheck) == 0 || !r.Mentions(l);
  } else if (r.variable() && (config & kUnifyRight) != 0 != 0 && sub->Add(r, l)) {
    return (config & kOccursCheck) == 0 || !l.Mentions(r);
  } else {
    return false;
  }
}

template<Term::UnificationConfiguration config>
internal::Maybe<Term::Substitution> Term::Unify(Term l, Term r) {
  Substitution sub;
  return Unify<config>(l, r, &sub) ? internal::Just(sub) : internal::Nothing;
}

bool Term::Isomorphic(Term l, Term r, Substitution* sub) {
  internal::Maybe<Term> u;
  if (l.function() && r.function() && l.symbol() == r.symbol()) {
    for (std::size_t i = 0; i < l.arity(); ++i) {
      if (!Isomorphic(l.arg(i), r.arg(i), sub)) {
        return false;
      }
    }
    return true;
  } else if (l.variable() && r.variable() && l.sort() == r.sort() && sub->Add(l, r) && sub->Add(r, l)) {
    return true;
  } else if (l.name() && r.name() && l.sort() == r.sort() && sub->Add(l, r) && sub->Add(r, l)) {
    return true;
  } else {
    return false;
  }
}

internal::Maybe<Term::Substitution> Term::Isomorphic(Term l, Term r) {
  Substitution sub;
  return Isomorphic(l, r, &sub) ? internal::Just(sub) : internal::Nothing;
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
  lela::internal::hash32_t operator()(const lela::Symbol s) const { return s.hash(); }
};

template<>
struct equal_to<lela::Symbol> {
  bool operator()(const lela::Symbol a, const lela::Symbol b) const { return a == b; }
};

template<>
struct hash<lela::Term> {
  lela::internal::hash32_t operator()(const lela::Term t) const { return t.hash(); }
};

template<>
struct equal_to<lela::Term> {
  bool operator()(const lela::Term a, const lela::Term b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_TERM_H_

