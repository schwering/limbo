// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Symbols are the non-logical symbols of the language: variables, standard
// names, and function symbols, which are sorted. Symbols are immutable.
//
// Sorts can be assumed to be small integers, which makes them suitable to be
// used as Key in IntMaps. Sorts are immutable.
//
// Terms can be built from symbols as usual. Terms are immutable.
//
// The implementation aims to keep Terms as lightweight as possible to
// facilitate extremely fast copying and comparison. For that reason, Terms
// are interned and represented only with an integer id that determines the
// index of the full structure in a heap structure. Creating a Term a second
// time yields the same id and hence also the same index. Note that for
// the index does not uniquely identify every term (see below).
//
// This id as opposed to a memory address keeps the representation lightweight,
// makes hashing deterministic over multiple runs of the program, and allows
// us to store some often-used information in the bits of the id.
//
// The id is a 31 bit number. We do not use the 32nd bit of the underlying
// integer to help the friend class Literal, which therefore can squeeze two
// terms and the sign of the literal into a 64 bit integer.
//
// The left-most bits of the id are not used for indexing but for
// classification of the term. We store whether or not the term is a primitive
// term, a name, a variable, or any other term.
//
// The reason for storing whether a term a name is that it is an extremely
// often accessed property in the subsumption and complementarity tests of
// literals; the encoding in the id avoids indirect lookups in the full
// representation. The reason for storing also the primitive property in a bit
// is that therefore we can store primitive terms and names (and variables and
// all other terms) on individual heaps, which keeps the indices of primitive
// terms and names small, so that arrays can be used to implement maps from
// from primtivie terms or names, respectively, to some other type.
//
// Since we can assume that there the number of primitive terms is much larger
// than the number of names, and the number of names is much larger than the
// number of variables and non-primitive terms, this encoding is
// space-efficient.

#ifndef LIMBO_TERM_H_
#define LIMBO_TERM_H_

#include <cassert>

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <limbo/internal/hash.h>
#include <limbo/internal/intmap.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/maybe.h>

namespace limbo {

template<typename T>
struct Singleton {
  static std::unique_ptr<T> instance;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance;

class Symbol {
 public:
  using Id = internal::u32;
  using Arity = internal::u8;

  class Sort {
   public:
    using Id = internal::u8;

    static Sort Nonrigid(Id id) { assert(id > 0 && (id & kBitMaskRigid) == 0); return Sort(id & ~kBitMaskRigid); }
    static Sort Rigid(Id id)    { assert(id > 0 && (id & kBitMaskRigid) == 0); return Sort(id | kBitMaskRigid); }

    Sort() = default;
    explicit Sort(Id id) : id_(id) {}
    explicit operator Id() const { return id_; }
    explicit operator int() const { return id_; }

    bool operator==(const Sort s) const { return id_ == s.id_; }
    bool operator!=(const Sort s) const { return id_ != s.id_; }

    internal::hash32_t hash() const { return internal::jenkins_hash(id_); }

    bool null() const { return id_ == 0; }

    bool rigid() const { return (id_ & kBitMaskRigid) != 0; }

    Id id() const { return id_; }
    int index() const { return id_ & ~kBitMaskRigid; }

   private:
    static constexpr Id kBitMaskRigid = 1 << (sizeof(Id) * 8 - 1);

    Id id_;
  };

  class Factory : private Singleton<Factory> {
   public:
    static Factory* Instance() {
      if (instance == nullptr) {
        instance = std::unique_ptr<Factory>(new Factory());
      }
      return instance.get();
    }

    static void Reset() { instance = nullptr; }

    static Symbol CreateName(Id index, Sort sort) {
      assert((index & kBitMaskMeta) == 0);
      return Symbol(index | kBitsName, sort, 0);
    }

    static Symbol CreateVariable(Id index, Sort sort) {
      assert((index & kBitMaskMeta) == 0);
      return Symbol(index | kBitsVariable, sort, 0);
    }

    static Symbol CreateFunction(Id index, Sort sort, Arity arity) {
      assert((index & kBitMaskMeta) == 0);
      assert(arity > 0 || !sort.rigid());
      return Symbol(index | kBitsFunction, sort, arity);
    }

    Sort CreateNonrigidSort() { return Sort::Nonrigid(++last_sort_); }
    Sort CreateRigidSort()    { return Sort::Rigid(++last_sort_); }

    Symbol CreateName(Sort sort)                  { return CreateName(last_name_++, sort); }
    Symbol CreateVariable(Sort sort)              { return CreateVariable(last_variable_++, sort); }
    Symbol CreateFunction(Sort sort, Arity arity) { return CreateFunction(last_function_++, sort, arity); }

