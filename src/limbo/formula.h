// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_FORMULA_H_
#define LIMBO_FORMULA_H_

#include <cassert>

#include <algorithm>
#include <list>
#include <unordered_map>

#include <limbo/clause.h>
#include <limbo/literal.h>

#include <limbo/internal/dense.h>
#include <limbo/internal/hash.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/singleton.h>

namespace limbo {

class Alphabet : private internal::Singleton<Alphabet> {
 public:
  using id_t = int;

  template<typename T>
  class Identifiable {
   public:
    Identifiable() = default;
    explicit Identifiable(id_t id) : id_(id) {}
    bool operator==(const Identifiable& i) const { return id_ == i.id_; }
    bool operator!=(const Identifiable& i) const { return !(*this == i); }
    bool null() const { return id_ == 0; }
    int index() const { return id_; }
   private:
    id_t id_;
  };

  class Sort : public Identifiable<Sort> {
   public:
    using Identifiable::Identifiable;
    bool rigid() const { return Instance()->sort_rigid_[*this]; }
  };

  class Fun : public Identifiable<Sort> {
   public:
    using Identifiable::Identifiable;
    Sort sort() const { return Instance()->fun_sort_[*this]; }
    int arity() const { return Instance()->fun_arity_[*this]; }
  };

  class Name : public Identifiable<Sort> {
   public:
    using Identifiable::Identifiable;
    Sort sort() const { return Instance()->name_sort_[*this]; }
    int arity() const { return Instance()->name_arity_[*this]; }
  };

  class Var : public Identifiable<Sort> {
   public:
    using Identifiable::Identifiable;
    Sort sort() const { return Instance()->var_sort_[*this]; }
  };

#if 0
  class QuasiName {
   public:
    QuasiName(Name n) : ground_(true) { u_.n = n; }
    QuasiName(Var v) : ground_(false) { u_.v = v; }
    bool ground() const { return !ground_; }
    Name name() const { return u_.n; }
    Var var() const { return u_.v; }
   private:
    bool ground_;
    union {
      Name n;
      Var v;
    } u_;
  };

  class QuasiLiteral {
    class ConstArgs {
     public:
      ConstArgs(const QuasiLiteral* owner) : owner_(owner) {}
      std::vector<QuasiName>::const_iterator begin() const { return owner_->ns_.begin(); }
      std::vector<QuasiName>::const_iterator end()   const { return owner_->ns_.end() - 1; }
     private:
      const QuasiLiteral* owner_;
    };

    class Args {
     public:
      Args(QuasiLiteral* owner) : owner_(owner) {}
      std::vector<QuasiName>::iterator begin() const { return owner_->ns_.begin(); }
      std::vector<QuasiName>::iterator end()   const { return owner_->ns_.end() - 1; }
     private:
      QuasiLiteral* owner_;
    };

    template<typename InputIt>
    QuasiLiteral(Fun f, InputIt args_begin, bool eq, QuasiName n) : eq_(eq), f_(f) {
      int arity = f.arity();
      ns_.resize(arity + 1);
      while (arity-- > 0) {
        ns_.push_back(*args_begin++);
      }
      ns_.push_back(n);
    }
    QuasiLiteral(QuasiName n1, bool eq, QuasiName n2) : eq_(eq) {
      ns_.resize(2);
      ns_.push_back(n1);
      ns_.push_back(n2);
    }

    bool triv() const { return f_.null(); }
    bool prim() const { return !triv(); }
    bool ground() const { return std::all_of(ns_.begin(), ns_.end(), [](QuasiName n) { return n.ground(); }); }

   private:
    bool eq_;
    Fun f_;
    std::vector<QuasiName> ns_;
  };

  class QuasiClause {
    QuasiClause() = default;
          QuasiLiteral& operator[](int i)       { return as_[i]; }
    const QuasiLiteral& operator[](int i) const { return as_[i]; }
   private:
    std::vector<QuasiLiteral> as_;
  };
#endif

  struct Symbol {
    enum Tag {
      kFun, kName, kVar, kFunTerm, kNameTerm, kEquals, kNotEquals, kLiteral, kClause,
      kNot, kOr, kAnd, kExists, kForall, kKnow, kMaybe, kBelieve, kAction
    };

