// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Formulas are represented as linked lists of symbols. For lightweight
// copying and read-access, RFormula only works with pointers to the list.

#ifndef LIMBO_FORMULA_H_
#define LIMBO_FORMULA_H_

#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/lit.h>
#include <limbo/internal/dense.h>
#include <limbo/internal/hash.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/maybe.h>
#include <limbo/internal/singleton.h>

namespace limbo {

class Alphabet : private internal::Singleton<Alphabet> {
 public:
  using id_t = int;

  class IntRepresented {
   public:
    explicit IntRepresented() = default;
    explicit IntRepresented(id_t id) : id_(id) {}

    IntRepresented(const IntRepresented&) = default;
    IntRepresented& operator=(const IntRepresented&) = default;
    IntRepresented(IntRepresented&&) = default;
    IntRepresented& operator=(IntRepresented&&) = default;

    bool operator==(const IntRepresented& i) const { return id_ == i.id_; }
    bool operator!=(const IntRepresented& i) const { return !(*this == i); }

    explicit operator bool() const { return id_; }
    bool null() const { return id_ == 0; }

    id_t id() const { return id_; }

   private:
    id_t id_ = 0;
  };

  class Sort : public IntRepresented {
   public:
    using IntRepresented::IntRepresented;
    bool rigid() const { return instance().sort_rigid_[*this]; }
  };

  class FunSymbol : public IntRepresented {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return instance().fun_sort_[*this]; }
    int arity() const { return instance().fun_arity_[*this]; }
  };