   private:
    Factory() = default;
    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;
    Factory(Factory&&) = delete;
    Factory& operator=(Factory&&) = delete;

    Sort::Id last_sort_ = 0;
    Id last_function_ = 0;
    Id last_name_ = 0;
    Id last_variable_ = 0;
  };

  Symbol() = default;

  bool operator==(Symbol s) const {
    assert(id_ != s.id_ || (sort_ == s.sort_ && arity_ == s.arity_));
    return id_ == s.id_;
  }
  bool operator!=(Symbol s) const { return !(*this == s); }

  internal::hash32_t hash() const { return internal::jenkins_hash(id_); }

  bool name()     const { return (id_ & kBitMaskMeta) == kBitsName; }
  bool variable() const { return (id_ & kBitMaskMeta) == kBitsVariable; }
  bool function() const { return (id_ & kBitMaskMeta) == kBitsFunction; }

  bool null() const { return id_ == 0; }

  Sort sort() const { return sort_; }
  Arity arity() const { return arity_; }

  Id id() const { return id_; }

  int index() const { assert(!null()); return id_ & ~kBitMaskMeta; }

 private:
  static constexpr Id kFirstBitUnused = sizeof(Id)*8 - 1;
  static constexpr Id kFirstBitMeta   = sizeof(Id)*8 - 3;

  static constexpr Id kBitMaskUnused = 1 << kFirstBitUnused;
  static constexpr Id kBitMaskMeta   = 3 << kFirstBitMeta;

  static constexpr Id kBitsFunction = 1 << kFirstBitMeta;
  static constexpr Id kBitsName     = 2 << kFirstBitMeta;
  static constexpr Id kBitsVariable = 3 << kFirstBitMeta;

  Symbol(Id id, Sort sort, Arity arity) : id_(id), sort_(sort), arity_(arity) {
    assert(sort.id() >= 0);
    assert(arity >= 0);
    assert(!function() || !variable() || arity == 0);
  }

  Id id_ = 0;
  Sort sort_ = Sort(0);
  Arity arity_ = 0;
};

class Term {
 public:
  class Factory;
  struct Substitution;
  using Vector = std::vector<Term>;  // using Vector within Term will be legal in C++17, but seems to be illegal before
  using UnificationConfiguration = internal::i8;
  template<typename T>
  using Maybe = internal::Maybe<T>;

  static constexpr UnificationConfiguration kUnifyLeft     = (1 << 0);
  static constexpr UnificationConfiguration kUnifyRight    = (1 << 1);
  static constexpr UnificationConfiguration kOccursCheck   = (1 << 2);
  static constexpr UnificationConfiguration kUnifyTwoWay   = kUnifyLeft | kUnifyRight;
  static constexpr UnificationConfiguration kDefaultConfig = kUnifyTwoWay;

  Term() = default;

  bool operator==(Term t) const { return id_ == t.id_; }
  bool operator!=(Term t) const { return id_ != t.id_; }
  bool operator<=(Term t) const { return id_ <= t.id_; }
  bool operator>=(Term t) const { return id_ >= t.id_; }
  bool operator<(Term t)  const { return id_ < t.id_; }
  bool operator>(Term t)  const { return id_ > t.id_; }

  internal::hash32_t hash() const { return internal::jenkins_hash(id_); }

  inline Symbol symbol()      const;
  inline const Vector& args() const;

  Symbol::Sort sort()   const { return symbol().sort(); }
  Symbol::Arity arity() const { return symbol().arity(); }
  Term arg(int i)       const { return args()[i]; }

  bool null()            const { return id_ == 0; }
  bool name()            const { return (id_ & kBitMaskMeta) == kBitsName; }
  bool variable()        const { return (id_ & kBitMaskMeta) == kBitsVariable; }
  bool function()        const { return !name() && !variable(); }
  bool primitive()       const { return (id_ & kBitMaskMeta) == kBitsPrimitive; }
  bool quasi_name()      const { return !function() || (sort().rigid() && no_arg<&Term::function>()); }
  bool quasi_primitive() const { return function() && !sort().rigid() && all_args<&Term::quasi_name>(); }
  bool ground()          const { return primitive() || name() || (function() && all_args<&Term::ground>()); }

  bool Mentions(Term t) const {
    return *this == t || std::any_of(args().begin(), args().end(), [t](Term tt) { return tt.Mentions(t); });
  }

  template<typename UnaryFunction>
  Term Substitute(UnaryFunction theta, Factory* tf) const;

  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static bool Unify(Term l, Term r, Substitution* sub);
  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static Maybe<Substitution> Unify(Term l, Term r);

