// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef LIMBO_ACTION_THEORY_H_
#define LIMBO_ACTION_THEORY_H_

#include <cassert>

#include <unordered_map>
#include <utility>
#include <vector>

#include <limbo/formula.h>
#include <limbo/literal.h>
#include <limbo/term.h>

#include <limbo/internal/iter.h>

namespace limbo {

class ActionTheory {
 public:
  ActionTheory(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
  ActionTheory(const ActionTheory&) = delete;
  ActionTheory& operator=(const ActionTheory&) = delete;
  ActionTheory(ActionTheory&&) = default;
  ActionTheory& operator=(ActionTheory&&) = default;

  bool Add(const Literal a, const Formula& alpha) {
    assert(a.pos() && a.lhs().sort() == a.rhs().sort());
    assert(alpha.objective());
    assert(!alpha.dynamic());
    if (Circular(alpha, a.lhs())) {
      return false;
    }
    defs_.push_back(Def(a, alpha.Clone()));
    return true;
  }

  void AddSenseFunction(Symbol::Sort sort, Symbol fun) {
    assert(fun.arity() == 1);
    sense_funs_.insert(sort, fun);
  }

  bool Add(const Term t, Literal a, const Formula& alpha) {
    assert(t.variable());
    assert(a.pos() && a.lhs().sort() == a.rhs().sort());
    assert(a.lhs().quasi_primitive() && !a.lhs().sort().rigid());
    assert(a.rhs().variable() || a.rhs().quasi_primitive());
    assert(alpha.objective());
    assert(!alpha.dynamic());
    ssas_.push_back(SSA(t, a, alpha.Clone()));
    return true;
  }

  Formula::Ref Regress(const Formula& alpha) const { return Regress({}, *alpha.NF(sf_, tf_)); }

  Formula::Ref Rewrite(const Formula& alpha) { return Rewrite({}, alpha); }

 private:
  struct Def {
    Def(Literal a, Formula::Ref psi) : a(a), psi(std::move(psi)) {}
    Literal a;
    Formula::Ref psi;
  };

  struct SSA {
    SSA(Term t, Literal a, Formula::Ref psi) : t(t), a(a), psi(std::move(psi)) {}
    Term t;
    Literal a;
    Formula::Ref psi;
  };

  typedef internal::IntMultiMap<Symbol::Sort, Symbol> SenseFunctionMap;
  typedef std::vector<internal::Maybe<Symbol>> LongSymbol;

  struct HashLongSymbol {
    internal::hash32_t operator()(const LongSymbol& symbols) const {
      internal::hash32_t h = 0;
      for (internal::Maybe<Symbol> ms : symbols) {
        h ^= ms ? ms.val.hash() : 0;
      }
      return h;
    }
  };

  bool Circular(const Formula& alpha, const Term t = Term()) {
    bool res;
    alpha.Traverse([this, t, &res](const Term tt) { res = tt.symbol() == t.symbol(); return true; });
    return res;
  }

  static Formula::Ref And(Formula::Ref alpha, Formula::Ref beta) {
    return Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(alpha)),
                                                      Formula::Factory::Not(std::move(beta))));
  }

  static Formula::Ref Forall(Term x, Formula::Ref alpha) {
    return Formula::Factory::Not(Formula::Factory::Exists(x, Formula::Factory::Not(std::move(alpha))));
  }

  Formula::Ref RegressLiteral(const Term::Vector& z, const Literal a) const {
    if (!z.empty()) {
      const Term t = z.back();
      const Term::Vector zz = Term::Vector(z.begin(), std::prev(z.end()));
      for (const SSA& ssa : ssas_) {
        internal::Maybe<Term::Substitution> sub = Literal::Unify<Term::kUnifyRight>(a, ssa.a);
        const bool ok = sub && Term::Unify<Term::kUnifyRight>(t, ssa.t, &sub.val);
        if (ok) {
          Formula::Ref psi = ssa.psi->Clone();
          if (!a.pos()) {
            psi = Formula::Factory::Not(std::move(psi));
          }
          psi->SubstituteFree(sub.val, tf_);
          Formula::Ref reg = Regress(zz, *psi)->Rectify(sf_, tf_);
          return reg;
        }
      }
    }
    for (const Def& def : defs_) {
      const internal::Maybe<Term::Substitution> sub = Literal::Unify<Term::kUnifyRight>(a, def.a);
      if (sub) {
        Formula::Ref psi = def.psi->Clone();
        if (!a.pos()) {
          psi = Formula::Factory::Not(std::move(psi));
        }
        psi->SubstituteFree(sub.val, tf_);
        Formula::Ref reg = Regress(z, *psi)->Rectify(sf_, tf_);
        return reg;
      }
    }
    return Formula::Factory::Atomic(Clause{a});
  }

  template<Formula::Ref (*KorM)(Formula::belief_level, Formula::Ref)>
  Formula::Ref RegressKorM(const Term::Vector& z, const Formula::belief_level k, const Formula& alpha) const {
    if (z.empty()) {
      return (*KorM)(k, Regress({}, alpha));
    }
    const Term::Vector zz = Term::Vector(z.begin(), std::prev(z.end()));
    const Term t = z.back();
    const SenseFunctionMap::Bucket& sense_funs = sense_funs_[t.sort()];
    Formula::Ref t_alpha = Formula::Factory::Action(t, alpha.Clone());
    if (sense_funs.empty()) {
      return Regress(zz, *(*KorM)(k, std::move(t_alpha)));
    }
    std::vector<Literal> sense_lits;
    Term::Vector xs;
    for (Symbol sense_fun : sense_funs) {
      const Term sf = tf_->CreateTerm(sense_fun, Term::Vector{t});
      const Term x = tf_->CreateTerm(sf_->CreateVariable(sense_fun.sort()));
      sense_lits.push_back(Literal::Neq(sf, x));
      xs.push_back(x);
    }
    Formula::Ref sense1 = Formula::Factory::Not(Formula::Factory::Atomic(Clause{sense_lits.begin(), sense_lits.end()}));
    Formula::Ref sense2 = sense1->Clone();
    // fa x (sf(t) = x -> K_k (sf(t) = x -> [t] alpha)
    return Regress(zz, *Formula::Factory::Forall(xs.begin(), xs.end(), Formula::Factory::Impl(
                std::move(sense1),
                (*KorM)(k, Formula::Factory::Impl(std::move(sense2), std::move(t_alpha))))));
  }

  Formula::Ref RegressBel(const Term::Vector& z, const Formula::belief_level k, const Formula::belief_level l,
                          const Formula& ante, const Formula& conse) const {
    if (z.empty()) {
      return Formula::Factory::Bel(k, l, Regress({}, ante), Regress({}, conse));
    }
    const Term::Vector zz = Term::Vector(z.begin(), std::prev(z.end()));
    const Term t = z.back();
    const SenseFunctionMap::Bucket& sense_funs = sense_funs_[t.sort()];
    Formula::Ref t_ante = Formula::Factory::Action(t, ante.Clone());
    Formula::Ref t_conse = Formula::Factory::Action(t, conse.Clone());
    if (sense_funs.empty()) {
      return Regress(zz, *Formula::Factory::Bel(k, l, std::move(t_ante), std::move(t_conse)));
    }
    std::vector<Literal> sense_lits;
    Term::Vector xs;
    for (Symbol sense_fun : sense_funs) {
      const Term sf = tf_->CreateTerm(sense_fun, Term::Vector{t});
      const Term x = tf_->CreateTerm(sf_->CreateVariable(sense_fun.sort()));
      sense_lits.push_back(Literal::Neq(sf, x));
      xs.push_back(x);
    }
    Formula::Ref sense1 = Formula::Factory::Not(Formula::Factory::Atomic(Clause{sense_lits.begin(), sense_lits.end()}));
    Formula::Ref sense2 = sense1->Clone();
    // fa x (sf(t) = x -> B_k^l (sf(t) = x ^ [t] ante ==> [t] conse)
    return Regress(zz, *Formula::Factory::Forall(xs.begin(), xs.end(), Formula::Factory::Impl(
                std::move(sense1),
                Formula::Factory::Bel(k, l,
                                      Formula::Factory::And(std::move(sense2), std::move(t_ante)),
                                      std::move(t_conse)))));
  }

  Formula::Ref Regress(const Term::Vector& z, const Formula& alpha) const {
    switch (alpha.type()) {
      case Formula::kAtomic: {
        const Clause& c = alpha.as_atomic().arg();
        auto r = internal::transform_range(c.begin(), c.end(),
                                           [this, &z](const Literal a) { return RegressLiteral(z, a); });
        return Formula::Factory::OrAll(r.begin(), r.end());
      }
      case Formula::kOr:
        return Formula::Factory::Or(Regress(z, alpha.as_or().lhs()), Regress(z, alpha.as_or().rhs()));
      case Formula::kNot:
        return Formula::Factory::Not(Regress(z, alpha.as_not().arg()));
      case Formula::kExists:
        return Formula::Factory::Exists(alpha.as_exists().x(), Regress(z, alpha.as_exists().arg()));
      case Formula::kKnow:
        return RegressKorM<&Formula::Factory::Know>(z, alpha.as_know().k(), alpha.as_know().arg());
      case Formula::kCons:
        return RegressKorM<&Formula::Factory::Cons>(z, alpha.as_cons().k(), alpha.as_cons().arg());
      case Formula::kBel:
        return RegressBel(z, alpha.as_bel().k(), alpha.as_bel().l(),
                          alpha.as_bel().antecedent(), alpha.as_bel().consequent());
      case Formula::kGuarantee:
        return Formula::Factory::Guarantee(Regress(z, alpha.as_guarantee().arg()));
      case Formula::kAction: {
        Term::Vector zz = z;
        zz.push_back(alpha.as_action().t());
        return Regress(zz, alpha.as_action().arg());
      }
    }
  }

  template<typename InputIt>
  static LongSymbol long_symbol(InputIt first, InputIt last) {
    auto r = internal::transform_range(first, last, [](const Symbol s) -> internal::Maybe<Symbol> {
      if (s.variable()) {
        return internal::Nothing;
      } else {
        return internal::Just(s);
      }
    });
    return LongSymbol(r.begin(), r.end());
  }

  static Symbol::Arity long_arity(const LongSymbol& long_symbol) {
    Symbol::Arity arity = 0;
    for (internal::Maybe<Symbol> ms : long_symbol) {
      assert(arity < arity + ms.val.arity());
      arity += ms ? 1 : ms.val.arity();
    }
    return arity;
  }

  template<typename InputIt>
  Symbol MergeSymbols(InputIt first, InputIt last) {
    assert(first != last);
    LongSymbol ls = long_symbol(first, last);
    assert(!ls.empty() && ls.back());
    auto it = merged_.find(ls);
    if (it != merged_.end()) {
      return it->second;
    } else {
      const Symbol symbol = ls.size() > 1 ? sf_->CreateFunction(ls.back().val.sort(), long_arity(ls)) : ls.back().val;
      merged_.insert(std::make_pair(ls, symbol));
      merged_reverse_.insert(std::make_pair(symbol, ls));
      return symbol;
    }
  }

  template<typename InputIt>
  Term MergeTerms(InputIt first, InputIt last) {
    auto r = internal::transform_range(first, last, [](const Term t) { return t.symbol(); });
    Symbol s = MergeSymbols(r.begin(), r.end());
    Term::Vector args;
    args.reserve(s.arity());
    for (auto it = first; it != last; ++it) {
      const Term t = *it;
      if (t.variable()) {
        args.push_back(t);
      } else {
        const Term::Vector& add = t.args();
        args.insert(args.end(), add.begin(), add.end());
      }
    }
    return tf_->CreateTerm(s, args);
  }

  Term Merge(const std::vector<Term>& z, Term t) {
    if (t.variable() || t.name()) {
      return t;
    }
    if (t.sort().rigid() && t.quasi_primitive()) {
      return t;
    }
    auto args = internal::transform_range(t.args(), [this, &z](const Term t) { return Merge(z, t); });
    t = tf_->CreateTerm(t.symbol(), Term::Vector(args.begin(), args.end()));
    auto tt = internal::singleton_range(t);
    auto ts = internal::join_ranges(z.begin(), z.end(), tt.begin(), tt.end());
    return MergeTerms(ts.begin(), ts.end());
  }

  Literal Merge(const std::vector<Term>& z, Literal a) {
    const Term lhs = Merge(z, a.lhs());
    const Term rhs = Merge(z, a.rhs());
    return a.pos() ? Literal::Eq(lhs, rhs) : Literal::Neq(lhs, rhs);
  }

  Term::Vector Reverse(const Term t) {
    auto it = merged_reverse_.find(t.symbol());
    if (it == merged_reverse_.end()) {
      return std::vector<Term>{t};
    }
    LongSymbol symbols = it->second;
    Term::Vector terms;
    terms.reserve(symbols.size());
    Term::Vector::const_iterator first_arg = t.args().begin();
    for (internal::Maybe<Symbol> ms : symbols) {
      if (!ms) {
        terms.push_back(*first_arg);
        ++first_arg;
      } else {
        const Symbol s = ms.val;
        const Term::Vector::const_iterator last_arg = first_arg + s.arity();
        terms.push_back(tf_->CreateTerm(s, Term::Vector(first_arg, last_arg)));
        first_arg = last_arg;
      }
    }
    return terms;
  }

  Formula::Ref Rewrite(const Term::Vector& z, const Formula& alpha) {
    switch (alpha.type()) {
      case Formula::kAtomic: {
        const Clause& c = alpha.as_atomic().arg();
        auto lits = internal::transform_range(c.begin(), c.end(), [this, &z](const Literal a) { return Merge(z, a); });
        return Formula::Factory::Atomic(Clause(lits.begin(), lits.end()));
      }
      case Formula::kOr:
        return Formula::Factory::Or(Rewrite(z, alpha.as_or().lhs()), Rewrite(z, alpha.as_or().rhs()));
      case Formula::kNot:
        return Formula::Factory::Not(Rewrite(z, alpha.as_not().arg()));
      case Formula::kExists:
        return Formula::Factory::Exists(alpha.as_exists().x(), Rewrite(z, alpha.as_exists().arg()));
      case Formula::kKnow:
        return Formula::Factory::Know(alpha.as_know().k(), Rewrite(z, alpha.as_know().arg()));
      case Formula::kCons:
        return Formula::Factory::Cons(alpha.as_cons().k(), Rewrite(z, alpha.as_cons().arg()));
      case Formula::kBel:
        return Formula::Factory::Bel(alpha.as_bel().k(), alpha.as_bel().l(),
                                     Rewrite(z, alpha.as_bel().antecedent()),
                                     Rewrite(z, alpha.as_bel().consequent()));
      case Formula::kGuarantee:
        return Formula::Factory::Guarantee(Rewrite(z, alpha.as_guarantee().arg()));
      case Formula::kAction: {
        Term::Vector zz = z;
        const Term t = Merge(z, alpha.as_action().t());
        zz.push_back(t);
        return Rewrite(z, alpha.as_action().arg());
      }
    }
  }

  Symbol::Factory* sf_;
  Term::Factory* tf_;
  SenseFunctionMap sense_funs_;

  std::vector<Def> defs_;
  std::vector<SSA> ssas_;

  std::unordered_map<LongSymbol, Symbol, HashLongSymbol> merged_;
  std::unordered_map<Symbol, LongSymbol> merged_reverse_;
};

}  // namespace limbo

#endif  // LIMBO_ACTION_THEORY_H_