  class NameSymbol : public IntRepresented {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return instance().name_sort_[*this]; }
    int arity() const { return instance().name_arity_[*this]; }
  };

  class VarSymbol : public IntRepresented {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return instance().var_sort_[*this]; }
  };

  template<typename T>
  struct ToId {
    int operator()(const T k) const { return k.id(); }
  };

  template<typename T>
  struct FromId {
    T operator()(const int i) const { return T::FromId(i); }
  };

  template<typename Key, typename Value>
  using DenseMap = internal::DenseMap<Key, Value, int, 1, ToId<Key>, FromId<Key>, internal::FastAdjustBoundCheck>;

  template<typename Key>
  using DenseSet = DenseMap<Key, bool>;

  struct Symbol {
    enum Tag {
      kFun = 1, kName, kVar, kStrippedFun, kStrippedName, kEquals, kNotEquals, kStrippedLit,
      kNot, kOr, kAnd, kExists, kForall, kKnow, kMaybe, kBelieve, kAction
    };

    using List = std::list<Symbol>;
    using Ref  = List::iterator;
    using CRef = List::const_iterator;

    static Symbol Fun(FunSymbol f)           { Symbol s; s.tag = kFun;          s.u.f = f;            return s; }
    static Symbol Name(NameSymbol n)         { Symbol s; s.tag = kName;         s.u.n = n;            return s; }
    static Symbol Var(VarSymbol x)           { Symbol s; s.tag = kVar;          s.u.x = x;            return s; }
    static Symbol StrippedFun(class Fun f)   { Symbol s; s.tag = kStrippedFun;  s.u.f_s = f;          return s; }
    static Symbol StrippedName(class Name n) { Symbol s; s.tag = kStrippedName; s.u.n_s = n;          return s; }
    static Symbol Equals()                   { Symbol s; s.tag = kEquals;                             return s; }
    static Symbol NotEquals()                { Symbol s; s.tag = kNotEquals;                          return s; }
    static Symbol Lit(Lit a)                 { Symbol s; s.tag = kStrippedLit;  s.u.a = a;            return s; }
    static Symbol Not()                      { Symbol s; s.tag = kNot;                                return s; }
    static Symbol Exists(VarSymbol x)        { Symbol s; s.tag = kExists;       s.u.x = x;            return s; }
    static Symbol Forall(VarSymbol x)        { Symbol s; s.tag = kForall;       s.u.x = x;            return s; }
    static Symbol Or(int k)                  { Symbol s; s.tag = kOr;           s.u.k = k;            return s; }
    static Symbol And(int k)                 { Symbol s; s.tag = kAnd;          s.u.k = k;            return s; }
    static Symbol Know(int k)                { Symbol s; s.tag = kKnow;         s.u.k = k;            return s; }
    static Symbol Maybe(int k)               { Symbol s; s.tag = kMaybe;        s.u.k = k;            return s; }
    static Symbol Believe(int k, int l)      { Symbol s; s.tag = kBelieve;      s.u.kl = {k,l};       return s; }
    static Symbol Action()                   { Symbol s; s.tag = kAction;                             return s; }

    Tag tag{};

    union {
      FunSymbol  f;    // kFun
      NameSymbol n;    // kName
      VarSymbol  x;    // kVar, kExists, kForall
      class Fun  f_s;  // kStrippedFun
      class Name n_s;  // kStrippedName
      class Lit  a;    // kStrippedLit
      int        k;    // kOr, kAnd, kKnow, kMaybe
      struct {
        int      k;
        int      l;
      } kl;            // kBelieve
    } u{};

    explicit Symbol() = default;

    Symbol(const Symbol&)            = default;
    Symbol& operator=(const Symbol&) = default;
    Symbol(Symbol&&)                 = default;
    Symbol& operator=(Symbol&&)      = default;

    bool operator==(const Symbol& s) const {
      if (tag != s.tag) {
        return false;
      }
      switch (tag) {
        case kFun:          return u.f == s.u.f;
        case kName:         return u.n == s.u.n;
        case kVar:          return u.x == s.u.x;
        case kStrippedFun:  return u.f_s == s.u.f_s;
        case kStrippedName: return u.n_s == s.u.n_s;
        case kEquals:       return true;
        case kNotEquals:    return true;
        case kStrippedLit:  return u.a == s.u.a;
        case kNot:          return true;
        case kExists:       return u.x == s.u.x;
        case kForall:       return u.x == s.u.x;
        case kOr:           return u.k == s.u.k;
        case kAnd:          return u.k == s.u.k;
        case kKnow:         return u.k == s.u.k;
        case kMaybe:        return u.k == s.u.k;
        case kBelieve:      return u.kl.k == s.u.kl.k && u.kl.l == s.u.kl.l;
        case kAction:       return true;
      }
      std::abort();
    }

    int arity() const {
      switch (tag) {
        case kFun:          return u.f.arity();
        case kName:         return u.n.arity();
        case kVar:          return 0;
        case kStrippedFun:  return 0;
        case kStrippedName: return 0;
        case kEquals:       return 2;
        case kNotEquals:    return 2;
        case kStrippedLit:  return 0;
        case kNot:          return 1;
        case kExists:       return 1;
        case kForall:       return 1;
        case kOr:           return u.k;
        case kAnd:          return u.k;
        case kKnow:         return 1;
        case kMaybe:        return 1;
        case kBelieve:      return 1;
        case kAction:       return 2;
      }
      std::abort();
    }

    bool stripped()         const { return tag == kStrippedFun || tag == kStrippedName || tag == kStrippedLit; }
    bool unstripped_term()  const { return tag == kFun || tag == kName || tag == kVar; }
    bool unstripped()       const { return unstripped_term() || tag == kEquals || tag == kNotEquals; }
    bool fun()              const { return tag == kFun || tag == kStrippedFun; }
    bool name()             const { return tag == kName || tag == kStrippedName; }
    bool var()              const { return tag == kVar; }
    bool term()             const { return fun() || name() || var(); }
    bool literal()          const { return tag == kEquals || tag == kNotEquals || tag == kStrippedLit; }
    bool quantifier()       const { return tag == kExists || tag == kForall; }
    bool objective()        const { return tag != kKnow || tag != kMaybe || tag != kBelieve; }

    Sort sort() const {
      assert(term());
      if (stripped()) {
        const RWord w = instance().Unstrip(*this);
        assert(w.begin() != w.end() && !w.begin()->stripped());
        return w.begin()->sort();
      } else if (tag == kFun) {
        return u.f.sort();
      } else if (tag == kName) {
        return u.n.sort();
      } else {
        assert(tag == kVar);
        return u.x.sort();
      }
    }
  };

  class RWord {
   public:
    explicit RWord() = default;
    explicit RWord(Symbol::CRef begin, Symbol::CRef end) : begin_(begin), end_(end) {}

    RWord(const RWord&)            = default;
    RWord& operator=(const RWord&) = default;
    RWord(RWord&&)                 = default;
    RWord& operator=(RWord&&)      = default;

    bool empty() const { return begin_ == end_; }

    Symbol::CRef begin() const { return begin_; }
    Symbol::CRef end()   const { return end_; }

   private:
    template<typename T>
    struct EmptyList {
      static const std::list<T> kInstance;
    };

    Symbol::CRef begin_ = EmptyList<Symbol>::kInstance.begin();
    Symbol::CRef end_   = EmptyList<Symbol>::kInstance.end();
  };

  class Word {
   public:
    explicit Word() = default;
    explicit Word(const RWord& w) : Word(w.begin(), w.end()) {}
    explicit Word(Symbol::List&& w) : symbols_(w) {}

    template<typename InputIt>
    explicit Word(InputIt begin, InputIt end) : symbols_(begin, end) {}

    bool operator==(const Word& w) const { return symbols_ == w.symbols_; }
    bool operator!=(const Word& w) const { return !(*this == w); }

    Word(Word&&)            = default;
    Word& operator=(Word&&) = default;

    Word Clone() const { return Word(*this); }

    RWord readable() const { return RWord(begin(), end()); }

    bool empty() const { return symbols_.empty(); }
    int size()   const { return symbols_.size(); }

    Symbol::Ref begin() { return symbols_.begin(); }
    Symbol::Ref end()   { return symbols_.end(); }

    Symbol::CRef begin() const { return symbols_.begin(); }
    Symbol::CRef end()   const { return symbols_.end(); }

    template<typename InputIt>
    Symbol::Ref Insert(Symbol::Ref before, InputIt first, InputIt last) { return symbols_.insert(before, first, last); }
    Symbol::Ref Insert(Symbol::Ref before, Symbol s) { return symbols_.insert(before, s); }

    Symbol::Ref Erase(Symbol::Ref first, Symbol::Ref last) { return symbols_.erase(first, last); }
    Symbol::Ref Erase(Symbol::Ref it) { return symbols_.erase(it); }

    void Move(Symbol::Ref before, Symbol::Ref first) { symbols_.splice(before, symbols_, first); }

    Symbol::List TakeOut(Symbol::Ref first, Symbol::Ref last) {
      Symbol::List tgt;
      tgt.splice(tgt.end(), symbols_, first, last);
      return tgt;
    }

    void PutIn(Symbol::Ref before, Symbol::List&& symbols) {
      symbols_.splice(before, symbols, symbols.begin(), symbols.end());
    }

    void PutIn(Symbol::Ref before, Word&& w) { PutIn(before, std::move(w.symbols_)); }

   private:
    Word(const Word&)            = default;
    Word& operator=(const Word&) = default;

    Symbol::List symbols_{};
  };

  static Alphabet& instance() {
    if (instance_ == nullptr) {
      instance_ = std::unique_ptr<Alphabet>(new Alphabet());
    }
    return *instance_.get();
  }

  static void reset_instance() {
    instance_ = nullptr;
  }

  Sort CreateSort(bool rigid) {
    // XXX Maybe future Sorts shouldn't be non-rigid/rigid but of different order
    // and the standard names of each order can only be formed with argumenst of
    // lower order.
    const Sort s = Sort(++last_sort_);
    sort_rigid_[s] = rigid;
    return s;
  }

  FunSymbol CreateFun(Sort s, int arity) {
    // XXX Should functions of a rigid sort be allowed?
    assert(!s.rigid());
    if (s.rigid()) {
      std::abort();
    }
    const FunSymbol f = FunSymbol(++last_fun_);
    fun_sort_[f] = s;
    fun_arity_[f] = arity;
    return f;
  }

  NameSymbol CreateName(Sort s, int arity = 0) {
    assert(s.rigid() || arity == 0);
    if (!s.rigid() && arity > 0) {
      std::abort();
    }
    const NameSymbol n = NameSymbol(++last_name_);
    name_sort_[n] = s;
    name_arity_[n] = arity;
    return n;
  }

  VarSymbol CreateVar(Sort s) {
    const VarSymbol x = VarSymbol(++last_var_);
    var_sort_[x] = s;
    return x;
  }

  FunSymbol Squaring(int sitlen, FunSymbol f) {
    if (sitlen == 0) {
      return f;
    } else {
      const int k = sitlen - 1;
      swear_funcs_.resize(sitlen);
      DenseMap<FunSymbol, FunSymbol>& map = swear_funcs_[k];
      if (map[f].null()) {
        map[f] = CreateFun(f.sort(), sitlen + f.arity());
      }
      return map[f];
    }
  }

  Symbol Strip(Word&& w) {
    assert(w.begin()->tag == Symbol::kFun || w.begin()->tag == Symbol::kName);
    assert(std::all_of(std::next(w.begin()), w.end(), [](const Symbol& s) { return s.tag == Symbol::kStrippedName; }));
    auto it = symbols_term_.find(w.readable());
    if (it != symbols_term_.end()) {
      return it->second;
    } else {
      if (w.begin()->tag == Symbol::kFun) {
        const Fun f = Fun::FromId(++last_fun_term_);
        const Symbol s = Symbol::StrippedFun(f);
        term_fun_symbols_[f] = std::move(w);
        symbols_term_[term_fun_symbols_[f].readable()] = s;
        return s;
      } else {
        const Name n = Name::FromId(++last_name_term_);
        const Symbol s = Symbol::StrippedName(n);
        term_name_symbols_[n] = std::move(w);
        symbols_term_[term_name_symbols_[n].readable()] = s;
        return s;
      }
    }
  }

  RWord Unstrip(const Symbol& s) const {
    assert(s.stripped() && s.term());
    return s.tag == Symbol::kStrippedFun ? Unstrip(s.u.f_s) : Unstrip(s.u.n_s);
  }

  RWord Unstrip(Fun f)  const { return term_fun_symbols_.key_in_range(f) ? term_fun_symbols_[f].readable() : RWord(); }
  RWord Unstrip(Name n) const { return term_name_symbols_.key_in_range(n) ? term_name_symbols_[n].readable() : RWord(); }

 private:
  struct DeepHash {
    internal::hash32_t operator()(const RWord& w) const {
      internal::hash32_t h = 0;
      for (const Symbol& s : w) {
        assert(s.term());
        int i = 0;
        switch (s.tag) {
          case Symbol::kFun:          i = s.u.f.id();   break;
          case Symbol::kName:         i = s.u.n.id();   break;
          case Symbol::kVar:          i = s.u.x.id();   break;
          case Symbol::kStrippedFun:  i = s.u.f_s.id(); break;
          case Symbol::kStrippedName: i = s.u.n_s.id(); break;
          default:                                      break;
        }
        h ^= internal::jenkins_hash(s.tag);
        h ^= internal::jenkins_hash(i);
      }
      return h;
    }
  };

  struct DeepEquals {
    bool operator()(const RWord& w1, const RWord& w2) const {
      return std::equal(w1.begin(), w1.end(), w2.begin());
    }
  };

  using TermMap = std::unordered_map<RWord, Symbol, DeepHash, DeepEquals>;

  explicit Alphabet() = default;

  Alphabet(const Alphabet&)            = delete;
  Alphabet& operator=(const Alphabet&) = delete;
  Alphabet(Alphabet&&)                 = delete;
  Alphabet& operator=(Alphabet&&)      = delete;

  DenseMap<Sort,       bool> sort_rigid_{};
  DenseMap<FunSymbol,  Sort> fun_sort_{};
  DenseMap<FunSymbol,  int>  fun_arity_{};
  DenseMap<NameSymbol, Sort> name_sort_{};
  DenseMap<NameSymbol, int>  name_arity_{};
  DenseMap<VarSymbol,  Sort> var_sort_{};

  TermMap              symbols_term_{};
  DenseMap<Fun,  Word> term_fun_symbols_{};
  DenseMap<Name, Word> term_name_symbols_{};
  int                  last_sort_      = 0;
  int                  last_var_       = 0;
  int                  last_fun_       = 0;
  int                  last_name_      = 0;
  int                  last_fun_term_  = 0;
  int                  last_name_term_ = 0;

  std::vector<DenseMap<FunSymbol, FunSymbol>> swear_funcs_{};
};