  static bool Isomorphic(Term l, Term r, Substitution* sub);
  static Maybe<Substitution> Isomorphic(Term l, Term r);

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const;

  int index() const { assert(!null()); return id_ & ~kBitMaskMeta; }

 private:
  friend class Literal;

  using Id = internal::u32;
  struct Data;

  static constexpr Id kFirstBitUnused = sizeof(Id)*8 - 1;
  static constexpr Id kFirstBitMeta   = sizeof(Id)*8 - 3;

  static constexpr Id kBitMaskUnused = 1 << kFirstBitUnused;
  static constexpr Id kBitMaskMeta   = 3 << kFirstBitMeta;

  static constexpr Id kBitsPrimitive = 1 << kFirstBitMeta;
  static constexpr Id kBitsName      = 2 << kFirstBitMeta;
  static constexpr Id kBitsVariable  = 3 << kFirstBitMeta;
  static constexpr Id kBitsOther     = 0 << kFirstBitMeta;

  explicit Term(Id id) : id_(id) {}

  inline const Data* data() const;

  Id id() const { return id_; }

  template<bool (Term::*Prop)() const>
  inline bool all_args() const {
    for (const Term t : args()) {
      if (!(t.*Prop)()) {
        return false;
      }
    }
    return true;
  }

  template<bool (Term::*Prop)() const>
  inline bool no_arg() const {
    for (const Term t : args()) {
      if ((t.*Prop)()) {
        return false;
      }
    }
    return true;
  }

  Id id_;
};

struct Term::Data {
  Data(Symbol symbol, const Vector& args) : symbol(symbol), args(args) {}

  bool operator==(const Data& d) const { return symbol == d.symbol && args == d.args; }
  bool operator!=(const Data& s) const { return !(*this == s); }

  internal::hash32_t hash() const {
    internal::hash32_t h = symbol.hash();
    for (const limbo::Term t : args) {
      h ^= t.hash();
    }
    return h;
  }

  Symbol symbol;
  Vector args;
};

class Term::Factory : private Singleton<Factory> {
 public:
  static Factory* Instance() {
    if (instance == nullptr) {
      instance = std::unique_ptr<Factory>(new Factory());
    }
    return instance.get();
  }

  static void Reset() { instance = nullptr; }

  ~Factory() {
    for (Data* data : heap_primitive_) {
      delete data;
    }
    for (Data* data : heap_name_) {
      delete data;
    }
    for (Data* data : heap_variable_) {
      delete data;
    }
    for (Data* data : heap_other_) {
      delete data;
    }
  }

  Term CreateTerm(Symbol symbol) { return CreateTerm(symbol, {}); }

  Term CreateTerm(Symbol symbol, const Vector& args) {
    assert(!symbol.null() && std::all_of(args.begin(), args.end(), [](const Term t) { return !t.null(); }));
    assert(symbol.arity() == static_cast<Symbol::Arity>(args.size()));
    Data* d = new Data(symbol, args);
    DataPtrSet* s = &memory_[symbol.sort()];
    auto it = s->find(d);
    if (it == s->end()) {
      auto all_args = [&args](auto p) {
        return std::all_of(args.begin(), args.end(), [&p](const Term t) { return p(t); });
      };
      const Symbol::Sort sort = symbol.sort();
      Id id;
      if (!sort.rigid() && symbol.function() && all_args([](const Term t) { return t.name(); })) {
        id = static_cast<Id>(heap_primitive_.size());
        assert((id & kBitMaskMeta) == 0);
        heap_primitive_.push_back(d);
        id |= kBitsPrimitive;
      } else if (symbol.name() || (sort.rigid() && symbol.function() &&
                                   all_args([](const Term t) { return t.name() && !t.function(); }))) {
        id = static_cast<Id>(heap_name_.size());
        assert((id & kBitMaskMeta) == 0);
        heap_name_.push_back(d);
        id |= kBitsName;
      } else if (symbol.variable()) {
        id = static_cast<Id>(heap_variable_.size());
        assert((id & kBitMaskMeta) == 0);
        heap_variable_.push_back(d);
        id |= kBitsVariable;
      } else {
        id = static_cast<Id>(heap_other_.size());
        assert((id & kBitMaskMeta) == 0);
        heap_other_.push_back(d);
        id |= kBitsOther;
      }
      s->insert(std::make_pair(d, id));
      const Term t(id);
      assert(!t.null());
      return t;
    } else {
      const Id id = it->second;
      delete d;
      return Term(id);
    }
  }

