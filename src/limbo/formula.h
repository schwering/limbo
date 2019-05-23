// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

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
#include <limbo/internal/singleton.h>

namespace limbo {

class Alphabet : private internal::Singleton<Alphabet> {
 public:
  using id_t = int;

  template<typename T>
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
    explicit operator int() const { return id_; }

    bool null() const { return id_ == 0; }

   private:
    id_t id_;
  };

  class Sort : public IntRepresented<Sort> {
   public:
    using IntRepresented::IntRepresented;
    bool rigid() const { return Instance()->sort_rigid_[*this]; }
  };

  class FunSymbol : public IntRepresented<Sort> {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return Instance()->fun_sort_[*this]; }
    int arity() const { return Instance()->fun_arity_[*this]; }
  };

  class NameSymbol : public IntRepresented<Sort> {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return Instance()->name_sort_[*this]; }
    int arity() const { return Instance()->name_arity_[*this]; }
  };

  class VarSymbol : public IntRepresented<Sort> {
   public:
    using IntRepresented::IntRepresented;
    Sort sort() const { return Instance()->var_sort_[*this]; }
  };

#if 0
  class QuasiName {
   public:
    explicit QuasiName(NameSymbol n) : ground_(true) { u_.n = n; }
    explicit QuasiName(Var v) : ground_(false) { u_.v = v; }
    bool ground() const { return !ground_; }
    NameSymbol name() const { return u_.n; }
    Var var() const { return u_.v; }
   private:
    bool ground_;
    union {
      NameSymbol n;
      Var v;
    } u_;
  };

  class QuasiLit {
    class ConstArgs {
     public:
      explicit ConstArgs(const QuasiLit* owner) : owner_(owner) {}
      std::vector<QuasiName>::const_iterator begin() const { return owner_->ns_.begin(); }
      std::vector<QuasiName>::const_iterator end()   const { return owner_->ns_.end() - 1; }
     private:
      const QuasiLit* owner_;
    };

    class Args {
     public:
      explicit Args(QuasiLit* owner) : owner_(owner) {}
      std::vector<QuasiName>::iterator begin() const { return owner_->ns_.begin(); }
      std::vector<QuasiName>::iterator end()   const { return owner_->ns_.end() - 1; }
     private:
      QuasiLit* owner_;
    };

    template<typename InputIt>
    explicit QuasiLit(FunSymbol f, InputIt args_begin, bool eq, QuasiName n) : eq_(eq), f_(f) {
      int arity = f.arity();
      ns_.resize(arity + 1);
      while (arity-- > 0) {
        ns_.push_back(*args_begin++);
      }
      ns_.push_back(n);
    }
    explicit QuasiLit(QuasiName n1, bool eq, QuasiName n2) : eq_(eq) {
      ns_.resize(2);
      ns_.push_back(n1);
      ns_.push_back(n2);
    }

    bool triv() const { return f_.null(); }
    bool prim() const { return !triv(); }
    bool ground() const { return std::all_of(ns_.begin(), ns_.end(), [](QuasiName n) { return n.ground(); }); }

   private:
    bool eq_;
    FunSymbol f_;
    std::vector<QuasiName> ns_;
  };

  class QuasiClause {
    explicit QuasiClause() = default;
          QuasiLit& operator[](int i)       { return as_[i]; }
    const QuasiLit& operator[](int i) const { return as_[i]; }
   private:
    std::vector<QuasiLit> as_;
  };