template<typename T>
const std::list<T> Alphabet::RWord::EmptyList<T>::kInstance;

class FormulaCommons {
 public:
  using Abc = Alphabet;

  class Scope {
   public:
    class Observer {
     public:
      explicit Observer() = default;

      Observer(const Observer&) = delete;
      Observer& operator=(const Observer&) = delete;
      Observer(Observer&&) = delete;
      Observer& operator=(Observer&&) = delete;

      void Munch(const Abc::Symbol& s) { k_ += s.arity(); --k_; }
      void Vomit(const Abc::Symbol& s) { k_ -= s.arity(); --k_; }  // XXX shouldn't it be ++k_?
      Scope scope()                   const { return Scope(k_); }
      bool active(const Scope& scope) const { return k_ >= scope.k_; }

     private:
      int k_ = 0;
    };

    explicit Scope() = default;
    explicit Scope(int k) : k_(k) {}

    Scope(const Scope&)            = default;
    Scope& operator=(const Scope&) = default;
    Scope(Scope&&)                 = default;
    Scope& operator=(Scope&&)      = default;

   private:
    int k_ = -1;
  };

  template<typename InputIt>
  static InputIt End(InputIt it) {
    Scope::Observer scoper;
    Scope scope = scoper.scope();
    while (scoper.active(scope)) {
      scoper.Munch(*it++);
    }
    return it;
  }

 protected:
  FormulaCommons() = default;
};

class RFormula : private FormulaCommons {
 public:
  explicit RFormula() = default;
  explicit RFormula(Abc::Symbol::CRef begin, Abc::Symbol::CRef end) : rword_(begin, end) {}
  explicit RFormula(const Abc::RWord& w) : RFormula(w.begin(), w.end()) {}
  explicit RFormula(Abc::Symbol::CRef begin) : RFormula(begin, End(begin)) {}

  const Abc::RWord& rword() const { return rword_; }
  const Abc::Symbol head()  const { return !empty() ? *begin() : Abc::Symbol(); }
  Abc::Symbol::Tag tag()    const { return head().tag; }
  int arity()               const { return head().arity(); }

  const RFormula& arg(int i) const {
    InitArg(i);
    return args_[i];
  }

  const std::vector<RFormula>& args() const {
    InitArg(arity() - 1);
    return args_;
  }

  bool empty() const { return rword_.empty(); }

  Abc::Symbol::CRef begin() const { return rword_.begin(); }
  Abc::Symbol::CRef end()   const { return rword_.end(); }