    using List = std::list<Symbol>;
    using Ref = List::iterator;
    using CRef = List::const_iterator;

    static Symbol Fun(Fun f)              { Symbol s; s.tag = kFun;        s.u.f = f;            return s; }
    static Symbol Name(Name n)            { Symbol s; s.tag = kName;       s.u.n = n;            return s; }
    static Symbol Var(Var x)              { Symbol s; s.tag = kVar;        s.u.x = x;            return s; }
    static Symbol FunTerm(limbo::Fun f)   { Symbol s; s.tag = kFunTerm;    s.u.ft = f;           return s; }
    static Symbol NameTerm(limbo::Name n) { Symbol s; s.tag = kNameTerm;   s.u.nt = n;           return s; }
    static Symbol Equals()                { Symbol s; s.tag = kEquals;                           return s; }
    static Symbol NotEquals()             { Symbol s; s.tag = kNotEquals;                        return s; }
    static Symbol Literal(Literal a)      { Symbol s; s.tag = kLiteral;    s.u.a = a;            return s; }
    static Symbol Clause(const Clause* c) { Symbol s; s.tag = kClause;     s.u.c = c;            return s; }
    static Symbol Not()                   { Symbol s; s.tag = kNot;                              return s; }
    static Symbol Exists(class Var x)     { Symbol s; s.tag = kExists;     s.u.x = x;            return s; }
    static Symbol Forall(class Var x)     { Symbol s; s.tag = kForall;     s.u.x = x;            return s; }
    static Symbol Or(int k)               { Symbol s; s.tag = kOr;         s.u.k = k;            return s; }
    static Symbol And(int k)              { Symbol s; s.tag = kAnd;        s.u.k = k;            return s; }
    static Symbol Know(int k)             { Symbol s; s.tag = kKnow;       s.u.k = k;            return s; }
    static Symbol Maybe(int k)            { Symbol s; s.tag = kMaybe;      s.u.k = k;            return s; }
    static Symbol Believe(int k, int l)   { Symbol s; s.tag = kBelieve;    s.u.k = k; s.u.l = l; return s; }
    static Symbol Action()                { Symbol s; s.tag = kAction;                           return s; }

    Symbol() = default;

    Tag tag;

    union {
      class Fun            f;   // kFun
      class Name           n;   // kName
      class Var            x;   // kVar, kExists, kForall
      limbo::Fun           ft;  // kFunTerm
      limbo::Name          nt;  // kNameTerm
      limbo::Literal       a;   // kLiteral
      const limbo::Clause* c;   // kClause
      int                  k;   // kOr, kAnd, kKnow, kMaybe, kBelieve
      int                  l;   // kBelieve
    } u;

    bool operator==(const Symbol& s) const {
      if (tag != s.tag) {
        return false;
      }
      switch (tag) {
        case kFun:       return u.f == s.u.f;
        case kName:      return u.n == s.u.n;
        case kVar:       return u.x == s.u.x;
        case kFunTerm:   return u.ft == s.u.ft;
        case kNameTerm:  return u.nt == s.u.nt;
        case kEquals:    return true;
        case kNotEquals: return true;
        case kLiteral:   return u.a == s.u.a;
        case kClause:    return u.c->operator==(*s.u.c);
        case kNot:       return true;
        case kExists:    return u.x == s.u.x;
        case kForall:    return u.x == s.u.x;
        case kOr:        return u.k == s.u.k;
        case kAnd:       return u.k == s.u.k;
        case kKnow:      return u.k == s.u.k;
        case kMaybe:     return u.k == s.u.k;
        case kBelieve:   return u.k == s.u.k && u.l == s.u.l;
        case kAction:    return true;
      }
    }

    int arity() const {
      switch (tag) {
        case kFun:       return u.f.arity();
        case kName:      return u.n.arity();
        case kVar:       return 0;
        case kFunTerm:   return 0;
        case kNameTerm:  return 0;
        case kEquals:    return 2;
        case kNotEquals: return 2;
        case kLiteral:   return 0;
        case kClause:    return 0;
        case kNot:       return 1;
        case kExists:    return 1;
        case kForall:    return 1;
        case kOr:        return u.k;
        case kAnd:       return u.k;
        case kKnow:      return 1;
        case kMaybe:     return 1;
        case kBelieve:   return 1;
        case kAction:    return 2;
      }
    }
  };