#endif

  struct Symbol {
    enum Tag {
      kFun, kName, kVar, kStrippedFun, kStrippedName, kEquals, kNotEquals, kStrippedLit, kStrippedClause,
      kNot, kOr, kAnd, kExists, kForall, kKnow, kMaybe, kBelieve, kAction
    };

    using List = std::list<Symbol>;
    using Ref = List::iterator;
    using CRef = List::const_iterator;

    static Symbol Fun(FunSymbol f)            { Symbol s; s.tag = kFun;            s.u.f = f;            return s; }
    static Symbol Name(NameSymbol n)          { Symbol s; s.tag = kName;           s.u.n = n;            return s; }
    static Symbol Var(VarSymbol x)            { Symbol s; s.tag = kVar;            s.u.x = x;            return s; }
    static Symbol StrippedFun(class Fun f)    { Symbol s; s.tag = kStrippedFun;    s.u.f_s = f;          return s; }
    static Symbol StrippedName(class Name n)  { Symbol s; s.tag = kStrippedName;   s.u.n_s = n;          return s; }
    static Symbol Equals()                    { Symbol s; s.tag = kEquals;                               return s; }
    static Symbol NotEquals()                 { Symbol s; s.tag = kNotEquals;                            return s; }
    static Symbol Lit(Lit a)                  { Symbol s; s.tag = kStrippedLit;    s.u.a = a;            return s; }
    static Symbol Clause(const Clause* c)     { Symbol s; s.tag = kStrippedClause; s.u.c = c;            return s; }
    static Symbol Not()                       { Symbol s; s.tag = kNot;                                  return s; }
    static Symbol Exists(VarSymbol x)         { Symbol s; s.tag = kExists;         s.u.x = x;            return s; }
    static Symbol Forall(VarSymbol x)         { Symbol s; s.tag = kForall;         s.u.x = x;            return s; }
    static Symbol Or(int k)                   { Symbol s; s.tag = kOr;             s.u.k = k;            return s; }
    static Symbol And(int k)                  { Symbol s; s.tag = kAnd;            s.u.k = k;            return s; }
    static Symbol Know(int k)                 { Symbol s; s.tag = kKnow;           s.u.k = k;            return s; }
    static Symbol Maybe(int k)                { Symbol s; s.tag = kMaybe;          s.u.k = k;            return s; }
    static Symbol Believe(int k, int l)       { Symbol s; s.tag = kBelieve;        s.u.k = k; s.u.l = l; return s; }
    static Symbol Action()                    { Symbol s; s.tag = kAction;                               return s; }

    explicit Symbol() = default;

    Symbol(const Symbol&)            = default;
    Symbol& operator=(const Symbol&) = default;
    Symbol(Symbol&&)                 = default;
    Symbol& operator=(Symbol&&)      = default;

    Tag tag;

    union {
      FunSymbol            f;    // kFun
      NameSymbol           n;    // kName
      VarSymbol            x;    // kVar, kExists, kForall
      class Fun            f_s;  // kStrippedFun
      class Name           n_s;  // kStrippedName
      class Lit            a;    // kStrippedLit
      const limbo::Clause* c;    // kStrippedClause
      int                  k;    // kOr, kAnd, kKnow, kMaybe, kBelieve
      int                  l;    // kBelieve
    } u;

    bool operator==(const Symbol& s) const {
      if (tag != s.tag) {
        return false;
      }
      switch (tag) {
        case kFun:             return u.f == s.u.f;
        case kName:            return u.n == s.u.n;
        case kVar:             return u.x == s.u.x;
        case kStrippedFun:     return u.f_s == s.u.f_s;
        case kStrippedName:    return u.n_s == s.u.n_s;
        case kEquals:          return true;
        case kNotEquals:       return true;
        case kStrippedLit:     return u.a == s.u.a;
        case kStrippedClause:  return u.c->operator==(*s.u.c);
        case kNot:             return true;
        case kExists:          return u.x == s.u.x;
        case kForall:          return u.x == s.u.x;
        case kOr:              return u.k == s.u.k;
        case kAnd:             return u.k == s.u.k;
        case kKnow:            return u.k == s.u.k;
        case kMaybe:           return u.k == s.u.k;
        case kBelieve:         return u.k == s.u.k && u.l == s.u.l;
        case kAction:          return true;
      }
      std::abort();
    }

    int arity() const {
      switch (tag) {
        case kFun:             return u.f.arity();
        case kName:            return u.n.arity();
        case kVar:             return 0;
        case kStrippedFun:     return 0;
        case kStrippedName:    return 0;
        case kEquals:          return 2;
        case kNotEquals:       return 2;
        case kStrippedLit:     return 0;
        case kStrippedClause:  return 0;
        case kNot:             return 1;
        case kExists:          return 1;
        case kForall:          return 1;
        case kOr:              return u.k;
        case kAnd:             return u.k;
        case kKnow:            return 1;
        case kMaybe:           return 1;
        case kBelieve:         return 1;
        case kAction:          return 2;
      }
      std::abort();
    }

    bool stripped() const {
      return tag == kStrippedFun || tag == kStrippedName || tag == kStrippedLit || tag == kStrippedClause;
    }
    bool fun()        const { return tag == kFun || tag == kStrippedFun; }
    bool name()       const { return tag == kName || tag == kStrippedName; }
    bool var()        const { return tag == kVar; }
    bool term()       const { return fun() || name() || var(); }
    bool literal()    const { return tag == kEquals || tag == kNotEquals || tag == kStrippedLit; }
    bool quantifier() const { return tag == kExists || tag == kForall; }
    bool objective()  const { return tag != kKnow || tag != kMaybe || tag != kBelieve; }

    Sort sort() const {
      assert(term());
      if (stripped()) {
        const RWord w = Instance()->Unstrip(*this);
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

    Symbol::CRef begin() const { return begin_; }
    Symbol::CRef end() const { return end_; }

   private:
    Symbol::CRef begin_;
    Symbol::CRef end_;
  };

  class Word {
   public:
    explicit Word() = default;
    explicit Word(const RWord& w) : Word(w.begin(), w.end()) {}
    explicit Word(Symbol::List&& w) : symbols_(w) {}

    template<typename InputIt>
    explicit Word(InputIt begin, InputIt end) : symbols_(begin, end) {}

    Word(Word&&)            = default;
    Word& operator=(Word&&) = default;

    Word Clone() const { return Word(*this); }

    RWord readable() const { return RWord(begin(), end()); }

    int size() const { return symbols_.size(); }

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

    Symbol::List symbols_;
  };

  static Alphabet* Instance() {
    if (instance == nullptr) {
      instance = std::unique_ptr<Alphabet>(new Alphabet());
    }
    return instance.get();
  }

  static void ResetInstance() {
    instance = nullptr;
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
        const limbo::Name n = limbo::Name::FromId(++last_name_term_);
        const Symbol s = Symbol::StrippedName(n);
        term_name_symbols_[n] = std::move(w);
        symbols_term_[term_name_symbols_[n].readable()] = s;
        return s;
      }
    }
  }

  RWord Unstrip(const Symbol& s) const {
    assert(s.stripped() && s.term());
    if (s.tag == Symbol::kStrippedFun) {
      return term_fun_symbols_[s.u.f_s].readable();
    } else {
      return term_name_symbols_[s.u.n_s].readable();
    }
  }

 private:
  struct DeepHash {
    internal::hash32_t operator()(const RWord& w) const {
      internal::hash32_t h = 0;
      for (const Symbol& s : w) {
        assert(s.term());
        int i = 0;
        switch (s.tag) {
          case Symbol::kFun:          i = int(s.u.f);   break;
          case Symbol::kName:         i = int(s.u.n);   break;
          case Symbol::kVar:          i = int(s.u.x);   break;
          case Symbol::kStrippedFun:  i = int(s.u.f_s); break;
          case Symbol::kStrippedName: i = int(s.u.n_s); break;
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

  template<typename Key, typename Value>
  using DenseMap = internal::DenseMap<Key, Value, internal::FastAdjustBoundCheck>;
  using TermMap = std::unordered_map<RWord, Symbol, DeepHash, DeepEquals>;

  explicit Alphabet() = default;

  Alphabet(const Alphabet&) = delete;
  Alphabet& operator=(const Alphabet&) = delete;
  Alphabet(Alphabet&&) = delete;
  Alphabet& operator=(Alphabet&&) = delete;

  DenseMap<Sort, bool>       sort_rigid_;
  DenseMap<FunSymbol,  Sort> fun_sort_;
  DenseMap<FunSymbol,  int>  fun_arity_;
  DenseMap<NameSymbol, Sort> name_sort_;
  DenseMap<NameSymbol, int>  name_arity_;
  DenseMap<VarSymbol,  Sort> var_sort_;

  TermMap              symbols_term_;
  DenseMap<Fun,  Word> term_fun_symbols_;
  DenseMap<Name, Word> term_name_symbols_;
  int                  last_sort_      = 0;
  int                  last_var_       = 0;
  int                  last_fun_       = 0;
  int                  last_name_      = 0;
  int                  last_fun_term_  = 0;
  int                  last_name_term_ = 0;

  std::vector<DenseMap<FunSymbol, FunSymbol>> swear_funcs_;
};

class FormulaCommons {
 public:
  using Abc = Alphabet;
  using Symbol = Abc::Symbol;

  class Scope {
   public:
    class Observer {
     public:
      explicit Observer() = default;

      Observer(const Observer&) = delete;
      Observer& operator=(const Observer&) = delete;
      Observer(Observer&&) = delete;
      Observer& operator=(Observer&&) = delete;

      void Munch(const Symbol& s) { k_ += s.arity(); --k_; }
      void Vomit(const Symbol& s) { k_ -= s.arity(); --k_; }
      Scope scope() const { return Scope(k_); }
      bool active(const Scope& scope) const { return k_ >= scope.k_; }
      int left(const Scope& scope) const { return k_ - scope.k_; }

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
};

class RFormula : private FormulaCommons {
 public:
  using RWord = Abc::RWord;
  template<typename T>
  using DenseSet = internal::DenseSet<T, internal::FastAdjustBoundCheck>;

  explicit RFormula(Symbol::CRef begin, Symbol::CRef end) : rword_(begin, end), meta_init_(false) {}
  explicit RFormula(const RWord& w) : RFormula(w.begin(), w.end()) {}
  explicit RFormula(Symbol::CRef begin) : RFormula(begin, End(begin)) {}

  const RWord& rword() const { return rword_; }
  const Symbol& head() const { return *begin(); }
  Symbol::Tag tag() const { return head().tag; }
  int arity() const { return head().arity(); }

  const RFormula& arg(int i) const {
    InitArg(i);
    return args_[i];
  }

  const std::vector<RFormula>& args() const {
    InitArg(arity() - 1);
    return args_;
  }

  Symbol::CRef begin() const { return rword_.begin(); }
  Symbol::CRef end()   const { return rword_.end(); }

  DenseSet<Abc::VarSymbol> FreeVars() const {
    struct QuantifierMarker {
      QuantifierMarker(Abc::VarSymbol x, Scope scope) : x(x), scope(scope) {}
      explicit QuantifierMarker(Scope scope) : scope(scope) {}
      Abc::VarSymbol x;
      Scope scope;
    };
    DenseSet<Abc::VarSymbol> free;
    DenseMap<Abc::VarSymbol, int> bound;
    std::vector<QuantifierMarker> quantifiers;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall) {
        ++bound[it->u.x];
        quantifiers.push_back(QuantifierMarker(it->u.x, scoper.scope()));
      } else if (it->tag == Abc::Symbol::kVar && bound[it->u.x] == 0) {
        free.Insert(it->u.x);
      }
      scoper.Munch(*it++);
      while (!quantifiers.empty() && !scoper.active(quantifiers.back().scope)) {
        --bound[quantifiers.back().x];
        quantifiers.pop_back();
      }
    }
    return free;
  }

  // Strongly well formed means the formula is well formed as defined in the
  // paper, i.e., quasi-primitive literals, quasi-trivial actions (flattened).
  // Weakly well formed admits nested functions.
  bool strongly_well_formed() const { if (!meta_init_) { InitMeta(); } return strongly_well_formed_; }
  bool weakly_well_formed()   const { if (!meta_init_) { InitMeta(); } return weakly_well_formed_; }
  bool nnf()                  const { if (!meta_init_) { InitMeta(); } return nnf_; }
  bool objective()            const { if (!meta_init_) { InitMeta(); } return objective_; }
  bool dynamic()              const { if (!meta_init_) { InitMeta(); } return !static_; }
  bool ground()               const { if (!meta_init_) { InitMeta(); } return !ground_; }

 private:
  template<typename Key, typename Value>
  using DenseMap = internal::DenseMap<Key, Value, internal::FastAdjustBoundCheck>;

  void InitMeta() const { const_cast<RFormula*>(this)->InitMeta(); }

  void InitMeta() {
    enum Expectation { kTerm, kQuasiName, kFormula };
    std::vector<Expectation> es;
    Scope::Observer scoper;
    Scope action_scope;
    bool action_scope_active = false;
    meta_init_ = true;
    strongly_well_formed_ = true;
    weakly_well_formed_ = true;
    nnf_ = true;
    objective_ = true;
    static_ = true;
    ground_ = true;
    for (auto it = begin(); it != end(); ) {
      const Symbol& s = *it;
      if (it != begin()) {
        if (!es.empty()) {
          switch (es.back()) {
            case kTerm:       weakly_well_formed_ &= s.term();  strongly_well_formed_ &= s.term();            break;
            case kQuasiName:  weakly_well_formed_ &= s.term();  strongly_well_formed_ &= s.name() || s.var(); break;
            case kFormula:    weakly_well_formed_ &= !s.term(); strongly_well_formed_ &= !s.term();           break;
          }
          es.pop_back();
        } else {
          weakly_well_formed_ &= !s.term();
          strongly_well_formed_ &= !s.term();
        }
      }
      switch (s.tag) {
        case Symbol::kFun:
        case Symbol::kName:         for (int i = 0; i < s.arity(); ++i) { es.push_back(kQuasiName); } break;
        case Symbol::kVar:
        case Symbol::kStrippedFun:
        case Symbol::kStrippedName:
        case Symbol::kStrippedLit:
        case Symbol::kStrippedClause:       break;
        case Symbol::kEquals:
        case Symbol::kNotEquals:    es.push_back(kQuasiName); es.push_back(kTerm); break;
        case Symbol::kExists:
        case Symbol::kForall:
        case Symbol::kOr:
        case Symbol::kAnd:
        case Symbol::kNot:
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:      for (int i = 0; i < s.arity(); ++i) { es.push_back(kFormula); } break;
        case Symbol::kAction:       es.push_back(kFormula); es.push_back(kQuasiName); break;
      }
      if (s.tag == Symbol::kAction && !action_scope_active) {
        action_scope = scoper.scope();
        action_scope_active = true;
      }
      if (action_scope_active && !scoper.active(action_scope)) {
        action_scope_active = false;
      }
      if (s.tag == Symbol::kNot || (s.tag != Symbol::kAction && !(s.stripped() && !s.term()) && action_scope_active)) {
        nnf_ = false;
      }
      if (!s.objective()) {
        objective_ = false;
      }
      if (s.tag == Symbol::kAction) {
        static_ = false;
      }
      if (s.tag == Symbol::kVar || s.quantifier()) {
        ground_ = false;
      }
      scoper.Munch(*it++);
    }
    if (!es.empty()) {
      weakly_well_formed_ = false;
      strongly_well_formed_ = false;
    }
  }

  void InitArg(int i) const { const_cast<RFormula*>(this)->InitArg(i); }

  void InitArg(int i) {
    args_.reserve(arity());
    for (int j = args_.size(); j <= i; ++j) {
      args_.push_back(RFormula(j > 0 ? args_[j-1].end() : std::next(begin())));
    }
  }

  RWord rword_;
  mutable std::vector<RFormula> args_;
  unsigned meta_init_            : 1;
  unsigned strongly_well_formed_ : 1;
  unsigned weakly_well_formed_   : 1;
  unsigned nnf_                  : 1;
  unsigned objective_            : 1;
  unsigned static_               : 1;
  unsigned ground_               : 1;
};

class Formula : private FormulaCommons {
 public:
  using RWord = Abc::RWord;
  using Word = Abc::Word;

  template<typename Formulas>
  static Formula Fun(Abc::FunSymbol f, Formulas&& args)            { Formula ff(Symbol::Fun(f)); ff.AddArgs(args); return ff; }
  template<typename Formulas>
  static Formula Name(Abc::NameSymbol n, Formulas&& args)          { Formula ff(Symbol::Name(n)); ff.AddArgs(args); return ff; }
  static Formula Var(Abc::VarSymbol x)                             { return Formula(Symbol::Var(x)); }
  static Formula Equals(Formula&& f1, Formula&& f2)                { return Formula(Symbol::Equals(), f1, f2); }
  static Formula Equals(const Formula& f1, const Formula& f2)      { return Formula(Symbol::Equals(), f1, f2); }
  static Formula NotEquals(Formula&& f1, Formula&& f2)             { return Formula(Symbol::NotEquals(), f1, f2); }
  static Formula NotEquals(const Formula& f1, const Formula& f2)   { return Formula(Symbol::NotEquals(), f1, f2); }
  static Formula Not(Formula&& f)                                  { return Formula(Symbol::Not(), f); }
  static Formula Not(const Formula& f)                             { return Formula(Symbol::Not(), f); }
  static Formula Exists(Abc::VarSymbol x, Formula&& f)             { return Formula(Symbol::Exists(x), f); }
  static Formula Exists(Abc::VarSymbol x, const Formula& f)        { return Formula(Symbol::Exists(x), f); }
  static Formula Forall(Abc::VarSymbol x, Formula&& f)             { return Formula(Symbol::Forall(x), f); }
  static Formula Forall(Abc::VarSymbol x, const Formula& f)        { return Formula(Symbol::Forall(x), f); }
  static Formula Or(Formula&& f1, Formula&& f2)                    { return Formula(Symbol::Or(2), f1, f2); }
  static Formula Or(const Formula& f1, const Formula& f2)          { return Formula(Symbol::Or(2), f1, f2); }
  static Formula And(Formula&& f1, Formula&& f2)                   { return Formula(Symbol::And(2), f1, f2); }
  static Formula And(const Formula& f1, const Formula& f2)         { return Formula(Symbol::And(2), f1, f2); }
  static Formula Know(int k, Formula&& f)                          { return Formula(Symbol::Know(k), f); }
  static Formula Know(int k, const Formula& f)                     { return Formula(Symbol::Know(k), f); }
  static Formula Maybe(int k, Formula&& f)                         { return Formula(Symbol::Maybe(k), f); }
  static Formula Maybe(int k, const Formula& f)                    { return Formula(Symbol::Maybe(k), f); }
  static Formula Believe(int k, int l, Formula&& f1, Formula&& f2) { return Formula(Symbol::Believe(k, l), f1, f2); }
  static Formula Believe(int k, int l, const Formula& f1, const Formula& f2)
                                                                   { return Formula(Symbol::Believe(k, l), f1, f2); }
  static Formula Action(Formula&& f1, Formula&& f2)                { return Formula(Symbol::Action(), f1, f2); }
  static Formula Action(const Formula& f1, const Formula& f2)      { return Formula(Symbol::Action(), f1, f2); }

  explicit Formula(Word&& w) : word_(std::move(w)) {}
  explicit Formula(const RFormula& f) : word_(f.rword()) {}

  Formula(const Formula&)            = delete;
  Formula& operator=(const Formula&) = delete;
  Formula(Formula&&)                 = default;
  Formula& operator=(Formula&&)      = default;

  Formula Clone() { return Formula(word_.Clone()); }

  RFormula readable() const { return RFormula(begin(), end()); }
  const Word& word() const { return word_; }
  const Symbol& head() const { return *begin(); }
  Symbol::Tag tag() const { return head().tag; }
  int arity() const { return head().arity(); }

  Symbol::Ref begin() { return word_.begin(); }
  Symbol::Ref end()   { return word_.end(); }

  Symbol::CRef begin() const { return word_.begin(); }
  Symbol::CRef end()   const { return word_.end(); }

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
      Abc::VarSymbol x;
      Scope scope;
    };
    struct NotMarker {
      explicit NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      explicit NotMarker(Scope scope) : neg(false), scope(scope) {}
      unsigned neg : 1;
      Scope scope;
    };
    std::vector<ForallMarker> foralls;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      const Symbol s = *it;
      const bool pos = nots.empty() || !nots.back().neg;
      switch (s.tag) {
        case Symbol::kFun:
        case Symbol::kName:
        case Symbol::kVar:
        case Symbol::kStrippedFun:
        case Symbol::kStrippedName:
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kStrippedLit:
        case Symbol::kStrippedClause:
        case Symbol::kOr:
        case Symbol::kAnd:
        case Symbol::kAction:
          scoper.Munch(*it++);
          break;
        case Symbol::kExists:
        case Symbol::kForall:
          if ((s.tag == Symbol::kExists) == pos) {
            Symbol::List symbols;
            for (const ForallMarker& fm : foralls) {
              symbols.push_back(Symbol::Var(fm.x));
            }
            const Abc::FunSymbol f = Abc::Instance()->CreateFun(s.u.x.sort(), symbols.size());
            symbols.push_front(Symbol::Fun(f));
            symbols.push_back(Symbol::Var(s.u.x));
            symbols.push_front(pos ? Symbol::NotEquals() : Symbol::Equals());
            symbols.push_front(pos ? Symbol::Or(2) : Symbol::And(2));
            it->tag = pos ? Symbol::kForall : Symbol::kExists;
            word_.PutIn(std::next(it), std::move(symbols));
            scoper.Munch(*it++);
          } else {
            scoper.Munch(*it++);
            foralls.push_back(ForallMarker(s.u.x, scoper.scope()));
          }
          break;
        case Symbol::kNot:
          scoper.Munch(*it++);
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          break;
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
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
      explicit ActionMarker(Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      Symbol::List symbols;
      Scope scope;
    };
    std::vector<ActionMarker> actions;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      switch (it->tag) {
        case Symbol::kFun:
          scoper.Munch(*it);
          it->u.f = Abc::Instance()->Squaring(actions.size(), it->u.f);
          ++it;
          for (const ActionMarker& am : actions) {
            word_.Insert(it, am.symbols.begin(), am.symbols.end());
          }
          break;
        case Symbol::kVar:
        case Symbol::kName:
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kStrippedLit:
        case Symbol::kNot:
        case Symbol::kExists:
        case Symbol::kForall:
        case Symbol::kOr:
        case Symbol::kAnd:
          scoper.Munch(*it++);
          break;
        case Symbol::kAction: {
          it = word_.Erase(it);
          auto first = it;
          auto last = End(it);
          it = last;
          actions.push_back(ActionMarker(word_.TakeOut(first, last), scoper.scope()));
          break;
        }
        case Symbol::kStrippedFun:
        case Symbol::kStrippedName:
        case Symbol::kStrippedClause:
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
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
      Abc::VarSymbol x;
      Scope scope;
    };
    struct NewVars {
      explicit NewVars() = default;
      bool used = false;
      std::vector<NewVar> vars;
    };
    DenseMap<Abc::VarSymbol, NewVars> vars;
    Scope::Observer scoper;
    for (Symbol& s : *this) {
      if (s.tag == Symbol::kExists || s.tag == Symbol::kForall) {
        const Abc::VarSymbol y = !vars[s.u.x].used && !all_new ? s.u.x : Abc::Instance()->CreateVar(s.u.x.sort());
        vars[s.u.x].used = true;
        vars[s.u.x].vars.push_back(NewVar(y, scoper.scope()));
        s.u.x = y;
      } else if (s.tag == Symbol::kVar && !vars[s.u.x].vars.empty()) {
        s.u.x = vars[s.u.x].vars.back().x;
      }
      scoper.Munch(s);
      for (auto& nvs : vars) {
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
      unsigned reset : 1;
      unsigned neg   : 1;
      Scope scope;
    };
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    Symbol::Ref or_ref = end();
    bool tolerate_fun = false;
    for (auto it = begin(); it != end(); ) {
      const bool pos = nots.empty() || !nots.back().neg;
      switch (it->tag) {
        case Symbol::kFun:
          if (!tolerate_fun) {
            const Abc::VarSymbol x = Abc::Instance()->CreateVar(it->u.f.sort());
            it = word_.Insert(it, Symbol::Var(x));
            const Symbol::Ref first = std::next(it);
            const Symbol::Ref last = End(first);
            Symbol::List symbols = word_.TakeOut(first, last);
            symbols.push_back(Symbol::Var(x));
            symbols.push_front(pos ? Symbol::NotEquals() : Symbol::Equals());
            auto bt = word_.Insert(or_ref, pos ? Symbol::Forall(x) : Symbol::Exists(x));
            while (--it != bt) {
              scoper.Vomit(*it);
            }
            ++(or_ref->u.k);
            word_.PutIn(std::next(or_ref), std::move(symbols));
          }
        case Symbol::kName:
        case Symbol::kVar:
          scoper.Munch(*it++);
          tolerate_fun = false;
          break;
        case Symbol::kEquals:
        case Symbol::kNotEquals:
          or_ref = word_.Insert(it, pos ? Symbol::Or(1) : Symbol::And(1));
          scoper.Munch(*or_ref);
          scoper.Munch(*it++);
          tolerate_fun = true;
          break;
        case Symbol::kAction:
          or_ref = word_.Insert(it, pos ? Symbol::Or(1) : Symbol::And(1));
          scoper.Munch(*or_ref);
          scoper.Munch(*it++);
          break;
        case Symbol::kOr:
        case Symbol::kAnd:
        case Symbol::kStrippedFun:
        case Symbol::kStrippedName:
        case Symbol::kStrippedLit:
        case Symbol::kStrippedClause:
          scoper.Munch(*it++);
          break;
        case Symbol::kNot:
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          scoper.Munch(*it++);
          break;
        case Symbol::kExists:
        case Symbol::kForall:
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
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
      explicit AndOrMarker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      explicit AndOrMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      Symbol::Ref ref;
      Scope scope;
    };
    struct ActionMarker {
      explicit ActionMarker(Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      explicit ActionMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      Symbol::List symbols;
      Scope scope;
    };
    struct NotMarker {
      explicit NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      explicit NotMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      unsigned neg   : 1;
      Scope scope;
    };
    std::vector<AndOrMarker> andors;
    std::vector<ActionMarker> actions;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      switch (it->tag) {
        case Symbol::kVar:
        case Symbol::kFun:
        case Symbol::kName:
        case Symbol::kStrippedFun:
        case Symbol::kStrippedName:
          scoper.Munch(*it++);
          break;
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kStrippedLit:
          if (andors.empty() || andors.back().reset) {
            const Symbol::Ref last = word_.Insert(it, Symbol::Or(1));
            scoper.Munch(*last);
            andors.push_back(AndOrMarker(last, scoper.scope()));
          }
          {
            Symbol::Ref pre = it;
            for (auto sit = actions.rbegin(); sit != actions.rend() && !sit->reset; ++sit) {
              pre = word_.Insert(pre, sit->symbols.begin(), sit->symbols.end());
            }
          }
          if (!nots.empty() && nots.back().neg) {
            switch (it->tag) {
              case Symbol::kEquals:      it->tag = Symbol::kNotEquals; break;
              case Symbol::kNotEquals:   it->tag = Symbol::kEquals; break;
              case Symbol::kStrippedLit: it->u.a = it->u.a.flip(); break;
              default:                   assert(false); std::abort();
            }
          }
          scoper.Munch(*it++);
          break;
        case Symbol::kStrippedClause:
          assert(false);
          std::abort();
          break;
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
          scoper.Munch(*it++);
          andors.push_back(AndOrMarker(scoper.scope()));
          actions.push_back(ActionMarker(scoper.scope()));
          nots.push_back(NotMarker(scoper.scope()));
          break;
        case Symbol::kAction: {
          auto first = it;  // we're taking out the kAction symbol plus the action
          auto last = End(std::next(it));
          it = last;
          actions.push_back(ActionMarker(word_.TakeOut(first, last), scoper.scope()));
          break;
        }
        case Symbol::kNot:
          nots.push_back(NotMarker(nots.empty() || !nots.back().neg, scoper.scope()));
          it = word_.Erase(it);
          break;
        case Symbol::kExists:
        case Symbol::kForall: {
          if (!nots.empty() && nots.back().neg) {
            it->tag = it->tag == Symbol::kExists ? Symbol::kForall : Symbol::kExists;
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
        case Symbol::kOr:
        case Symbol::kAnd:
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

  // Replaces primitive terms, names, and primitive literals with their
  // stripped version, that is, Fun, Name, Lit.
  void Strip() {
    assert(readable().weakly_well_formed());
    struct TermMarker {
      explicit TermMarker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      bool ok = true;
      Symbol::Ref ref;
      Scope scope;
    };
    std::vector<TermMarker> terms;
    Scope::Observer scoper;
    Symbol::Ref last_eq = end();
    for (auto it = begin(); it != end(); ) {
      if (it->tag == Symbol::kFun) {
        for (TermMarker& tm : terms) {
          tm.ok &= tm.ref->term();
        }
        terms.push_back(TermMarker(it, scoper.scope()));
      } else if (it->tag == Symbol::kName) {
        terms.push_back(TermMarker(it, scoper.scope()));
      } else if (it->tag == Symbol::kVar) {
        for (TermMarker& tm : terms) {
          tm.ok = false;
        }
      } else if (it->tag == Symbol::kEquals || it->tag == Symbol::kNotEquals) {
        last_eq = it;
      }
      scoper.Munch(*it++);
      while (!terms.empty() && !scoper.active(terms.back().scope)) {
        const TermMarker& tm = terms.back();
        if (tm.ok) {
          Word w = Word(word_.TakeOut(tm.ref, it));
          const Symbol s = Abc::Instance()->Strip(std::move(w));
          word_.Insert(it, s);
        }
        terms.pop_back();
      }
      Symbol::Ref lhs;
      Symbol::Ref rhs;
      if (last_eq != end() && (lhs = std::next(last_eq)) != end() && (rhs = std::next(lhs)) != end()) {
        assert(last_eq->tag == Symbol::kEquals || last_eq->tag == Symbol::kNotEquals);
        if (lhs->stripped() && lhs->term() && rhs->stripped() && rhs->term() && (lhs->name() != rhs->name())) {
          Symbol s;
          if (lhs->name() && rhs->name()) {
            s = (lhs->u.n_s == rhs->u.n_s) == (last_eq->tag == Symbol::kEquals) ? Symbol::And(0) : Symbol::Or(0);
          } else if (lhs->sort() != rhs->sort()) {
            s = last_eq->tag == Symbol::kEquals ? Symbol::Or(0) : Symbol::And(0);
          } else {
            bool pos = last_eq->tag == Symbol::kEquals;
            const class Fun f = lhs->fun() ? lhs->u.f_s : rhs->u.f_s;
            const class Name n = rhs->name() ? rhs->u.n_s : lhs->u.n_s;
            s = Symbol::Lit(Lit(pos, f, n));
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
      explicit Marker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      Symbol::Ref ref;
      Scope scope;
    };
    std::vector<Marker> markers;
    Scope::Observer scoper;
    for (auto it = begin(); it != end(); ) {
      if (select(*it)) {
        markers.push_back(Marker(it, scoper.scope()));
      }
      scoper.Munch(*it++);
      while (!markers.empty() && !scoper.active(markers.back().scope)) {
        const Symbol::Ref begin = markers.back().ref;
        Formula f = reduce(RFormula(begin, it));
        it = word_.Erase(begin, it);
        word_.PutIn(it, std::move(f.word_));
        markers.pop_back();
      }
    }
    assert(readable().weakly_well_formed());
  }

 private:
  template<typename Key, typename Value>
  using DenseMap = internal::DenseMap<Key, Value, internal::FastAdjustBoundCheck>;

  explicit Formula(Symbol s)                                                  { word_.Insert(end(), s); }
  explicit Formula(Symbol s, Formula&& f)                        : Formula(s) { assert(s.arity() == 1); AddArg(f); }
  explicit Formula(Symbol s, const Formula& f)                   : Formula(s) { assert(s.arity() == 1); AddArg(f); }
  explicit Formula(Symbol s, Formula&& f, Formula&& g)           : Formula(s) { assert(s.arity() == 2); AddArg(f); AddArg(g); }
  explicit Formula(Symbol s, const Formula& f, const Formula& g) : Formula(s) { assert(s.arity() == 2); AddArg(f); AddArg(g); }

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
    assert(tag() != Symbol::kName || !head().u.n.sort().rigid() || !f.head().u.n.sort().rigid());
    word_.PutIn(end(), std::move(f.word_));
  }

  void AddArg(const Formula& f) {
    assert((tag() == Abc::Symbol::kEquals || tag() == Abc::Symbol::kNotEquals ||
            tag() == Abc::Symbol::kFun || tag() == Abc::Symbol::kName ||
            (tag() == Abc::Symbol::kAction && word_.size() == 1)) == f.head().term());
    assert(tag() != Symbol::kName || !head().u.n.sort().rigid() || !f.head().u.n.sort().rigid());
    word_.Insert(end(), f.begin(), f.end());
  }

  Word word_;
};

}  // namespace limbo

#endif  // LIMBO_FORMULA_H_