  Abc::DenseSet<Abc::VarSymbol> FreeVars() const {
    struct QuantifierMarker {
      QuantifierMarker(Abc::VarSymbol x, Scope scope) : x(x), scope(scope) {}
      explicit QuantifierMarker(Scope scope) : scope(scope) {}
      Abc::VarSymbol x;
      Scope scope;
    };
    Abc::DenseSet<Abc::VarSymbol> free;
    Abc::DenseMap<Abc::VarSymbol, int> bound;
    std::vector<QuantifierMarker> quantifiers;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall) {
        ++bound[it->u.x];
        quantifiers.push_back(QuantifierMarker(it->u.x, scoper.scope()));
      } else if (it->tag == Abc::Symbol::kVar && bound[it->u.x] == 0) {
        free[it->u.x] = true;
      }
      scoper.Munch(*it++);
      while (!quantifiers.empty() && !scoper.active(quantifiers.back().scope)) {
        --bound[quantifiers.back().x];
        quantifiers.pop_back();
      }
    }
    return free;
  }

  Abc::DenseSet<Abc::VarSymbol> BoundVars() const {
    Abc::DenseSet<Abc::VarSymbol> bound;
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall) {
        bound[it->u.x] = true;
      }
    }
    return bound;
  }

  internal::Maybe<std::vector<std::vector<Lit>>> CnfClauses() {
    assert(weakly_well_formed() && ground() && stripped());
    std::vector<std::vector<Lit>> cs;
    Scope::Observer scoper;
    bool disjunction = false;
    bool disjunction_valid = false;
    Scope disjunction_scope = scoper.scope();
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Abc::Symbol::kOr) {
        if (!disjunction) {
          disjunction = true;
          disjunction_valid = false;
          disjunction_scope = scoper.scope();
          cs.emplace_back();
        }
      } else if (disjunction && it->tag == Abc::Symbol::kAnd && it->arity() == 0) {
        disjunction_valid = true;
        cs.pop_back();
      } else if (disjunction && it->tag == Abc::Symbol::kStrippedLit) {
        if (!disjunction_valid) {
          cs.back().push_back(it->u.a);
        }
      } else if (!disjunction && it->tag == Abc::Symbol::kStrippedLit) {
        cs.emplace_back();
        cs.back().push_back(it->u.a);
      } else if (!disjunction && it->tag == Abc::Symbol::kAnd) {
      } else {
        return internal::Nothing;
      }
      scoper.Munch(*it++);
      disjunction &= scoper.active(disjunction_scope);
    }
    return internal::Just(std::move(cs));
  }

  bool SatisfiedBy(const TermMap<Fun, Name>& model, std::vector<Lit>* reason) const {
    assert(stripped());
    assert(nnf());
    struct ConnectiveMarker {
      explicit ConnectiveMarker() = default;
      explicit ConnectiveMarker(bool conjunctive, const std::vector<Lit>* reason, Scope scope)
          : conjunctive(conjunctive), backtrack(int(reason != nullptr ? reason->size() : 0)), scope(scope) {}
      bool conjunctive = false;
      int backtrack;
      int satisfied = 2 * conjunctive - 1;
      Scope scope;
    };
    auto unexplain = [&reason](const ConnectiveMarker& m) { if (reason != nullptr) { reason->resize(m.backtrack); } };
    std::vector<ConnectiveMarker> s;
    Scope::Observer scoper;
    s.push_back(ConnectiveMarker(false, reason, scoper.scope()));
    for (auto it = begin(); it != end(); ) {
      switch (it->tag) {
        case Abc::Symbol::kOr: {
          s.push_back(ConnectiveMarker(false, reason, scoper.scope()));
          break;
        }
        case Abc::Symbol::kAnd: {
          s.push_back(ConnectiveMarker(true, reason, scoper.scope()));
          break;
        }
        case Abc::Symbol::kStrippedLit: {
          ConnectiveMarker& cm = s.back();
          if (cm.conjunctive == (cm.satisfied > 0)) {
            const Lit a = it->u.a;
            const Fun f = a.fun();
            const Name m = model.key_in_range(f) ? model[f] : Name();
            const Name n = a.name();
            cm.satisfied = m.null() ? 0 : a.pos() == (m == n) ? 1 : -1;
            if (cm.satisfied > 0) {
              if (reason != nullptr) {
                reason->push_back(a);
              }
            } else {
              unexplain(cm);
            }
          }
          break;
        }
        case Abc::Symbol::kNot:
        case Abc::Symbol::kFun:
        case Abc::Symbol::kVar:
        case Abc::Symbol::kName:
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall:
        case Abc::Symbol::kAction:
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:
          assert(false);
          std::abort();
          break;
      }
      scoper.Munch(*it++);
      while (s.size() >= 2 && !scoper.active(s.back().scope)) {
        ConnectiveMarker& cm = s[s.size() - 2];
        ConnectiveMarker& cn = s.back();
        if (cm.conjunctive == (cm.satisfied > 0)) {
          cm.satisfied = cn.satisfied;
          if (cm.satisfied <= 0) {
            unexplain(cm);
          }
        } else {
          unexplain(cn);
        }
        s.pop_back();
      }
    }
    assert(s.size() == 1);
    return s[0].satisfied > 0;
  }

  // Strongly well formed means the formula is well formed as defined in the
  // paper, i.e., quasi-primitive literals, quasi-trivial actions (flattened).
  // Weakly well formed admits nested functions and right-hand side functions.
  bool stripped()             const { if (!m_.init) { InitMeta(); } return m_.stripped; }
  bool ground()               const { if (!m_.init) { InitMeta(); } return m_.ground; }
  bool objective()            const { if (!m_.init) { InitMeta(); } return m_.objective; }
  bool subjective()           const { if (!m_.init) { InitMeta(); } return m_.subjective; }
  bool dynamic()              const { if (!m_.init) { InitMeta(); } return m_.dynamic; }
  bool nnf()                  const { if (!m_.init) { InitMeta(); } return m_.nnf; }
  bool proper_plus()          const { if (!m_.init) { InitMeta(); } return m_.proper_plus; }
  bool weakly_well_formed()   const { if (!m_.init) { InitMeta(); } return m_.weakly_well_formed; }
  bool strongly_well_formed() const { if (!m_.init) { InitMeta(); } return m_.strongly_well_formed; }

 private:
  void InitMeta() const { const_cast<RFormula*>(this)->InitMeta(); }

  void InitMeta() {
    enum Expectation { kTerm, kQuasiName, kFormula };
    std::vector<Expectation> es;
    Scope::Observer scoper;
    Scope action_scope;
    Scope negation_scope;
    Scope epistemic_scope;
    bool action_scope_active    = false;
    bool negation_scope_active  = false;
    bool epistemic_scope_active = false;
    m_.init                 = true;
    m_.stripped             = true;
    m_.ground               = true;
    m_.objective            = true;
    m_.subjective           = true;
    m_.dynamic              = false;
    m_.nnf                  = true;
    m_.proper_plus          = true;
    m_.weakly_well_formed   = true;
    m_.strongly_well_formed = true;
    for (auto it = begin(); it != end(); ) {
      const Abc::Symbol& s = *it;
      if (s.tag == Abc::Symbol::kNot && !negation_scope_active) {
        negation_scope = scoper.scope();
        negation_scope_active = true;
      }
      if (s.tag == Abc::Symbol::kAction && !action_scope_active) {
        action_scope = scoper.scope();
        action_scope_active = true;
      }
      if (!s.objective() && !action_scope_active) {
        epistemic_scope = scoper.scope();
        epistemic_scope_active = true;
      }
      negation_scope_active  &= scoper.active(negation_scope);
      action_scope_active    &= scoper.active(action_scope);
      epistemic_scope_active &= scoper.active(epistemic_scope);
      m_.stripped    &= !s.unstripped();
      m_.ground      &= s.tag != Abc::Symbol::kVar && !s.quantifier();
      m_.objective   &= s.objective();
      m_.subjective  &= epistemic_scope_active || (!s.fun() && s.tag != Abc::Symbol::kStrippedLit);
      m_.dynamic     |= s.tag == Abc::Symbol::kAction;
      m_.nnf         &= s.tag != Abc::Symbol::kNot &&
                        (!action_scope_active || s.tag == Abc::Symbol::kAction || s.term() || s.literal());
      m_.proper_plus &= s.term() || s.literal() ||
                        (negation_scope_active && (s.tag == Abc::Symbol::kExists || s.tag == Abc::Symbol::kAnd)) ||
                        (!negation_scope_active && (s.tag == Abc::Symbol::kForall || s.tag == Abc::Symbol::kOr));
      if (it != begin()) {
        if (!es.empty()) {
          switch (es.back()) {
            case kTerm:       m_.weakly_well_formed &= s.term();  m_.strongly_well_formed &= s.term();            break;
            case kQuasiName:  m_.weakly_well_formed &= s.term();  m_.strongly_well_formed &= s.name() || s.var(); break;
            case kFormula:    m_.weakly_well_formed &= !s.term(); m_.strongly_well_formed &= !s.term();           break;
          }
          es.pop_back();
        } else {
          m_.weakly_well_formed   &= !s.term();
          m_.strongly_well_formed &= !s.term();
        }
      }
      switch (s.tag) {
        case Abc::Symbol::kFun:
        case Abc::Symbol::kName:         for (int i = 0; i < s.arity(); ++i) { es.push_back(kQuasiName); } break;
        case Abc::Symbol::kVar:
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
        case Abc::Symbol::kStrippedLit:  break;
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:    es.push_back(kQuasiName); es.push_back(kTerm); break;
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall:
        case Abc::Symbol::kOr:
        case Abc::Symbol::kAnd:
        case Abc::Symbol::kNot:
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:      for (int i = 0; i < s.arity(); ++i) { es.push_back(kFormula); } break;
        case Abc::Symbol::kAction:       es.push_back(kFormula); es.push_back(kQuasiName); break;
      }
      scoper.Munch(*it++);
    }
    m_.weakly_well_formed   &= es.empty();
    m_.strongly_well_formed &= es.empty();
  }

  void InitArg(int i) const { const_cast<RFormula*>(this)->InitArg(i); }

  void InitArg(int i) {
    args_.reserve(arity());
    while (int(args_.size()) <= i) {
      args_.push_back(RFormula(!args_.empty() ? args_.back().end() : std::next(begin())));
    }
  }

  Abc::RWord rword_{};
  mutable std::vector<RFormula> args_{};
  struct {
    unsigned init                 : 1;
    unsigned ground               : 1;
    unsigned stripped             : 1;
    unsigned objective            : 1;
    unsigned subjective           : 1;
    unsigned dynamic              : 1;
    unsigned nnf                  : 1;
    unsigned proper_plus          : 1;
    unsigned weakly_well_formed   : 1;
    unsigned strongly_well_formed : 1;
  } m_{};
};