  class RWord {
   public:
    RWord() = default;
    RWord(Symbol::CRef begin, Symbol::CRef end) : begin_(begin), end_(end) {}
    Symbol::CRef begin() const { return begin_; }
    Symbol::CRef end() const { return end_; }
   private:
    Symbol::CRef begin_;
    Symbol::CRef end_;
  };

  class Word;

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
    const Sort s = Sort(++last_sort_);
    sort_rigid_.Capacitate(s, [](auto i) { return internal::next_power_of_two(i) + 1; });
    sort_rigid_[s] = rigid;
    return s;
  }

  Fun CreateFun(Sort s, int arity) {
    assert(!s.rigid());
    if (s.rigid()) {
      std::abort();
    }
    const Fun f = Fun(++last_fun_);
    fun_sort_.Capacitate(f, [](auto i) { return internal::next_power_of_two(i) + 1; });
    fun_sort_[f] = s;
    fun_arity_.Capacitate(f, [](auto i) { return internal::next_power_of_two(i) + 1; });
    fun_arity_[f] = arity;
    return f;
  }

  Name CreateName(Sort s, int arity = 0) {
    assert(s.rigid() || arity == 0);
    if (!s.rigid() && arity > 0) {
      std::abort();
    }
    const Name n = Name(++last_name_);
    name_sort_.Capacitate(n, [](auto i) { return internal::next_power_of_two(i) + 1; });
    name_sort_[n] = s;
    name_arity_.Capacitate(n, [](auto i) { return internal::next_power_of_two(i) + 1; });
    name_arity_[n] = arity;
    return n;
  }

  Var CreateVar(Sort s) {
    const Var x = Var(++last_var_);
    var_sort_.Capacitate(x, [](auto i) { return internal::next_power_of_two(i) + 1; });
    var_sort_[x] = s;
    return x;
  }

  Fun Squaring(const int sitlen, const Fun f) {
    if (sitlen == 0) {
      return f;
    } else {
      const int k = sitlen - 1;
      swear_funcs_.reserve(internal::next_power_of_two(sitlen));
      swear_funcs_.resize(sitlen);
      internal::DenseMap<Fun, Fun>& map = swear_funcs_[k];
      map.Capacitate(f, [](auto i) { return internal::next_power_of_two(i) + 1; });
      if (map[f].null()) {
        map[f] = CreateFun(f.sort(), sitlen + f.arity());
      }
      return map[f];
    }
  }

  Symbol Termify(const RWord& w) {
    assert((w.begin()->tag == Symbol::kFun || w.begin()->tag == Symbol::kName));
    auto it = symbols_term_.find(w);
    if (it != symbols_term_.end()) {
      return it->second;
    } else {
      const bool is_fun = w.begin()->tag == Symbol::kFun;
      if (is_fun) {
        const limbo::Fun f = limbo::Fun(++last_fun_term_);
        const Symbol s = Symbol::FunTerm(f);
        symbols_term_[w] = s;
        term_fun_symbols_[f] = w;
        return s;
      } else {
        const limbo::Name n = limbo::Name(++last_fun_term_);
        const Symbol s = Symbol::NameTerm(n);
        symbols_term_[w] = s;
        term_name_symbols_[n] = w;
        return s;
      }
    }
  }

 private:
  struct DeepHash {
    internal::hash32_t operator()(const RWord& sr) const {
      internal::hash32_t h = 0;
      for (const Symbol& s : sr) {
        int i = 0;
        switch (s.tag) {
          case Symbol::kFun:      i = s.u.f.index();  break;
          case Symbol::kName:     i = s.u.n.index();  break;
          case Symbol::kVar:      i = s.u.x.index();  break;
          case Symbol::kFunTerm:  i = s.u.ft.index(); break;
          case Symbol::kNameTerm: i = s.u.nt.index(); break;
          default:                                    break;
        }
        h ^= internal::jenkins_hash(s.tag);
        h ^= internal::jenkins_hash(i);
      }
      return h;
    }
  };

