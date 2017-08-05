// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef LIMBO_ACTION_THEORY_H_
#define LIMBO_ACTION_THEORY_H_

#include <cassert>

#include <unordered_map>
#include <vector>

#include <limbo/formula.h>
#include <limbo/literal.h>
#include <limbo/term.h>

namespace limbo {

class ActionTheory {
 public:
  ActionTheory(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}

  bool Add(const Literal a, const Formula& alpha) {
    assert(a.pos() && a.lhs().sort() == a.rhs().sort());
    assert(a.lhs().function() && !a.lhs().sort().compound());
    assert(std::all_of(a.lhs().args().begin(), a.lhs().args().end(), [](const Term t) { return t.variable(); }));
    assert(alpha.objective());
    assert(!alpha.dynamic());
    if (AlreadyDefined(a.lhs()) || Circular(alpha, a.lhs())) {
      return false;
    }
    defs_.push_back(Def(a, alpha.Clone()));
    return true;
  }

  bool Add(const Term t, Literal a, const Formula& alpha) {
    assert(t.variable());
    assert(a.pos() && a.lhs().sort() == a.rhs().sort());
    assert(a.lhs().function() && !a.lhs().sort().compound());
    assert(std::all_of(a.lhs().args().begin(), a.lhs().args().end(), [](const Term t) { return t.variable(); }));
    assert(alpha.objective());
    assert(!alpha.dynamic());
    if (AlreadyDefined(a.lhs()) || Circular(alpha)) {
      return false;
    }
    ssas_.push_back(SSA(t, a, alpha.Clone()));
    return true;
  }

 private:
  struct Def {
    Def(Literal a, Formula::Ref gamma) : a(a), gamma(std::move(gamma)) {}
    Literal a;
    Formula::Ref gamma;
  };

  struct SSA {
    SSA(Term t, Literal a, Formula::Ref gamma) : t(t), a(a), gamma(std::move(gamma)) {}
    Term t;
    Literal a;
    Formula::Ref gamma;
  };

  struct HashSymbolVector {
    internal::hash32_t operator()(const std::vector<Symbol> symbols) const {
      internal::hash32_t h = 0;
      for (Symbol s : symbols) {
        h ^= s.hash();
      }
      return h;
    }
  };

  bool AlreadyDefined(const Term t) const {
    return std::any_of(defs_.begin(), defs_.end(), [t](const Def& def) { return def.a.lhs() == t; }) ||
           std::any_of(ssas_.begin(), ssas_.end(), [t](const SSA& ssa) { return ssa.a.lhs() == t; });
  }

  bool Circular(const Formula& alpha, const Term t = Term()) {
    bool res;
    alpha.Traverse([this, t, &res](const Term tt) { res = AlreadyDefined(tt) || tt == t; return true; });
    return res;
  }

  Symbol Merge(const std::vector<Symbol>& symbols) {
    assert(!symbols.empty());
    auto it = merged_.find(symbols);
    if (it != merged_.end()) {
      return it->second;
    } else {
      Symbol::Arity arity = 0;
      for (Symbol s : symbols) {
        assert(arity < arity + s.arity());
        arity += s.arity();
      }
      const Symbol symbol = sf_->CreateFunction(symbols.back().sort(), arity);
      merged_.insert(std::make_pair(symbols, symbol));
      merged_reverse_.insert(std::make_pair(symbol, symbols));
      return symbol;
    }
  }

  Term Merge(const std::vector<Term>& terms) {
    std::vector<Symbol> symbols;
    symbols.reserve(terms.size());
    for (Term t : terms) {
      symbols.push_back(t.symbol());
    }
    Symbol symbol = Merge(symbols);
    Term::Vector args;
    args.resize(symbol.arity());
    for (Term t : terms) {
      const Term::Vector& add = t.args();
      args.insert(args.end(), add.begin(), add.end());
    }
    return tf_->CreateTerm(symbol, args);
  }

  std::vector<Term> Reverse(const Term t) {
    auto it = merged_reverse_.find(t.symbol());
    if (it == merged_reverse_.end()) {
      return std::vector<Term>{t};
    }
    std::vector<Symbol> symbols = it->second;
    std::vector<Term> terms;
    terms.reserve(symbols.size());
    size_t i = 0;
    for (Symbol symbol : symbols) {
      Term::Vector args;
      args.reserve(symbol.arity());
      for (; i < symbol.arity(); ++i) {
        args.push_back(t.arg(i));
      }
      terms.push_back(tf_->CreateTerm(symbol, args));
    }
    return terms;
  }


  Symbol::Factory* sf_;
  Term::Factory* tf_;
  std::vector<Def> defs_;
  std::vector<SSA> ssas_;
  std::unordered_map<std::vector<Symbol>, Symbol, HashSymbolVector> merged_;
  std::unordered_map<Symbol, std::vector<Symbol>> merged_reverse_;
};

}  // namespace limbo

#endif  // LIMBO_ACTION_THEORY_H_