class Formula : private FormulaCommons {
 public:
  template<typename Formulas>
  static Formula Fun(Abc::FunSymbol f, Formulas&& args)            { Formula ff(Abc::Symbol::Fun(f)); ff.AddArgs(args); return ff; }
  static Formula Fun(Abc::FunSymbol f)                             { Formula ff(Abc::Symbol::Fun(f)); return ff; }
  static Formula Fun(class Fun f)                                  { Formula ff(Abc::Symbol::StrippedFun(f)); return ff; }
  template<typename Formulas>
  static Formula Name(Abc::NameSymbol n, Formulas&& args)          { Formula ff(Abc::Symbol::Name(n)); ff.AddArgs(args); return ff; }
  static Formula Name(Abc::NameSymbol n)                           { Formula ff(Abc::Symbol::Name(n)); return ff; }
  static Formula Name(class Name n)                                { Formula ff(Abc::Symbol::StrippedName(n)); return ff; }
  static Formula Lit(class Lit a)                                  { return Formula(Abc::Symbol::Lit(a)); }
  static Formula Var(Abc::VarSymbol x)                             { return Formula(Abc::Symbol::Var(x)); }
  static Formula Equals(Formula&& f1, Formula&& f2)                { return Formula(Abc::Symbol::Equals(), f1, f2); }
  static Formula Equals(const Formula& f1, const Formula& f2)      { return Formula(Abc::Symbol::Equals(), f1, f2); }
  static Formula NotEquals(Formula&& f1, Formula&& f2)             { return Formula(Abc::Symbol::NotEquals(), f1, f2); }
  static Formula NotEquals(const Formula& f1, const Formula& f2)   { return Formula(Abc::Symbol::NotEquals(), f1, f2); }
  static Formula Not(Formula&& f)                                  { return Formula(Abc::Symbol::Not(), f); }
  static Formula Not(const Formula& f)                             { return Formula(Abc::Symbol::Not(), f); }
  static Formula Exists(Abc::VarSymbol x, Formula&& f)             { return Formula(Abc::Symbol::Exists(x), f); }
  static Formula Exists(Abc::VarSymbol x, const Formula& f)        { return Formula(Abc::Symbol::Exists(x), f); }
  static Formula Forall(Abc::VarSymbol x, Formula&& f)             { return Formula(Abc::Symbol::Forall(x), f); }
  static Formula Forall(Abc::VarSymbol x, const Formula& f)        { return Formula(Abc::Symbol::Forall(x), f); }
  static Formula Or(Formula&& f1, Formula&& f2)                    { return Formula(Abc::Symbol::Or(2), f1, f2); }
  static Formula Or(const Formula& f1, const Formula& f2)          { return Formula(Abc::Symbol::Or(2), f1, f2); }
  template<typename Formulas>
  static Formula Or(Formulas&& fs)                                 { Formula ff(Abc::Symbol::Or(fs.size())); ff.AddArgs(fs); return ff; }
  static Formula And(Formula&& f1, Formula&& f2)                   { return Formula(Abc::Symbol::And(2), f1, f2); }
  static Formula And(const Formula& f1, const Formula& f2)         { return Formula(Abc::Symbol::And(2), f1, f2); }
  template<typename Formulas>
  static Formula And(Formulas&& fs)                                { Formula ff(Abc::Symbol::And(fs.size())); ff.AddArgs(fs); return ff; }
  static Formula Know(int k, Formula&& f)                          { return Formula(Abc::Symbol::Know(k), f); }
  static Formula Know(int k, const Formula& f)                     { return Formula(Abc::Symbol::Know(k), f); }
  static Formula Maybe(int k, Formula&& f)                         { return Formula(Abc::Symbol::Maybe(k), f); }
  static Formula Maybe(int k, const Formula& f)                    { return Formula(Abc::Symbol::Maybe(k), f); }
  static Formula Believe(int k, int l, Formula&& f1, Formula&& f2) { return Formula(Abc::Symbol::Believe(k, l), f1, f2); }
  static Formula Believe(int k, int l, const Formula& f1, const Formula& f2)
                                                                   { return Formula(Abc::Symbol::Believe(k, l), f1, f2); }
  static Formula Action(Formula&& t, Formula&& f)                  { return Formula(Abc::Symbol::Action(), t, f); }
  static Formula Action(const Formula& t, const Formula& f)        { return Formula(Abc::Symbol::Action(), t, f); }

  explicit Formula(Abc::Word&& w) : word_(std::move(w)) {}
  explicit Formula(const RFormula& f) : word_(f.rword()) {}

  Formula()                          = default;
  Formula(const Formula&)            = delete;
  Formula& operator=(const Formula&) = delete;
  Formula(Formula&&)                 = default;
  Formula& operator=(Formula&&)      = default;

  bool operator==(const Formula& f) const { return word_ == f.word_; }
  bool operator!=(const Formula& f) const { return !(*this == f); }

  Formula Clone() const { return Formula(word_.Clone()); }

  RFormula           readable() const { return RFormula(begin(), end()); }
  const Abc::Word&   word()     const { return word_; }
  const Abc::Symbol& head()     const { return *begin(); }
  Abc::Symbol::Tag   tag()      const { return head().tag; }
  int                arity()    const { return head().arity(); }

  bool empty() const { return word_.empty(); }

  Abc::Symbol::CRef begin() const { return word_.begin(); }
  Abc::Symbol::CRef end()   const { return word_.end(); }

  Abc::Symbol::Ref begin() { return word_.begin(); }
  Abc::Symbol::Ref end()   { return word_.end(); }

  void Normalize() {
    Rectify();
    Flatten();
    PushInwards();
  }