  struct DeepEquals {
    bool operator()(const RWord& sr1, const RWord& sr2) const {
      return std::equal(sr1.begin(), sr1.end(), sr2.begin());
    }
  };

  using TermMap = std::unordered_map<RWord, Symbol, DeepHash, DeepEquals>;

  Alphabet() = default;
  Alphabet(const Alphabet&) = delete;
  Alphabet(Alphabet&&) = delete;
  Alphabet& operator=(const Alphabet&) = delete;
  Alphabet& operator=(Alphabet&&) = delete;

  internal::DenseMap<Sort, bool> sort_rigid_;
  internal::DenseMap<Fun, Sort>  fun_sort_;
  internal::DenseMap<Fun, int>   fun_arity_;
  internal::DenseMap<Name, Sort> name_sort_;
  internal::DenseMap<Name, int>  name_arity_;
  internal::DenseMap<Var, Sort>  var_sort_;

  internal::DenseMap<limbo::Fun, RWord> term_fun_symbols_;
  internal::DenseMap<limbo::Name, RWord> term_name_symbols_;
  int last_sort_ = 0;
  int last_var_ = 0;
  int last_fun_ = 0;
  int last_name_ = 0;
  int last_fun_term_ = 0;
  int last_name_term_ = 0;

  TermMap symbols_term_;

  std::vector<internal::DenseMap<Fun, Fun>> swear_funcs_;
};

class Alphabet::Word {
 public:
  Word() = default;
  Word(const RWord& w) : Word(w.begin(), w.end()) {}

  template<typename InputIt>
  Word(InputIt begin, InputIt end) : symbols_(begin, end) {}

  Word(const Word&) = default;
  Word& operator=(const Word&) = default;
  Word(Word&&) = default;
  Word& operator=(Word&&) = default;

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
  Symbol::List symbols_;
};

class CommonFormula {
 public:
  using Abc = Alphabet;
  using Symbol = Abc::Symbol;

  class Scope {
   public:
    class Observer {
     public:
      Observer() = default;
      void Munch(const Symbol& s) { k_ += s.arity(); --k_; }
      void Vomit(const Symbol& s) { k_ -= s.arity(); --k_; }
      Scope scope() const { return Scope(k_); }
      bool active(const Scope& scope) const { return k_ >= scope.k_; }
      int left(const Scope& scope) const { return k_ - scope.k_; }

     private:
      Observer(const Observer&) = delete;
      Observer& operator=(const Observer&) = delete;
      Observer(Observer&&) = delete;
      Observer& operator=(Observer&&) = delete;

      int k_ = 0;
    };

    Scope() = default;
    explicit Scope(int k) : k_(k) {}

   private:
    int k_;
  };

  template<typename InputIt>
  static InputIt End(InputIt it) {
    int n = 0;
    Scope::Observer scoper;
    Scope scope = scoper.scope();
    while (scoper.active(scope)) {
      ++n;
      scoper.Munch(*it++);
    }
    return std::make_pair(it, n);
  }
};

class RFormula : private CommonFormula {
 public:
  using RWord = Abc::RWord;

  explicit RFormula(Symbol::CRef begin) : RFormula(begin, End(begin)) {}
  RFormula(Symbol::CRef begin, Symbol::CRef end) : rword_(begin, end) {}

  const RWord& rword() const { return rword_; }
  const Symbol& head() const { return *begin(); }
  Symbol::Tag tag() const { return begin()->tag; }
  int arity() const { return begin()->arity(); }

  RFormula arg(int i) const {
    args_.reserve(arity());
    for (int j = args_.size(); j <= i; ++j) {
      args_.push_back(RFormula(j > 0 ? args_[j-1].end() : std::next(begin())));
    }
    return args_[i];
  }

  Symbol::CRef begin() const { return rword_.begin(); }
  Symbol::CRef end()   const { return rword_.end(); }

 private:
  RWord rword_;
  mutable std::vector<RFormula> args_;
};