  const Data* get_data(Term t) const {
    assert(!t.null());
    const Id id = t.id();
    const int index = t.index();
    switch (id & kBitMaskMeta) {
      case kBitsPrimitive: return heap_primitive_[index];
      case kBitsName:      return heap_name_[index];
      case kBitsVariable:  return heap_variable_[index];
      case kBitsOther:     return heap_other_[index];
      default:             assert(false); return nullptr;
    }
  }

 private:
  struct DataPtrHash { internal::hash32_t operator()(const Term::Data* d) const { return d->hash(); } };
  struct DataPtrEquals { bool operator()(const Term::Data* a, const Term::Data* b) const { return *a == *b; } };

  // heap_other_ indices should start at 1 to distinguish from null term because kBitsOther == 0.
  Factory() { heap_other_.push_back(nullptr); }
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;
  Factory(Factory&&) = delete;
  Factory& operator=(Factory&&) = delete;

  using DataPtrSet = std::unordered_map<Data*, Id, DataPtrHash, DataPtrEquals>;
  internal::IntMap<Symbol::Sort, DataPtrSet> memory_;
  std::vector<Data*> heap_primitive_;
  std::vector<Data*> heap_name_;
  std::vector<Data*> heap_variable_;
  std::vector<Data*> heap_other_;
};

struct Term::Substitution {
  Substitution() = default;
  Substitution(Term old, Term sub) { Add(old, sub); }

  bool Add(Term old, Term sub) {
    Maybe<Term> bef = operator()(old);
    if (!bef) {
      subs_.push_back(std::make_pair(old, sub));
      return true;
    } else {
      return bef.val == sub;
    }
  }

  Maybe<Term> operator()(const Term t) const {
    for (auto p : subs_) {
      if (p.first == t) {
        return internal::Just(p.second);
      }
    }
    return internal::Nothing;
  }

  std::vector<std::pair<Term, Term>> subs_;
};

inline Symbol Term::symbol()            const { return data()->symbol; }
inline const Term::Vector& Term::args() const { return data()->args; }
inline const Term::Data* Term::data()   const { return Factory::Instance()->get_data(*this); }

template<typename UnaryFunction>
Term Term::Substitute(UnaryFunction theta, Factory* tf) const {
  Maybe<Term> t = theta(*this);
  if (t) {
    return t.val;
  } else if (arity() > 0) {
    Vector args;
    args.reserve(data()->args.size());
    for (Term arg : data()->args) {
      args.push_back(arg.Substitute(theta, tf));
    }
    if (args != data()->args) {
      return tf->CreateTerm(data()->symbol, args);
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
  Maybe<Term> u;
  l = (config & kUnifyLeft) != 0 && (u = (*sub)(l)) ? u.val : l;
  r = (config & kUnifyRight) != 0 && (u = (*sub)(r)) ? u.val : r;
  if (l.sort() != r.sort()) {
    return false;
  }
  if (l.symbol() == r.symbol()) {
    for (int i = 0; i < l.arity(); ++i) {
      if (!Unify<config>(l.arg(i), r.arg(i), sub)) {
        return false;
      }
    }
    return true;
  } else if (l.variable() && (config & kUnifyLeft) != 0 && sub->Add(l, r)) {
    return (config & kOccursCheck) == 0 || !r.Mentions(l);
  } else if (r.variable() && (config & kUnifyRight) != 0 && sub->Add(r, l)) {
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
  if (l.function() && r.function() && !l.name() && !r.name() && l.symbol() == r.symbol()) {
    for (Symbol::Arity i = 0; i < l.arity(); ++i) {
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

}  // namespace limbo


namespace std {

template<>
struct hash<limbo::Symbol::Sort> {
  limbo::internal::hash32_t operator()(const limbo::Symbol::Sort s) const { return s.hash(); }
};

template<>
struct equal_to<limbo::Symbol::Sort> {
  bool operator()(const limbo::Symbol::Sort a, const limbo::Symbol::Sort b) const { return a == b; }
};

template<>
struct hash<limbo::Symbol> {
  limbo::internal::hash32_t operator()(const limbo::Symbol s) const { return s.hash(); }
};

template<>
struct equal_to<limbo::Symbol> {
  bool operator()(const limbo::Symbol a, const limbo::Symbol b) const { return a == b; }
};

template<>
struct hash<limbo::Term> {
  limbo::internal::hash32_t operator()(const limbo::Term t) const { return t.hash(); }
};

template<>
struct equal_to<limbo::Term> {
  bool operator()(const limbo::Term a, const limbo::Term b) const { return a == b; }
};

}  // namespace std

#endif  // LIMBO_TERM_H_