  // Eliminates existential (or, within an odd number of negations, universal)
  // quantifiers. Traditionally, every variable x with an existential quantifier
  // is within universal quantifiers for x_1,...,x_m is replaced with a new
  // function f(x_1,...,x_m). This is unsound in our case when there are actions
  // between different occurrences of x, because the f(x_1,...,x_m) would not
  // refer to the same objects then. Hence we instead replace the formula
  // ex x phi with fa x (f(x_1,...,x_m) != x v phi). This is similar to doing
  // normal Skolemization and flatten immediately.
  // Precondition: Existential variables must be of non-rigid sort.
  // Reason: Rigid sorts do not support functions (at least for now).
  void Skolemize() {
    assert(readable().weakly_well_formed());
    struct ForallMarker {
      explicit ForallMarker(Abc::VarSymbol x, Scope scope) : x(x), scope(scope) {}
      explicit ForallMarker(Scope scope) : scope(scope) {}
      Abc::VarSymbol x{};
      Scope scope{};
    };
    struct NotMarker {
      explicit NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      explicit NotMarker(Scope scope) : scope(scope) {}
      bool neg = false;
      Scope scope{};
    };
    std::vector<ForallMarker> foralls;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      const Abc::Symbol s = *it;
      const bool pos = nots.empty() || !nots.back().neg;
      switch (s.tag) {
        case Abc::Symbol::kFun:
        case Abc::Symbol::kName:
        case Abc::Symbol::kVar:
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:
        case Abc::Symbol::kStrippedLit:
        case Abc::Symbol::kOr:
        case Abc::Symbol::kAnd:
        case Abc::Symbol::kAction:
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall:
          if ((s.tag == Abc::Symbol::kExists) == pos) {
            Abc::Symbol::List symbols;
            for (const ForallMarker& fm : foralls) {
              symbols.push_back(Abc::Symbol::Var(fm.x));
            }
            const Abc::FunSymbol f = Abc::instance().CreateFun(s.u.x.sort(), symbols.size());
            symbols.push_front(Abc::Symbol::Fun(f));
            symbols.push_back(Abc::Symbol::Var(s.u.x));
            symbols.push_front(pos ? Abc::Symbol::NotEquals() : Abc::Symbol::Equals());
            symbols.push_front(pos ? Abc::Symbol::Or(2) : Abc::Symbol::And(2));
            it->tag = pos ? Abc::Symbol::kForall : Abc::Symbol::kExists;
            word_.PutIn(std::next(it), std::move(symbols));
            scoper.Munch(*it++);
          } else {
            scoper.Munch(*it++);
            foralls.push_back(ForallMarker(s.u.x, scoper.scope()));
          }
          break;
        case Abc::Symbol::kNot:
          scoper.Munch(*it++);
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          break;
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:
          scoper.Munch(*it++);
          nots.push_back(NotMarker(scoper.scope()));
          break;
      }
      while (!foralls.empty() && !scoper.active(foralls.back().scope)) {
        foralls.pop_back();
      }
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
    }
    assert(readable().weakly_well_formed());
  }

  // Replaces every function f(t_{m+1},...,t_n) in the scope of actions
  // t_1,...,t_m with a new function f^m(t_1,...,t_n).
  // Precondition: Formula is objective.
  // Reason: We can't push actions inside epistemic operators.
  void Squaring() {
    assert(readable().weakly_well_formed());
    struct ActionMarker {
      explicit ActionMarker(Abc::Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      Abc::Symbol::List symbols{};
      Scope scope{};
    };
    std::vector<ActionMarker> actions;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      switch (it->tag) {
        case Abc::Symbol::kFun:
          scoper.Munch(*it);
          it->u.f = Abc::instance().Squaring(actions.size(), it->u.f);
          ++it;
          for (const ActionMarker& am : actions) {
            word_.Insert(it, am.symbols.begin(), am.symbols.end());
          }
          break;
        case Abc::Symbol::kVar:
        case Abc::Symbol::kName:
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:
        case Abc::Symbol::kStrippedLit:
        case Abc::Symbol::kNot:
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall:
        case Abc::Symbol::kOr:
        case Abc::Symbol::kAnd:
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kAction: {
          it = word_.Erase(it);
          auto first = it;
          auto last = End(it);
          it = last;
          actions.push_back(ActionMarker(word_.TakeOut(first, last), scoper.scope()));
          break;
        }
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:
          assert(false);
          std::abort();
          break;
      }
      while (!actions.empty() && !scoper.active(actions.back().scope)) {
        actions.pop_back();
      }
    }
    assert(readable().weakly_well_formed());
    assert(!readable().dynamic());
  }

  // Introduces new variables so that no variable is quantified twice.
  // If all_new is true, then every variable is replaced with a new one;
  // otherwise the first occurrences remain unaltered.
  void Rectify(bool all_new = false) {
    assert(readable().weakly_well_formed());
    struct NewVar {
      explicit NewVar() = default;
      explicit NewVar(Abc::VarSymbol x, Scope scope) : x(x), scope(scope) {}
      Abc::VarSymbol x{};
      Scope scope{};
    };
    struct NewVars {
      explicit NewVars() = default;
      bool used = false;
      std::vector<NewVar> vars{};
    };
    Abc::DenseMap<Abc::VarSymbol, NewVars> vars;
    Scope::Observer scoper;
    for (Abc::Symbol& s : *this) {
      if (s.tag == Abc::Symbol::kExists || s.tag == Abc::Symbol::kForall) {
        const Abc::VarSymbol y = !vars[s.u.x].used && !all_new ? s.u.x : Abc::instance().CreateVar(s.u.x.sort());
        vars[s.u.x].used = true;
        vars[s.u.x].vars.push_back(NewVar(y, scoper.scope()));
        s.u.x = y;
      } else if (s.tag == Abc::Symbol::kVar && !vars[s.u.x].vars.empty()) {
        s.u.x = vars[s.u.x].vars.back().x;
      }
      scoper.Munch(s);
      for (auto& nvs : vars.values()) {
        while (!nvs.vars.empty() && !scoper.active(nvs.vars.back().scope)) {
          nvs.vars.pop_back();
        }
      }
    }
    assert(readable().weakly_well_formed());
  }

  // Flattens terms in order to make literals quasi-primitive and actions
  // quasi-names. That is, the left-hand side of a literal is a variable, name,
  // or function applied to variables and/or names. The right-hand side of a
  // literal is a name or a variable. The term in an action operator is a name
  // or a variable.
  // This requires newly introduced universally (or, within an odd number of
  // negations, existentially) quantified variables.
  void Flatten() {
    assert(readable().weakly_well_formed());
    struct NotMarker {
      explicit NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      explicit NotMarker(Scope scope) : reset(true), scope(scope) {}
      bool reset = false;
      bool neg   = false;
      Scope scope{};
    };
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    Abc::Symbol::Ref or_ref = end();
    bool tolerate_fun = false;
    for (auto it = begin(); it != end(); ) {
      const bool pos = nots.empty() || !nots.back().neg;
      switch (it->tag) {
        case Abc::Symbol::kFun:
          if (!tolerate_fun) {
            const Abc::VarSymbol x = Abc::instance().CreateVar(it->u.f.sort());
            it = word_.Insert(it, Abc::Symbol::Var(x));
            const Abc::Symbol::Ref first = std::next(it);
            const Abc::Symbol::Ref last = End(first);
            Abc::Symbol::List symbols = word_.TakeOut(first, last);
            symbols.push_back(Abc::Symbol::Var(x));
            symbols.push_front(pos ? Abc::Symbol::NotEquals() : Abc::Symbol::Equals());
            auto bt = word_.Insert(or_ref, pos ? Abc::Symbol::Forall(x) : Abc::Symbol::Exists(x));
            while (--it != bt) {
              scoper.Vomit(*it);
            }
            ++(or_ref->u.k);
            word_.PutIn(std::next(or_ref), std::move(symbols));
          }
        case Abc::Symbol::kName:
        case Abc::Symbol::kVar:
          scoper.Munch(*it++);
          tolerate_fun = false;
          break;
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:
          or_ref = word_.Insert(it, pos ? Abc::Symbol::Or(1) : Abc::Symbol::And(1));
          scoper.Munch(*or_ref);
          scoper.Munch(*it++);
          tolerate_fun = true;
          break;
        case Abc::Symbol::kAction:
          or_ref = word_.Insert(it, pos ? Abc::Symbol::Or(1) : Abc::Symbol::And(1));
          scoper.Munch(*or_ref);
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kOr:
        case Abc::Symbol::kAnd:
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
        case Abc::Symbol::kStrippedLit:
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kNot:
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall:
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:
          scoper.Munch(*it++);
          nots.push_back(NotMarker(scoper.scope()));
          break;
      }
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
    }
    assert(readable().weakly_well_formed());
    assert(readable().strongly_well_formed());
  }

  // Pushes actions and negations inwards, pulls quantifiers out of dis- and
  // conjunctions, pulls quantifiers out of dis- and conjunctions.
  // Precondition: Formula is rectified.
  // Reasons: (1) We pull quantifiers out of dis- and conjunctions.
  //          (2) We push actions inwards.
  void PushInwards() {
    assert(readable().weakly_well_formed());
    struct AndOrMarker {
      explicit AndOrMarker(Abc::Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      explicit AndOrMarker(Scope scope) : reset(true), scope(scope) {}
      bool reset = false;
      Abc::Symbol::Ref ref{};
      Scope scope{};
    };
    struct ActionMarker {
      explicit ActionMarker(Abc::Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      explicit ActionMarker(Scope scope) : reset(true), scope(scope) {}
      bool reset = false;
      Abc::Symbol::List symbols{};
      Scope scope{};
    };
    struct NotMarker {
      explicit NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      explicit NotMarker(Scope scope) : reset(true), scope(scope) {}
      bool reset = false;
      bool neg   = false;
      Scope scope{};
    };
    std::vector<AndOrMarker> andors;
    std::vector<ActionMarker> actions;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      switch (it->tag) {
        case Abc::Symbol::kVar:
        case Abc::Symbol::kFun:
        case Abc::Symbol::kName:
        case Abc::Symbol::kStrippedFun:
        case Abc::Symbol::kStrippedName:
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kEquals:
        case Abc::Symbol::kNotEquals:
        case Abc::Symbol::kStrippedLit:
          if (andors.empty() || andors.back().reset) {
            const Abc::Symbol::Ref last = word_.Insert(it, Abc::Symbol::Or(1));
            scoper.Munch(*last);
            andors.push_back(AndOrMarker(last, scoper.scope()));
          }
          {
            Abc::Symbol::Ref pre = it;
            for (auto sit = actions.rbegin(); sit != actions.rend() && !sit->reset; ++sit) {
              pre = word_.Insert(pre, sit->symbols.begin(), sit->symbols.end());
            }
          }
          if (!nots.empty() && nots.back().neg) {
            switch (it->tag) {
              case Abc::Symbol::kEquals:      it->tag = Abc::Symbol::kNotEquals; break;
              case Abc::Symbol::kNotEquals:   it->tag = Abc::Symbol::kEquals; break;
              case Abc::Symbol::kStrippedLit: it->u.a = it->u.a.flip(); break;
              default:                        assert(false); std::abort();
            }
          }
          scoper.Munch(*it++);
          break;
        case Abc::Symbol::kKnow:
        case Abc::Symbol::kMaybe:
        case Abc::Symbol::kBelieve:
          scoper.Munch(*it++);
          andors.push_back(AndOrMarker(scoper.scope()));
          actions.push_back(ActionMarker(scoper.scope()));
          nots.push_back(NotMarker(scoper.scope()));
          break;
        case Abc::Symbol::kAction: {
          auto first = it;  // we're taking out the kAction symbol plus the action
          auto last = End(std::next(it));
          it = last;
          actions.push_back(ActionMarker(word_.TakeOut(first, last), scoper.scope()));
          break;
        }
        case Abc::Symbol::kNot:
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          it = word_.Erase(it);
          break;
        case Abc::Symbol::kExists:
        case Abc::Symbol::kForall: {
          if (!nots.empty() && nots.back().neg) {
            it->tag = it->tag == Abc::Symbol::kExists ? Abc::Symbol::kForall : Abc::Symbol::kExists;
          }
          scoper.Munch(*it++);
          if (!andors.empty()) {
            const AndOrMarker& aom = andors.back();
            if (!aom.reset) {
              word_.Move(andors.back().ref, std::prev(it));
            } else {
              andors.push_back(AndOrMarker(scoper.scope()));
            }
          }
          break;
        }
        case Abc::Symbol::kOr:
        case Abc::Symbol::kAnd:
          if (!andors.empty() && !andors.back().reset && andors.back().ref->tag == it->tag) {
            andors.back().ref->u.k += it->u.k - 1;
            scoper.Munch(*it);
            it = word_.Erase(it);
          } else {
            andors.push_back(AndOrMarker(it, scoper.scope()));
            scoper.Munch(*it++);
          }
          break;
      }
      while (!andors.empty() && !scoper.active(andors.back().scope)) {
        andors.pop_back();
      }
      while (!actions.empty() && !scoper.active(actions.back().scope)) {
        actions.pop_back();
      }
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
    }
    assert(readable().nnf());
    assert(readable().weakly_well_formed());
  }

  // Replaces existential and universal quantifiers with disjunctions or
  // conjunctions, respectively, over the names.
  template<typename UnaryFunction>
  void Ground(UnaryFunction sort_names) {
    using Container = decltype(std::declval<UnaryFunction>()(Abc::Sort()));
    using Iterator = decltype(std::declval<const Container>().begin());
    struct SubstitutionMarker {
      explicit SubstitutionMarker(Abc::VarSymbol x, Iterator n_begin, Iterator n_end, Scope scope)
          : x(x), n_begin(n_begin), n_end(n_end), scope(scope) {}
      Abc::VarSymbol x{};
      Iterator n_begin{};
      Iterator n_end{};
      Scope scope{};
    };
    std::vector<SubstitutionMarker> markers;
    Abc::DenseMap<Abc::VarSymbol, class Name> map;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ++it) {
      scoper.Munch(*it);
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall) {
        const Abc::VarSymbol x = it->u.x;
        const Container names = sort_names(x.sort());
        const int arity = int(std::distance(names.begin(), names.end()));
        const Abc::Symbol::Ref first = std::next(it);
        Abc::Symbol::Ref before = End(first);
        Abc::Symbol::List subformula = word_.TakeOut(first, before);
        for (int i = arity; i > 0; --i) {
          Abc::Symbol::List formula = subformula;
          const Abc::Symbol::Ref new_before = formula.begin();
          word_.PutIn(before, std::move(formula));
          before = new_before;
        }
        if (arity > 0) {
          markers.push_back(SubstitutionMarker(x, names.begin(), names.end(), scoper.scope()));
          map[x] = *names.begin();
        }
        *it = it->tag == Abc::Symbol::kExists ? Abc::Symbol::Or(arity) : Abc::Symbol::And(arity);
      } else if (it->tag == Abc::Symbol::kVar) {
        const Abc::VarSymbol x = it->u.x;
        if (!map[x].null()) {
          *it = Abc::Symbol::StrippedName(map[x]);
        }
      }
      for (int i = int(markers.size()) - 1; i >= 0 && !scoper.active(markers[i].scope); --i) {
        markers[i].scope = scoper.scope();
        ++markers[i].n_begin;
        if (markers[i].n_begin == markers[i].n_end) {
          markers.pop_back();
        } else {
          map[markers[i].x] = *markers[i].n_begin;
        }
      }
    }
  }

  // Replaces primitive terms, names, and primitive literals with their
  // stripped version, that is, Fun, Name, Lit.
  void Strip() {
    assert(readable().weakly_well_formed());
    struct TermMarker {
      explicit TermMarker(Abc::Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      bool ok = true;
      Abc::Symbol::Ref ref{};
      Scope scope{};
    };
    std::vector<TermMarker> terms;
    Scope::Observer scoper;
    Abc::Symbol::Ref last_eq = end();
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Abc::Symbol::kFun) {
        for (TermMarker& tm : terms) {
          tm.ok &= tm.ref->term();
        }
        terms.push_back(TermMarker(it, scoper.scope()));
      } else if (it->tag == Abc::Symbol::kName) {
        terms.push_back(TermMarker(it, scoper.scope()));
      } else if (it->tag == Abc::Symbol::kVar) {
        for (TermMarker& tm : terms) {
          tm.ok = false;
        }
      } else if (it->tag == Abc::Symbol::kEquals || it->tag == Abc::Symbol::kNotEquals) {
        last_eq = it;
      }
      scoper.Munch(*it++);
      while (!terms.empty() && !scoper.active(terms.back().scope)) {
        const TermMarker& tm = terms.back();
        if (tm.ok) {
          Abc::Word w = Abc::Word(word_.TakeOut(tm.ref, it));
          const Abc::Symbol s = Abc::instance().Strip(std::move(w));
          word_.Insert(it, s);
        }
        terms.pop_back();
      }
      Abc::Symbol::Ref lhs;
      Abc::Symbol::Ref rhs;
      if (last_eq != end() && (lhs = std::next(last_eq)) != end() && (rhs = std::next(lhs)) != end()) {
        assert(last_eq->tag == Abc::Symbol::kEquals || last_eq->tag == Abc::Symbol::kNotEquals);
        if (lhs->stripped() && lhs->term() && rhs->stripped() && rhs->term()) {
          Abc::Symbol s;
          const bool pos = last_eq->tag == Abc::Symbol::kEquals;
          if (lhs->name() && rhs->name()) {
            s = (lhs->u.n_s == rhs->u.n_s) == pos ? Abc::Symbol::And(0) : Abc::Symbol::Or(0);
          } else if (lhs->sort() != rhs->sort()) {
            s = pos ? Abc::Symbol::Or(0) : Abc::Symbol::And(0);
          } else {
            const class Fun f = lhs->fun() ? lhs->u.f_s : rhs->u.f_s;
            const class Name n = rhs->name() ? rhs->u.n_s : lhs->u.n_s;
            s = Abc::Symbol::Lit(pos ? Lit::Eq(f, n) : Lit::Neq(f, n));
          }
          it = word_.Erase(last_eq, std::next(rhs));
          word_.Insert(it, s);
          last_eq = end();
        }
      }
    }
    assert(readable().weakly_well_formed());
  }

  // Replaces subformulas from the inside to the outside.
  // Can be used to implement the Representation Theorem as well as Regression.
  template<typename UnaryPredicate, typename UnaryFunction>
  void Reduce(UnaryPredicate select = UnaryPredicate(), UnaryFunction reduce = UnaryFunction()) {
    assert(readable().weakly_well_formed());
    struct Marker {
      explicit Marker(Abc::Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      Abc::Symbol::Ref ref{};
      Scope scope{};
    };
    std::vector<Marker> markers;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      if (select(*it)) {
        markers.push_back(Marker(it, scoper.scope()));
      }
      scoper.Munch(*it++);
      while (!markers.empty() && !scoper.active(markers.back().scope)) {
        const Abc::Symbol::Ref begin = markers.back().ref;
        Formula f = reduce(RFormula(begin, it));
        it = word_.Erase(begin, it);
        word_.PutIn(it, std::move(f.word_));
        markers.pop_back();
      }
    }
    assert(readable().weakly_well_formed());
  }

 private:
  explicit Formula(Abc::Symbol s)                                                  { word_.Insert(end(), s); }
  explicit Formula(Abc::Symbol s, Formula&& f)                        : Formula(s) { assert(s.arity() == 1); AddArg(f); }
  explicit Formula(Abc::Symbol s, const Formula& f)                   : Formula(s) { assert(s.arity() == 1); AddArg(f); }
  explicit Formula(Abc::Symbol s, Formula&& f, Formula&& g)           : Formula(s) { assert(s.arity() == 2); AddArg(f); AddArg(g); }
  explicit Formula(Abc::Symbol s, const Formula& f, const Formula& g) : Formula(s) { assert(s.arity() == 2); AddArg(f); AddArg(g); }

  template<typename Formulas>
  void AddArgs(Formulas&& args) {
    assert(arity() == args.size());
    for (Formula& f : args) {
      AddArg(std::move(f));
    }
  }

  void AddArg(Formula&& f) {
    assert((tag() == Abc::Symbol::kEquals || tag() == Abc::Symbol::kNotEquals ||
            tag() == Abc::Symbol::kFun || tag() == Abc::Symbol::kName ||
            (tag() == Abc::Symbol::kAction && word_.size() == 1)) == f.head().term());
    assert(tag() != Abc::Symbol::kName || !head().u.n.sort().rigid() || !f.head().u.n.sort().rigid());
    word_.PutIn(end(), std::move(f.word_));
  }

  void AddArg(const Formula& f) {
    assert((tag() == Abc::Symbol::kEquals || tag() == Abc::Symbol::kNotEquals ||
            tag() == Abc::Symbol::kFun || tag() == Abc::Symbol::kName ||
            (tag() == Abc::Symbol::kAction && word_.size() == 1)) == f.head().term());
    assert(tag() != Abc::Symbol::kName || !head().u.n.sort().rigid() || !f.head().u.n.sort().rigid());
    word_.Insert(end(), f.begin(), f.end());
  }

  Abc::Word word_;
};

}  // namespace limbo

#endif  // LIMBO_FORMULA_H_