class Formula : private CommonFormula {
 public:
  using Word = Abc::Word;

  template<typename Formulas>
  static Formula Fun(Abc::Fun f, Formulas&& args) { return Formula(Symbol::Fun(f)).WithArgs(args); }
  template<typename Formulas>
  static Formula Name(Abc::Name n, Formulas&& args) { return Formula(Symbol::Name(n)).WithArgs(args); }
  static Formula Var(Abc::Var x)                                   { return Formula(Symbol::Var(x)); }
  static Formula Equals(Formula&& f1, Formula&& f2)                { return Formula(Symbol::Equals(), f1, f2); }
  static Formula Equals(const Formula& f1, const Formula& f2)      { return Formula(Symbol::Equals(), f1, f2); }
  static Formula NotEquals(Formula&& f1, Formula&& f2)             { return Formula(Symbol::NotEquals(), f1, f2); }
  static Formula NotEquals(const Formula& f1, const Formula& f2)   { return Formula(Symbol::NotEquals(), f1, f2); }
  static Formula Not(Formula&& f)                                  { return Formula(Symbol::Not(), f); }
  static Formula Not(const Formula& f)                             { return Formula(Symbol::Not(), f); }
  static Formula Exists(Abc::Var x, Formula&& f)                   { return Formula(Symbol::Exists(x), f); }
  static Formula Exists(Abc::Var x, const Formula& f)              { return Formula(Symbol::Exists(x), f); }
  static Formula Forall(Abc::Var x, Formula&& f)                   { return Formula(Symbol::Forall(x), f); }
  static Formula Forall(Abc::Var x, const Formula& f)              { return Formula(Symbol::Forall(x), f); }
  static Formula Or(Formula&& f1, Formula&& f2)                    { return Formula(Symbol::Or(2), f1, f2); }
  static Formula Or(const Formula& f1, const Formula& f2)          { return Formula(Symbol::Or(2), f1, f2); }
  static Formula And(Formula&& f1, Formula&& f2)                   { return Formula(Symbol::And(2), f1, f2); }
  static Formula And(const Formula& f1, const Formula& f2)         { return Formula(Symbol::And(2), f1, f2); }
  static Formula Know(int k, Formula&& f)                          { return Formula(Symbol::Know(k), f); }
  static Formula Know(int k, const Formula& f)                     { return Formula(Symbol::Know(k), f); }
  static Formula Maybe(int k, Formula&& f)                         { return Formula(Symbol::Maybe(k), f); }
  static Formula Maybe(int k, const Formula& f)                    { return Formula(Symbol::Maybe(k), f); }
  static Formula Believe(int k, int l, Formula&& f1, Formula&& f2) { return Formula(Symbol::Believe(k, l), f1, f2); }
  static Formula Believe(int k, int l, const Formula& f1, const Formula& f2) { return Formula(Symbol::Believe(k, l), f1, f2); }
  static Formula Action(Formula&& f1, Formula&& f2)                { return Formula(Symbol::Action(), f1, f2); }
  static Formula Action(const Formula& f1, const Formula& f2)      { return Formula(Symbol::Action(), f1, f2); }

  explicit Formula(Word&& w) : word_(std::move(w)) {}
  explicit Formula(const RFormula& f) : word_(f.rword()) {}

  RFormula readable() const { return RFormula(word_.begin()); }
  const Word& word() const { return word_; }

  void Normalize() {
    Rectify();
    Flatten();
    PushInwards();
  }

  void Skolemize() {
    // Because of actions, we can't do normal Skolemization. Instead, we replace
    // the formula ex x phi with fa x (f != x v phi), where f is the Skolem
    // function for x.
    struct ForallMarker {
      ForallMarker(Abc::Var x, Scope scope) : x(x), scope(scope) {}
      ForallMarker(Scope scope) : scope(scope) {}
      Abc::Var x;
      Scope scope;
    };
    struct NotMarker {
      NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      NotMarker(Scope scope) : neg(false), scope(scope) {}
      unsigned neg : 1;
      Scope scope;
    };
    std::vector<ForallMarker> foralls;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = word_.begin(); it != word_.end(); ) {
      const Symbol s = *it;
      while (!foralls.empty() && !scoper.active(foralls.back().scope)) {
        foralls.pop_back();
      }
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
      const bool pos = nots.empty() || !nots.back().neg;
      switch (s.tag) {
        case Symbol::kFun:
        case Symbol::kName:
        case Symbol::kVar:
        case Symbol::kFunTerm:
        case Symbol::kNameTerm:
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kLiteral:
        case Symbol::kClause:
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
            const Abc::Fun f = Abc::Instance()->CreateFun(s.u.x.sort(), symbols.size());
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
    }
  }

