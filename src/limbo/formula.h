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
#include <limbo/term.h>

#include <limbo/internal/dense.h>
#include <limbo/internal/hash.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/singleton.h>

namespace limbo {

class Language : private internal::Singleton<Language> {
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

  class Func : public Identifiable<Sort> {
   public:
    using Identifiable::Identifiable;
    Sort sort() const { return Instance()->func_sort_[*this]; }
    int arity() const { return Instance()->func_arity_[*this]; }
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
    QuasiLiteral(Func f, InputIt args_begin, bool eq, QuasiName n) : eq_(eq), f_(f) {
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
    Func f_;
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
    enum Type {
      kFunc, kName, kVar, kTerm,
      kEquals, kNotEquals, kLiteral, kClause,
      kNot, kOr, kAnd, kExists, kForall, kKnow,
      kMaybe, kBelieve, kAction
    };

    using List = std::list<Symbol>;
    using Ref = List::iterator;
    using CRef = List::const_iterator;

    class Range {
     public:
      Range(Ref begin, Ref end) : begin_(begin), end_(end) {}

      Ref begin() const { return begin_; }
      Ref end() const { return end_; }

      Type type() const { assert(!empty()); return begin_->type; }
      bool empty() const { return end_ == begin_; }
      bool ground() const { return std::all_of(begin(), end(), [](Symbol s) { return s.type != kVar; }); }

     private:
      Ref begin_;
      Ref end_;
    };

    static Symbol Func(Func f)            { Symbol s; s.type = kFunc;      s.u.f = f;            return s; }
    static Symbol Name(Name n)            { Symbol s; s.type = kName;      s.u.n = n;            return s; }
    static Symbol Var(Var x)              { Symbol s; s.type = kVar;       s.u.x = x;            return s; }
    static Symbol Term(Term t)            { Symbol s; s.type = kTerm;      s.u.t = t;            return s; }
    static Symbol Equals()                { Symbol s; s.type = kEquals;                          return s; }
    static Symbol NotEquals()             { Symbol s; s.type = kNotEquals;                       return s; }
    static Symbol Literal(Literal a)      { Symbol s; s.type = kLiteral;   s.u.a = a;            return s; }
    static Symbol Clause(const Clause* c) { Symbol s; s.type = kClause;    s.u.c = c;            return s; }
    static Symbol Not()                   { Symbol s; s.type = kNot;                             return s; }
    static Symbol Exists(class Var x)     { Symbol s; s.type = kExists;    s.u.x = x;            return s; }
    static Symbol Forall(class Var x)     { Symbol s; s.type = kForall;    s.u.x = x;            return s; }
    static Symbol Or(int k)               { Symbol s; s.type = kOr;        s.u.k = k;            return s; }
    static Symbol And(int k)              { Symbol s; s.type = kAnd;       s.u.k = k;            return s; }
    static Symbol Know(int k)             { Symbol s; s.type = kKnow;      s.u.k = k;            return s; }
    static Symbol Maybe(int k)            { Symbol s; s.type = kMaybe;     s.u.k = k;            return s; }
    static Symbol Believe(int k, int l)   { Symbol s; s.type = kBelieve;   s.u.k = k; s.u.l = l; return s; }
    static Symbol Action()                { Symbol s; s.type = kAction;                          return s; }

    Symbol() = default;

    Type type;

    union {
      class Func          f;  // kFunc
      class Name          n;  // kName
      class Var           x;  // kVar, kExists, kForall
      class Term          t;  // kTerm
      class Literal       a;  // kLiteral
      const class Clause* c;  // kClause
      int k;                  // kOr, kAnd, kKnow, kMaybe, kBelieve
      int l;                  // kBelieve
    } u;

    bool operator==(const Symbol& s) const {
      if (type != s.type) {
        return false;
      }
      switch (type) {
        case kFunc:      return u.f == s.u.f;
        case kName:      return u.n == s.u.n;
        case kVar:       return u.x == s.u.x;
        case kTerm:      return u.t == s.u.t;
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

    int n_args() const {
      switch (type) {
        case kFunc:      return u.f.arity();
        case kName:      return u.n.arity();
        case kVar:       return 0;
        case kTerm:      return 0;
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

  class Word {
   public:
    class Consumer;
    class Scope {
     public:
      Scope() = default;
      explicit Scope(int k) : k_(k) {}

     private:
      friend class Consumer;
      int k_;
    };

    class Consumer {
     public:
      Consumer() = default;
      void Munch(const Symbol& s) { k_ += s.n_args(); --k_; }
      Scope scope() const { return Scope(k_); }
      bool active(const Scope& scope) const { return k_ >= scope.k_; }

     private:
      Consumer(const Consumer&) = delete;
      Consumer& operator=(const Consumer&) = delete;
      Consumer(Consumer&&) = delete;
      Consumer& operator=(Consumer&&) = delete;

      int k_ = 0;
    };

    template<typename Words>
    static Word Func(Func f, Words&& args)    { return Word(Symbol::Func(f), args.begin(), args.end()); }
    static Word Name(Name n)                  { return Word(Symbol::Name(n)); }
    static Word Var(Var x)                    { return Word(Symbol::Var(x)); }
    static Word Term(Term a)                  { return Word(Symbol::Term(a)); }
    static Word Equals(Word w1, Word w2)      { return Word(Symbol::Equals(), w1, w2); }
    static Word Literal(Literal a)            { return Word(Symbol::Literal(a)); }
    static Word Clause(const Clause* c)       { return Word(Symbol::Clause(c)); }
    static Word Not(Word w)                   { return Word(Symbol::Not(), w); }
    static Word Exists(class Var x, Word w)   { return Word(Symbol::Exists(x), w); }
    static Word Forall(class Var x, Word w)   { return Word(Symbol::Forall(x), w); }
    static Word Or(Word w1, Word w2)          { return Word(Symbol::Or(2), w1, w2); }
    static Word Or(std::list<Word> ws)        { return Word(Symbol::Or(ws.size()), ws.begin(), ws.end()); }
    static Word And(Word w1, Word w2)         { return Word(Symbol::And(2), w1, w2); }
    static Word And(std::list<Word> ws)       { return Word(Symbol::And(ws.size()), ws.begin(), ws.end()); }
    static Word Know(int k, Word w)           { return Word(Symbol::Know(k), w); }
    static Word Maybe(int k, Word w)          { return Word(Symbol::Know(k), w); }
    static Word Believe(int k, int l, Word w) { return Word(Symbol::Believe(k, l), w); }
    static Word Action(Word w1, Word w2)      { return Word(Symbol::Action(), w1, w2); }

    static Symbol::Ref End(Symbol::Ref s) { return EndLength(s).first; }
    static int Length(Symbol::Ref s)      { return EndLength(s).second; }

    static std::pair<Symbol::Ref, int> EndLength(Symbol::Ref s) {
      int n = 0;
      Consumer consumer;
      Scope scope = consumer.scope();
      do {
        ++n;
        consumer.Munch(*s++);
      } while (consumer.active(scope));
      return std::make_pair(s, n);
    }

    Word(Word&&) = default;
    Word& operator=(Word&&) = default;

    Word Clone() const { return Word(*this); }

    Word() = default;
    Word(std::initializer_list<Symbol> symbols) : symbols_(symbols) {}

    Word(Symbol s) { symbols_.push_back(s); }
    Word(Symbol s, Word&& w) : Word(s) { symbols_.splice(symbols_.end(), w.symbols_); }
    Word(Symbol s, Word&& w1, Word&& w2) : Word(s) {
      symbols_.splice(symbols_.end(), w1.symbols_);
      symbols_.splice(symbols_.end(), w2.symbols_);
    }
    template<typename InputIt>
    Word(Symbol s, InputIt w_begin, InputIt w_end) {
      symbols_.push_back(s);
      for (; w_begin != w_end; ++w_begin) {
        Word& w = *w_begin;
        symbols_.splice(symbols_.end(), w.symbols_);
      }
    }

    Word(Symbol s, const Word& w) : Word(s, w.Clone()) {}
    Word(Symbol s, const Word& w1, const Word& w2) : Word(s, w1.Clone(), w2.Clone()) {}

    Word(const Word&) = default;
    Word& operator=(const Word&) = delete;

    Symbol::Ref begin() { return symbols_.begin(); }
    Symbol::Ref end()   { return symbols_.end(); }
    Symbol::CRef begin() const { return symbols_.cbegin(); }
    Symbol::CRef end()   const { return symbols_.cend(); }

    Symbol::Ref Insert(Symbol::Ref tgt, Symbol s) {
      return symbols_.insert(tgt, s);
    }

    template<typename InputIt>
    Symbol::Ref Insert(Symbol::Ref tgt, InputIt first, InputIt last) {
      return symbols_.insert(tgt, first, last);
    }

    Symbol::Ref Erase(Symbol::Ref it) {
      return symbols_.erase(it);
    }

    Symbol::Ref Erase(Symbol::Ref first, Symbol::Ref last) {
      return symbols_.erase(first, last);
    }

    void Move(Symbol::Ref tgt, Symbol::Ref first) {
      symbols_.splice(tgt, symbols_, first);
    }

    void Move(Symbol::Ref tgt, Symbol::Ref first, Symbol::Ref last) {
      symbols_.splice(tgt, symbols_, first, last);
    }

    Symbol::List TakeOut(Symbol::Ref first, Symbol::Ref last) {
      Symbol::List tgt;
      tgt.splice(tgt.end(), symbols_, first, last);
      return tgt;
    }

   private:
    Symbol::List symbols_;
  };

  static Language* Instance() {
    if (instance == nullptr) {
      instance = std::unique_ptr<Language>(new Language());
    }
    return instance.get();
  }

  Sort CreateSort(bool rigid) {
    const Sort s = Sort(++last_sort_);
    sort_rigid_.Capacitate(s, [](auto i) { return internal::next_power_of_two(i) + 1; });
    sort_rigid_[s] = rigid;
    return s;
  }

  Func CreateFunc(Sort s, int arity) {
    const Func f = Func(++last_func_);
    func_sort_.Capacitate(f, [](auto i) { return internal::next_power_of_two(i) + 1; });
    func_sort_[f] = s;
    func_arity_.Capacitate(f, [](auto i) { return internal::next_power_of_two(i) + 1; });
    func_arity_[f] = arity;
    return f;
  }

  Name CreateName(Sort s, int arity = 0) {
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

  Term Termify(Symbol::Range sr) {
    assert((sr.type() == Symbol::kFunc || sr.type() == Symbol::kName) && sr.ground());
    auto it = symbols_term_.find(sr);
    if (it != symbols_term_.end()) {
      return it->second;
    } else {
      const bool is_func = sr.type() == Symbol::kFunc;
      const Term t = is_func ? Term::CreateFunc(++last_func_term_) : Term::CreateName(++last_name_term_);
      symbols_term_[sr] = t;
      (is_func ? term_func_symbols_ : term_name_symbols_)[t] = sr;
      return t;
    }
  }

 private:
  struct DeepHash {
    internal::hash32_t operator()(const Symbol::Range& sr) const {
      internal::hash32_t h = 0;
      for (Symbol s : sr) {
        int i = 0;
        switch (s.type) {
          case Symbol::kFunc: i = s.u.f.index(); break;
          case Symbol::kName: i = s.u.n.index(); break;
          case Symbol::kVar:  i = s.u.x.index(); break;
          case Symbol::kTerm: i = s.u.t.index(); break;
          default:                               break;
        }
        h ^= internal::jenkins_hash(s.type);
        h ^= internal::jenkins_hash(i);
      }
      return h;
    }
  };

  struct DeepEquals {
    bool operator()(const Symbol::Range& sr1, const Symbol::Range& sr2) const {
      return std::equal(sr1.begin(), sr1.end(), sr2.begin());
    }
  };

  using TermMap = std::unordered_map<Symbol::Range, class Term, DeepHash, DeepEquals>;

  Language() = default;
  Language(const Language&) = delete;
  Language(Language&&) = delete;
  Language& operator=(const Language&) = delete;
  Language& operator=(Language&&) = delete;

  internal::DenseMap<Sort, bool> sort_rigid_;
  internal::DenseMap<Func, Sort> func_sort_;
  internal::DenseMap<Func, int>  func_arity_;
  internal::DenseMap<Name, Sort> name_sort_;
  internal::DenseMap<Name, int>  name_arity_;
  internal::DenseMap<Var, Sort>  var_sort_;

  internal::DenseMap<Term, Symbol::Range> term_func_symbols_;
  internal::DenseMap<Term, Symbol::Range> term_name_symbols_;
  TermMap symbols_term_;
  int last_sort_ = 0;
  int last_var_ = 0;
  int last_func_ = 0;
  int last_name_ = 0;
  int last_func_term_ = 0;
  int last_name_term_ = 0;
};

class Formula {
 public:
  using Func = Language::Func;
  using Name = Language::Name;
  using Var = Language::Var;
  using Symbol = Language::Symbol;
  using Word = Language::Word;
  using Consumer = Word::Consumer;
  using Scope = Word::Scope;

  Formula(Word&& w) : word_(w) {}

  const Word& word() const { return word_; }

  void Normalize() {
    Rectify();
    PushInwards();
    Flatten();
  }

  void Skolemize() {
    struct ForallMarker {
      ForallMarker(Var x, Scope scope) : x(x), scope(scope) {}
      ForallMarker(Scope scope) : scope(scope) {}
      Var x;
      Scope scope;
    };
    struct NotMarker {
      NotMarker(bool neg, Scope scope) : neg(neg), scope(scope) {}
      NotMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      unsigned neg   : 1;
      Scope scope;
    };
    struct SkolemFunction {
      SkolemFunction(Symbol::List symbols, Scope scope) : reset(false), symbols(symbols), scope(scope) {}
      SkolemFunction(Scope scope) : reset(true), scope(scope) {}
      unsigned reset : 1;
      Symbol::List symbols;
      Scope scope;
    };
    std::vector<ForallMarker> foralls;
    std::vector<NotMarker> nots;
    internal::DenseMap<Var, std::vector<SkolemFunction>> funcs;
    Consumer consumer;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!foralls.empty() && !consumer.active(foralls.back().scope)) {
        foralls.pop_back();
      }
      while (!nots.empty() && !consumer.active(nots.back().scope)) {
        nots.pop_back();
      }
      for (auto& sfs : funcs) {
        while (!sfs.empty() && !consumer.active(sfs.back().scope)) {
          sfs.pop_back();
        }
      }
      const bool pos = nots.empty() || nots.back().reset || !nots.back().neg;
      if (it->type == Symbol::kVar || it->type == Symbol::kExists || it->type == Symbol::kForall) {
        funcs.Capacitate(it->u.x, [](auto i) { return internal::next_power_of_two(i) + 1; });
      }
      if (it->type == Symbol::kVar) {
        if (!funcs[it->u.x].empty()) {
          const SkolemFunction& sf = funcs[it->u.x].back();
          if (!sf.reset) {
            it = word_.Erase(it);
            word_.Insert(it, sf.symbols.begin(), sf.symbols.end());
          } else {
            consumer.Munch(*it++);
          }
        }
      } else if ((it->type == Symbol::kExists && pos) || (it->type == Symbol::kForall && !pos)) {
        Symbol::List symbols;
        for (const ForallMarker& fm : foralls) {
          symbols.push_back(Symbol::Var(fm.x));
        }
        const Func f = Language::Instance()->CreateFunc(it->u.x.sort(), symbols.size());
        symbols.push_front(Symbol::Func(f));
        funcs[it->u.x].push_back(SkolemFunction(symbols, consumer.scope()));
        it = word_.Erase(it);
      } else if ((it->type == Symbol::kExists && !pos) || (it->type == Symbol::kForall && pos)) {
        funcs[it->u.x].push_back(SkolemFunction(consumer.scope()));
        foralls.push_back(ForallMarker(it->u.x, consumer.scope()));
        consumer.Munch(*it++);
      } else if (it->type == Symbol::kNot) {
        nots.push_back(NotMarker(nots.empty() || nots.back().reset || !nots.back().neg, consumer.scope()));
        consumer.Munch(*it++);
      } else if (it->type == Symbol::kKnow || it->type == Symbol::kMaybe || it->type == Symbol::kBelieve) {
        nots.push_back(NotMarker(consumer.scope()));
        consumer.Munch(*it++);
      } else {
        consumer.Munch(*it++);
      }
    }
  }

  void Rectify() {
    struct NewVar {
      NewVar() = default;
      NewVar(Var x, Scope scope) : x(x), scope(scope) {}
      Var x;
      Scope scope;
    };
    internal::DenseMap<Var, std::vector<NewVar>> vars;
    Consumer consumer;
    for (Symbol& s : word_) {
      for (auto& nvs : vars) {
        while (!nvs.empty() && !consumer.active(nvs.back().scope)) {
          nvs.pop_back();
        }
      }
      if (s.type == Symbol::kExists || s.type == Symbol::kForall) {
        vars.Capacitate(s.u.x, [](auto i) { return internal::next_power_of_two(i) + 1; });
        const Var y = vars[s.u.x].empty() ? s.u.x : Language::Instance()->CreateVar(s.u.x.sort());
        vars[s.u.x].push_back(NewVar(y, consumer.scope()));
        s.u.x = y;
      } else if (s.type == Symbol::kVar && !vars[s.u.x].empty()) {
        s.u.x = vars[s.u.x].back().x;
      }
      consumer.Munch(s);
    }
  }

  void PushInwards() {
    // Preserves equivalence in generaly only if formula is rectified because
    // quantifiers are pulled out of disjunctions.
    struct AndOrMarker {
      AndOrMarker(Symbol::Ref ref, Scope scope) : ref(ref), scope(scope) {}
      AndOrMarker(Scope scope) : reset(true), scope(scope) {}
      unsigned reset  : 1;
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
    Consumer consumer;
    for (auto it = word_.begin(); it != word_.end(); ) {
      while (!andors.empty() && !consumer.active(andors.back().scope)) {
        andors.pop_back();
      }
      while (!actions.empty() && !consumer.active(actions.back().scope)) {
        actions.pop_back();
      }
      while (!nots.empty() && !consumer.active(nots.back().scope)) {
        nots.pop_back();
      }
      switch (it->type) {
        case Symbol::kFunc:
        case Symbol::kName:
        case Symbol::kVar:
        case Symbol::kTerm:
          consumer.Munch(*it++);
          break;
        case Symbol::kEquals:
        case Symbol::kNotEquals:
        case Symbol::kLiteral:
          if (andors.empty() || andors.back().reset) {
            const Symbol::Ref last = word_.Insert(it, Symbol::Or(1));
            andors.push_back(AndOrMarker(last, consumer.scope()));
            consumer.Munch(*last);
          }
          {
            Symbol::Ref pre = it;
            for (auto sit = actions.rbegin(); sit != actions.rend() && !sit->reset; ++sit) {
              pre = word_.Insert(pre, sit->symbols.begin(), sit->symbols.end());
            }
          }
          if (!nots.empty() && nots.back().neg) {
            switch (it->type) {
              case Symbol::kEquals:    it->type = Symbol::kNotEquals; break;
              case Symbol::kNotEquals: it->type = Symbol::kEquals; break;
              case Symbol::kLiteral:   it->u.a = it->u.a.flip(); break;
              default:                 std::abort();
            }
          }
          consumer.Munch(*it++);
          break;
        case Symbol::kClause:
          std::abort();
          break;
        case Symbol::kKnow:
        case Symbol::kMaybe:
        case Symbol::kBelieve:
          andors.push_back(AndOrMarker(consumer.scope()));
          actions.push_back(ActionMarker(consumer.scope()));
          nots.push_back(NotMarker(consumer.scope()));
          consumer.Munch(*it++);
          break;
        case Symbol::kAction:
          actions.push_back(ActionMarker(word_.TakeOut(it, Word::End(std::next(it))), consumer.scope()));
          break;
        case Symbol::kNot:
          nots.push_back(NotMarker(nots.empty() || nots.back().reset || !nots.back().neg, consumer.scope()));
          it = word_.Erase(it);
          break;
        case Symbol::kExists:
        case Symbol::kForall:
          if (!nots.empty() && nots.back().neg) {
            it->type = it->type == Symbol::kExists ? Symbol::kForall : Symbol::kExists;
          }
          consumer.Munch(*it++);
          if (!andors.empty() && !andors.back().reset) {
            word_.Move(andors.back().ref, std::prev(it));
          }
          break;
        case Symbol::kOr:
        case Symbol::kAnd:
          consumer.Munch(*it);
          if (!andors.empty() && !andors.back().reset && andors.back().ref->type == it->type) {
            andors.back().ref->u.k += it->u.k;
            it = word_.Erase(it);
          } else {
            andors.push_back(AndOrMarker(it, consumer.scope()));
            ++it;
          }
          break;
      }
    }
  }

  void Flatten() {
  }

 private:
  Word word_;
};

}  // namespace limbo

#endif  // LIMBO_FORMULA_H_