  // Precondition: Formula is objective
  void Squaring() {
    struct ActionMarker {
      ActionMarker(Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      Symbol::List symbols;
      Scope scope;
    };
    std::vector<ActionMarker> actions;
    Scope::Observer scoper;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!actions.empty() && !scoper.active(actions.back().scope)) {
        actions.pop_back();
      }
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
        case Symbol::kLiteral:
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
        case Symbol::kFunTerm:
        case Symbol::kNameTerm:
        case Symbol::kClause:
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
          assert(false);
          std::abort();
          break;
      }
    }
  }

  void Rectify(bool all_new = false) {
    struct NewVar {
      NewVar() = default;
      NewVar(Abc::Var x, Scope scope) : x(x), scope(scope) {}
      Abc::Var x;
      Scope scope;
    };
    struct NewVars {
      NewVars() = default;
      bool used = false;
      std::vector<NewVar> vars;
    };
    internal::DenseMap<Abc::Var, NewVars> vars;
    Scope::Observer scoper;
    for (Symbol& s : word_) {
      for (auto& nvs : vars) {
        while (!nvs.vars.empty() && !scoper.active(nvs.vars.back().scope)) {
          nvs.vars.pop_back();
        }
      }
      if (s.tag == Symbol::kExists || s.tag == Symbol::kForall) {
        vars.Capacitate(s.u.x, [](auto i) { return internal::next_power_of_two(i) + 1; });
        const Abc::Var y = !vars[s.u.x].used && !all_new ? s.u.x : Abc::Instance()->CreateVar(s.u.x.sort());
        vars[s.u.x].used = true;
        vars[s.u.x].vars.push_back(NewVar(y, scoper.scope()));
        s.u.x = y;
      } else if (s.tag == Symbol::kVar && !vars[s.u.x].vars.empty()) {
        s.u.x = vars[s.u.x].vars.back().x;
      }
      scoper.Munch(s);
    }
  }

  void Flatten() {
    struct NotMarker {
      NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      NotMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      unsigned neg   : 1;
      Scope scope;
    };
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    Symbol::Ref or_ref = word_.end();
    bool tolerate_fun = false;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
      const bool pos = nots.empty() || !nots.back().neg;
      switch (it->tag) {
        case Symbol::kFun:
          if (!tolerate_fun) {
            const Abc::Var x = Abc::Instance()->CreateVar(it->u.f.sort());
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
        case Symbol::kFunTerm:
        case Symbol::kNameTerm:
        case Symbol::kLiteral:
        case Symbol::kClause:
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
    }
  }

  // Precondition: Formula is rectified.
  // Reasons: (1) We pull quantifiers out of disjunctions and conjunctions.
  //          (2) We push actions inwards.
  void PushInwards() {
    struct AndOrMarker {
      AndOrMarker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      AndOrMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      Symbol::Ref ref;
      Scope scope;
    };
    struct ActionMarker {
      ActionMarker(Symbol::List symbols, Scope scope) : symbols(symbols), scope(scope) {}
      ActionMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      Symbol::List symbols;
      Scope scope;
    };
    struct NotMarker {
      NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      NotMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      unsigned neg   : 1;
      Scope scope;
    };
    std::vector<AndOrMarker> andors;
    std::vector<ActionMarker> actions;
    std::vector<NotMarker> nots;
    Scope::Observer scoper;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!andors.empty() && !scoper.active(andors.back().scope)) {
        andors.pop_back();
      }
      while (!actions.empty() && !scoper.active(actions.back().scope)) {
        actions.pop_back();
      }
      while (!nots.empty() && !scoper.active(nots.back().scope)) {
        nots.pop_back();
      }
      switch (it->tag) {
        case Symbol::kVar:
        case Symbol::kFun:
        case Symbol::kName:
        case Symbol::kFunTerm:
        case Symbol::kNameTerm:
          scoper.Munch(*it++);
          break;
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kLiteral:
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
              case Symbol::kEquals:    it->tag = Symbol::kNotEquals; break;
              case Symbol::kNotEquals: it->tag = Symbol::kEquals; break;
              case Symbol::kLiteral:   it->u.a = it->u.a.flip(); break;
              default:                 assert(false); std::abort();
            }
          }
          scoper.Munch(*it++);
          break;
        case Symbol::kClause:
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
    }
  }

  template<typename UnaryPredicate, typename UnaryFunction>
  void Reduce(UnaryPredicate select = UnaryPredicate(), UnaryFunction reduce = UnaryFunction()) {
    struct Marker {
      Marker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      Symbol::Ref ref;
      Scope scope;
    };
    std::vector<Marker> markers;
    Scope::Observer scoper;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!markers.empty() && !scoper.active(markers.back().scope)) {
        const Symbol::Ref begin = markers.back().ref;
        Formula f = reduce(RFormula(begin, it));
        it = word_.Erase(begin, it);
        word_.PutIn(it, std::move(f.word_));
        markers.pop_back();
      }
      if (select(*it)) {
        markers.push_back(Marker(it, scoper.scope()));
      }
      scoper.Munch(*it++);
    }
  }

  internal::DenseSet<Abc::Var> FreeVars() const {
    struct QuantifierMarker {
      QuantifierMarker(Abc::Var x, Scope scope) : x(x), scope(scope) {}
      QuantifierMarker(Scope scope) : scope(scope) {}
      Abc::Var x;
      Scope scope;
    };
    internal::DenseSet<Abc::Var> free;
    internal::DenseMap<Abc::Var, int> bound;
    std::vector<QuantifierMarker> quantifiers;
    Scope::Observer scoper;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!quantifiers.empty() && !scoper.active(quantifiers.back().scope)) {
        --bound[quantifiers.back().x];
        quantifiers.pop_back();
      }
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall || it->x == Abc::Symbol::kVar) {
        bound.Capacitate(it->u.x, [](auto i) { return internal::next_power_of_two(i) + 1; });
        free.Capacitate(it->u.x, [](auto i) { return internal::next_power_of_two(i) + 1; });
      }
      if (it->tag == Abc::Symbol::kExists || it->tag == Abc::Symbol::kForall) {
        ++bound[it->u.x];
        quantifiers.push_back(QuantifierMarker(it->u.x, scoper.scope()));
      } else if (it->tag == Abc::Symbol::kVar && bound[it->u.x] == 0) {
        free.Insert(it->u.x);
      }
      scoper.Munch(*it++);
    }
    return free;
  }

 private:
  static Symbol::Ref End(Symbol::Ref s) {
    Scope::Observer scoper;
    Scope scope = scoper.scope();
    while (scoper.active(scope)) {
      scoper.Munch(*s++);
    }
    return s;
  }

  explicit Formula(Symbol s) { word_.Insert(word_.end(), s); }
  Formula(Symbol s, Formula&& f) : Formula(s) { word_.PutIn(word_.end(), std::move(f.word_)); }
  Formula(Symbol s, Formula&& f, Formula&& g) : Formula(s, std::move(f)) { word_.PutIn(word_.end(), std::move(g.word_)); }
  Formula(Symbol s, const Formula& g) : Formula(s) { word_.Insert(word_.end(), g.word_.begin(), g.word_.end()); }
  Formula(Symbol s, const Formula& f, const Formula& g) : Formula(s, f) { word_.Insert(word_.end(), g.word_.begin(), g.word_.end()); }

  template<typename Formulas>
  Formula WithArgs(Formulas&& args) {
    for (Formula& f : args) {
      word_.PutIn(word_.end(), std::move(f.word_));
    }
    return *this;
  }

  Word word_;
};

}  // namespace limbo

#endif  // LIMBO_FORMULA_H_

