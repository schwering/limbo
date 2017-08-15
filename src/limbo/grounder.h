// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A Grounder determines how many standard names need to be substituted for
// variables in a proper+ knowledge base and in queries.
//
// The grounder incrementally builds up the setup whenever AddClause(),
// PrepareForQuery(), or GuaranteeConsistency() are called. In particular,
// the relevant standard names (including the additional names) are managed and
// the clauses are regrounded accordingly. The Grounder is designed for fast
// backtracking.
//
// PrepareForQuery() should not be called before GuaranteeConsistency().
// Otherwise their behaviour is undefined.
//
// Quantification requires the temporary use of additional standard names.
// Grounder uses a temporary NamePool where names can be returned for later
// re-use. This NamePool is public for it can also be used to handle free
// variables in the representation theorem.


#ifndef LIMBO_GROUNDER_H_
#define LIMBO_GROUNDER_H_

#include <cassert>

#include <algorithm>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/formula.h>
#include <limbo/setup.h>

#include <limbo/internal/hash.h>
#include <limbo/internal/intmap.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>

#include <limbo/format/output.h>

namespace limbo {

class Grounder {
 public:
  typedef internal::size_t size_t;
  typedef Setup::ClauseRange::Index ClauseIndex;

  template<Symbol (Symbol::Factory::*CreateSymbol)(Symbol::Sort)>
  class Pool {
   public:
    Pool(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
    Pool(Pool&&) = default;
    Pool& operator=(Pool&&) = default;

    Term Create(Symbol::Sort sort) {
      if (terms_[sort].empty()) {
        return tf_->CreateTerm((sf_->*CreateSymbol)(sort));
      } else {
        Term t = terms_[sort].back();
        terms_[sort].pop_back();
        return t;
      }
    }

    void Return(Term t) { terms_[t.sort()].push_back(t); }

    Term Get(Symbol::Sort sort, size_t i) {
      Term::Vector& ts = terms_[sort];
      while (i < ts.size()) {
        ts.push_back(tf_->CreateTerm((sf_->*CreateSymbol)(sort)));
      }
      return ts[i];
    }

   private:
    Symbol::Factory* const sf_;
    Term::Factory* const tf_;
    internal::IntMap<Symbol::Sort, Term::Vector> terms_;
  };

  typedef Pool<&Symbol::Factory::CreateName> NamePool;
  typedef Pool<&Symbol::Factory::CreateVariable> VariablePool;

  typedef Formula::SortedTermSet SortedTermSet;

  template<typename T>
  struct Ungrounded {
    typedef T value_type;
    struct Hash { internal::hash32_t operator()(const Ungrounded<T>& u) const { return u.val.hash(); } };
    typedef std::vector<Ungrounded> Vector;
    typedef std::unordered_set<Ungrounded, Hash> Set;

    Ungrounded(const Ungrounded&) = default;
    Ungrounded& operator=(const Ungrounded&) = default;
    Ungrounded(Ungrounded&&) = default;
    Ungrounded& operator=(Ungrounded&&) = default;

    bool operator==(const Ungrounded& u) const { return val == u.val; }
    bool operator!=(const Ungrounded& u) const { return !(*this != u); }

    T val;
    SortedTermSet vars;

   private:
    friend class Grounder;

    explicit Ungrounded(const T& val) : val(val) {}
  };

  struct Ply {
    typedef std::list<Ply> List;

    Ply(const Ply&) = delete;
    Ply& operator=(const Ply&) = delete;
    Ply(Ply&&) = default;
    Ply& operator=(Ply&&) = default;

    struct {
      Ungrounded<Clause>::Vector ungrounded;
      Setup::ShallowCopy shallow_setup;
      std::unordered_map<Term, std::unordered_set<ClauseIndex>> with_term;
    } clauses;
    struct {
      SortedTermSet mentioned;       // names mentioned in clause or prepared-for query (but are not plus-names)
      SortedTermSet plus_max;        // plus-names that may be used for multiple purposes
      SortedTermSet plus_new;        // plus-names that may not be used for multiple purposes
      SortedTermSet plus_mentioned;  // plus-names that later occurred in formulas (which lead to plus_new names)
    } names;
    struct {
      Ungrounded<Literal>::Set ungrounded;  // literals in prepared-for query
      std::unordered_map<Term, std::unordered_set<Term>> map;  // grounded lhs-rhs index for clauses, prepared-for query
    } lhs_rhs;
    struct {
      bool filter = false;  // enabled after consistency guarantee
      Ungrounded<Term>::Set ungrounded;
      std::unordered_set<Term> terms;
      std::unordered_set<ClauseIndex> clauses;
    } relevant;
    bool do_not_add_if_inconsistent = false;  // enabled for fix-literals

   private:
    friend class Grounder;

    Ply() = default;
  };

  struct Plies {
    typedef Ply::List::const_reverse_iterator iterator;
    enum Policy { kAll, kNew, kOld };

    iterator begin() const {
      switch (policy) {
        case kAll:
        case kNew:
          return owner->plies_.rbegin();
        case kOld:
          return std::next(owner->plies_.rbegin());
      }
      return owner->plies_.rbegin();
    }

    iterator end() const {
      switch (policy) {
        case kAll:
        case kOld:
          return owner->plies_.rend();
        case kNew:
          return std::next(begin());
      }
    }

   private:
    friend class Grounder;

    explicit Plies(const Grounder* owner, Policy policy = kAll) : owner(owner), policy(policy) {}

    const Grounder* const owner;
    const Plies::Policy policy;
  };

  struct LhsTerms {
    struct First { Term operator()(const std::pair<Term, std::unordered_set<Term>>& p) const { return p.first; } };
    typedef std::unordered_map<Term, std::unordered_set<Term>>::const_iterator pair_iterator;
    typedef internal::transform_iterator<pair_iterator, First> term_iterator;

    struct New {
      New() = default;
      New(Plies::iterator begin, Plies::iterator end) : begin(begin), end(end) {}
      bool operator()(Term t) const {
        for (auto it = begin; it != end; ++it) {
          auto& m = it->lhs_rhs.map;
          if (m.find(t) != m.end()) {
            return false;
          }
        }
        return true;
      }
     private:
      Plies::iterator begin;
      Plies::iterator end;
    };

    typedef internal::filter_iterator<term_iterator, New> unique_term_iterator;

    struct Begin {
      explicit Begin(const Plies* plies) : plies(plies) {}
      unique_term_iterator operator()(const Plies::iterator it) const {
        term_iterator b = term_iterator(it->lhs_rhs.map.begin(), First());
        term_iterator e = term_iterator(it->lhs_rhs.map.end(), First());
        return unique_term_iterator(b, e, New(plies->begin(), it));
      }
     private:
      const Plies* plies;
    };

    struct End {
      explicit End(const Plies* plies) : plies(plies) {}
      unique_term_iterator operator()(const Plies::iterator it) const {
        term_iterator e = term_iterator(it->lhs_rhs.map.end(), First());
        return unique_term_iterator(e, e, New(plies->begin(), it));
      }
     private:
      const Plies* plies;
    };

    struct IsRelevant {
      explicit IsRelevant(const Grounder* owner) : owner(owner) {}
      bool operator()(const Term t) const { return owner->IsRelevantTerm(t); }
     private:
      const Grounder* owner;
    };

    typedef internal::flatten_iterator<Plies::iterator, unique_term_iterator, Begin, End> flat_iterator;
    typedef internal::filter_iterator<flat_iterator, IsRelevant> iterator;

    iterator begin() const {
      auto b = flat_iterator(plies.begin(), plies.end(), Begin(&plies), End(&plies));
      auto e = flat_iterator(plies.end(), plies.end(), Begin(&plies), End(&plies));
      return iterator(b, e, is_relevant);
    }

    iterator end() const {
      auto e = flat_iterator(plies.end(), plies.end(), Begin(&plies), End(&plies));
      return iterator(e, e, is_relevant);
    }

   private:
    friend class Grounder;

    LhsTerms(const Grounder* owner, Plies::Policy policy)
        : is_relevant(owner), plies(owner, policy) {}

    const IsRelevant is_relevant;
    const Plies plies;
  };

  struct RhsNames {
    typedef std::unordered_set<Term>::const_iterator name_iterator;

    struct Begin {
      Begin(Term t, name_iterator end) : t(t), end(end) {}
      name_iterator operator()(const Ply& p) const {
        auto it = p.lhs_rhs.map.find(t);
        return it != p.lhs_rhs.map.end() ? it->second.begin() : end;
      }
     private:
      Term t;
      name_iterator end;
    };

    struct End {
      End(Term t, name_iterator end) : t(t), end(end) {}
      name_iterator operator()(const Ply& p) const {
        auto it = p.lhs_rhs.map.find(t);
        return it != p.lhs_rhs.map.end() ? it->second.end() : end;
      }
     private:
      Term t;
      name_iterator end;
    };

    typedef internal::flatten_iterator<Plies::iterator, name_iterator, Begin, End> flat_iterator;
    typedef internal::singleton_iterator<Term> plus_iterator;
    typedef internal::joined_iterator<flat_iterator, plus_iterator> iterator;

    ~RhsNames() { if (!n_it->null()) { owner->name_pool_.Return(*n_it); } }

    iterator begin() const {
      flat_iterator b = flat_iterator(plies.begin(), plies.end(), Begin(t, ts.end()), End(t, ts.end()));
      flat_iterator e = flat_iterator(plies.end(),   plies.end(), Begin(t, ts.end()), End(t, ts.end()));
      if (n_it->null()) {
        n_it = plus_iterator(owner->name_pool_.Create(t.sort()));
      }
      return iterator(b, e, n_it);
    }

    iterator end() const {
      flat_iterator b = flat_iterator(plies.end(), plies.end(), Begin(t, ts.end()), End(t, ts.end()));
      flat_iterator e = flat_iterator(plies.end(), plies.end(), Begin(t, ts.end()), End(t, ts.end()));
      return iterator(b, e, plus_iterator());
    }

   private:
    friend class Grounder;

    RhsNames(Grounder* owner, Term t, Plies::Policy policy)
        : owner(owner), plies(owner, policy), t(t), n_it(Term()) { assert(n_it->null()); }

    Grounder* const owner;
    const Plies plies;
    const Term t;
    mutable plus_iterator n_it;
    const std::unordered_set<Term> ts = {};
  };

  struct Names {
    typedef internal::joined_iterator<typename SortedTermSet::value_iterator> plus_iterator;
    typedef internal::joined_iterator<typename SortedTermSet::value_iterator, plus_iterator> name_iterator;

    struct Begin {
      explicit Begin(Symbol::Sort sort) : sort(sort) {}
      name_iterator operator()(const Ply& p) const {
        auto b = plus_iterator(p.names.plus_max.begin(sort), p.names.plus_max.end(sort), p.names.plus_new.begin(sort));
        return name_iterator(p.names.mentioned.begin(sort), p.names.mentioned.end(sort), b);
      }
     private:
      Symbol::Sort sort;
    };

    struct End {
      explicit End(Symbol::Sort sort) : sort(sort) {}
      name_iterator operator()(const Ply& p) const {
        auto e = plus_iterator(p.names.plus_max.end(sort), p.names.plus_max.end(sort), p.names.plus_new.begin(sort));
        return name_iterator(p.names.mentioned.end(sort), p.names.mentioned.end(sort), e);
      }
     private:
      Symbol::Sort sort;
    };

    typedef internal::flatten_iterator<Plies::iterator, name_iterator, Begin, End> iterator;

    iterator begin() const { return iterator(plies.begin(), plies.end(), Begin(sort), End(sort)); }
    iterator end()   const { return iterator(plies.end(),   plies.end(), Begin(sort), End(sort)); }

   private:
    friend class Grounder;

    Names(const Grounder* owner, Symbol::Sort sort, Plies::Policy policy) : sort(sort), plies(owner, policy) {}

    const Symbol::Sort sort;
    const Plies plies;
  };

  struct ClausesWithTerm {
    typedef std::unordered_set<ClauseIndex>::const_iterator clause_iterator;

    struct Begin {
      Begin(Term t, clause_iterator end) : t(t), end(end) {}
      clause_iterator operator()(const Ply& p) const {
        auto it = p.clauses.with_term.find(t);
        return it != p.clauses.with_term.end() ? it->second.begin() : end;
      }
     private:
      Term t;
      clause_iterator end;
    };

    struct End {
      End(Term t, clause_iterator end) : t(t), end(end) {}
      clause_iterator operator()(const Ply& p) const {
        auto it = p.clauses.with_term.find(t);
        return it != p.clauses.with_term.end() ? it->second.end() : end;
      }
     private:
      Term t;
      clause_iterator end;
    };

    typedef internal::flatten_iterator<Plies::iterator, clause_iterator, Begin, End> iterator;

    ClausesWithTerm(const Grounder* owner, Term t, Plies::Policy policy) : owner(owner), plies(owner, policy), t(t) {}

    iterator begin() const { return iterator(plies.begin(), plies.end(), Begin(t, cs.end()), End(t, cs.end())); }
    iterator end()   const { return iterator(plies.end(), plies.end(), Begin(t, cs.end()), End(t, cs.end())); }

   private:
    const Grounder* const owner;
    const Plies plies;
    const Term t;
    const std::unordered_set<ClauseIndex> cs = {};
  };

  struct RelevantTerms {
    typedef std::unordered_set<Term>::const_iterator term_iterator;

    struct Begin { term_iterator operator()(const Ply& p) const { return p.relevant.terms.begin(); } };
    struct End   { term_iterator operator()(const Ply& p) const { return p.relevant.terms.end(); } };

    typedef internal::flatten_iterator<Plies::iterator, term_iterator, Begin, End> iterator;

    iterator begin() const { return iterator(plies.begin(), plies.end()); }
    iterator end()   const { return iterator(plies.end(), plies.end()); }

   private:
    friend class Grounder;

    RelevantTerms(const Grounder* owner, Plies::Policy policy) : owner(owner), plies(owner, policy) {}

    const Grounder* const owner;
    const Plies plies;
  };

  struct RelevantClauses {
    typedef std::unordered_set<ClauseIndex>::const_iterator clause_iterator;

    struct Begin { clause_iterator operator()(const Ply& p) const { return p.relevant.clauses.begin(); } };
    struct End   { clause_iterator operator()(const Ply& p) const { return p.relevant.clauses.end(); } };

    typedef internal::flatten_iterator<Plies::iterator, clause_iterator, Begin, End> iterator;

    iterator begin() const { return iterator(plies.begin(), plies.end()); }
    iterator end()   const { return iterator(plies.end(), plies.end()); }

   private:
    friend class Grounder;

    RelevantClauses(const Grounder* owner, Plies::Policy policy) : plies(owner, policy) {}

    const Plies plies;
  };

  class Undo {
   public:
    Undo() = default;
    Undo(const Undo&) = delete;
    Undo& operator=(const Undo&) = delete;
    Undo(Undo&& u) : owner_(u.owner_) { u.owner_ = nullptr; }
    Undo& operator=(Undo&& u) { owner_ = u.owner_; u.owner_ = nullptr; return *this; }
    ~Undo() {
      if (owner_) {
        owner_->UndoLast();
      }
      owner_ = nullptr;
    }

   private:
    friend class Grounder;

    explicit Undo(Grounder* owner) : owner_(owner) {}

    Grounder* owner_ = nullptr;
  };

  Grounder(Symbol::Factory* sf, Term::Factory* tf)
      : tf_(tf), name_pool_(sf, tf), var_pool_(sf, tf), setup_(new Setup()) {}
  Grounder(const Grounder&) = delete;
  Grounder& operator=(const Grounder&) = delete;
  Grounder(Grounder&&) = default;
  Grounder& operator=(Grounder&&) = default;
  ~Grounder() {
    while (!plies_.empty()) {
      plies_.erase(std::prev(plies_.end()));
    }
  }

  NamePool& temp_name_pool() { return name_pool_; }

  const Setup& setup() const { return *setup_; }

  // 1. AddClause(c):
  // New ply.
  // Add c to ungrounded_clauses.
  // Add new names in c to names.
  // Add variables to vars, generate plus-names.
  // Re-ground.
  //
  // 2. PrepareForQuery(phi):
  // New ply.
  // Add new names in phi to names.
  // Add variables to vars, generate plus-names.
  // Re-ground.
  // Add f(.)=n, f(.)/=n pairs from grounded phi to lhs_rhs.
  //
  // 3. AddUnit(t=n):
  // New ply.
  // Add t=n to ungrounded_clauses.
  // If t=n contains a plus-name, add these to names and generate new plus-names.
  // If n is new, add n to names.
  // If either of the two cases, re-ground.
  //
  // 3. AddUnits(U):
  // New ply.
  // Add U to ungrounded_clauses.
  // If U contains t=n for new n, add n to names and re-ground.
  // (Note: in this case, all literals in U are of the form t'=n.)
  //
  // Re-ground:
  // Ground ungrounded_clauses for names and vars from last ply.
  // Add f(.)=n, f(.)/=n pairs from newly grounded clauses to lhs_rhs.
  // [ Close unit sets from previous AddUnit(U) plies under isomorphism with the names and vars from the last ply. ]
  //
  // We add the plus-names for quantifiers in the query in advance and ground
  // everything with them as if they occurred in the query.
  // So to determine the split and fix names, lhs_rhs suffices.
  //
  // Splits: {t=n | t in terms, n in lhs_rhs[t] or single new one}
  // Fixes: {t=n | t in terms, n in lhs_rhs[t] or single new one} or
  //        for every t in terms, n in lhs_rhs[t]:
  //        {t*=n* | t* in terms, n in lhs_rhs[t] or x in lhs_rhs[t], t=n, t*=n* isomorphic}
  //
  // Isomorphic literals: the bijection for a literal f(n1,...,nK)=n0 should only modify n1,...,nK, but not n0 unless
  // it is contained in n1,...,nK. Otherwise we'd add f(n1,...,nK)=n0, f(n1,...,nK)=n0*, etc. which obviously is
  // inconsistent.

  Setup::Result AddClause(const Clause& c, Undo* undo = nullptr, bool do_not_add_if_inconsistent = false) {
    auto r = internal::singleton_range(c);
    return AddClauses(r.begin(), r.end(), undo, do_not_add_if_inconsistent);
  }

  template<typename InputIt>
  Setup::Result AddClauses(InputIt first,
                           InputIt last,
                           Undo* undo = nullptr,
                           const bool do_not_add_if_inconsistent = false) {
    // Add c to ungrounded_clauses.
    // Add new names in c to names.
    // Add variables to vars, generate plus-names.
    // Re-ground.
    Ply& p = new_ply();
    for (; first != last; ++first) {
      Ungrounded<Clause> uc(*first);
      assert(uc.val.well_formed());
      uc.val.Traverse([this, &p, &uc](Term t) {
        if (t.variable()) {
          uc.vars.insert(t);
        }
        if (t.name()) {
          if (!IsOccurringName(t)) {
            if (IsPlusName(t)) {
              p.names.plus_mentioned.insert(t);
            } else {
              p.names.mentioned.insert(t);
            }
          }
        }
        return true;
      });
      CreateMaxPlusNames(uc.vars, 1);
      p.clauses.ungrounded.push_back(std::move(uc));
    }
    CreateNewPlusNames(p.names.plus_mentioned);
    p.do_not_add_if_inconsistent = do_not_add_if_inconsistent;
    const Setup::Result r = Reground();
    if (undo) {
      *undo = Undo(this);
    }
    return r;
  }

  void PrepareForQuery(const Term t, Undo* undo = nullptr) {
    const Term x = var_pool_.Create(t.sort());
    const Literal a = Literal::Eq(t, x);
    const Formula::Ref phi = Formula::Factory::Atomic(Clause{a});
    PrepareForQuery(*phi, undo);
    var_pool_.Return(x);
  }

  void PrepareForQuery(const Formula& phi, Undo* undo = nullptr) {
    // New ply.
    // Add new names in phi to names.
    // Add variables to vars, generate plus-names.
    // Re-ground.
    // Add f(.)=n, f(.)/=n pairs from grounded phi to lhs_rhs.
    Ply& p = new_ply();
    phi.Traverse([this, &p](const Literal a) {
      Ungrounded<Literal> ua(a.pos() ? a : a.flip());
      a.Traverse([this, &p, &ua](const Term t) {
        if (t.name()) {
          if (!IsOccurringName(t)) {
            if (IsPlusName(t)) {
              p.names.plus_mentioned.insert(t);
            } else {
              p.names.mentioned.insert(t);
            }
          }
        } else if (t.variable()) {
          ua.vars.insert(t);
        }
        return true;
      });
      if (ua.val.lhs().function() && !ua.val.lhs().sort().rigid() && !IsUngroundedLhsRhs(ua, Plies::kAll)) {
        last_ply().lhs_rhs.ungrounded.insert(std::move(ua));
      }
      return true;
    });
    CreateNewPlusNames(p.names.plus_mentioned);
    CreateMaxPlusNames(phi.n_vars());  // XXX or CreateNewPlusNames()?
    Reground();
    if (undo) {
      *undo = Undo(this);
    }
  }

  void GuaranteeConsistency(const Formula& alpha, Undo* undo = nullptr) {
    Ply& p = new_ply();
    p.relevant.filter = true;
    alpha.Traverse([this, &p](const Term t) {
      if (t.quasi_primitive()) {
        Ungrounded<Term> ut(t);
        t.Traverse([&ut](const Term x) { if (x.variable()) { ut.vars.insert(x); } return true; });
        p.relevant.ungrounded.insert(std::move(ut));
      }
      return false;
    });
    for (const Ungrounded<Term>& u : p.relevant.ungrounded) {
      for (const Term g : groundings(&u.val, &u.vars)) {
        p.relevant.terms.insert(g);
      }
    }
    CloseRelevance(Plies::kAll);
    if (undo) {
      *undo = Undo(this);
    }
  }

  void GuaranteeConsistency(Term t, Undo* undo) {
    assert(t.primitive());
    Ply& p = new_ply();
    p.relevant.filter = true;
    p.relevant.ungrounded.insert(Ungrounded<Term>(t));
    p.relevant.terms.insert(t);
    CloseRelevance(Plies::kAll);
    if (undo) {
      *undo = Undo(this);
    }
  }

  void UndoLast() { pop_ply(); }

  void Consolidate() { MergePlies(); }

  Literal Variablify(Literal a) {
    assert(a.ground());
    Term::Vector ns;
    a.lhs().Traverse([&ns](Term t) {
      if (t.name() && std::find(ns.begin(), ns.end(), t) != ns.end()) {
        ns.push_back(t);
      }
      return true;
    });
    return a.Substitute([this, ns](Term t) {
      const size_t i = std::find(ns.begin(), ns.end(), t) - ns.begin();
      return i != ns.size() ? internal::Just(var_pool_.Get(t.sort(), i)) : internal::Nothing;
    }, tf_);
  }

  LhsTerms lhs_terms(Plies::Policy p = Plies::kAll) const { return LhsTerms(this, p); }
  // The additional name must not be used after RhsName's death.
  RhsNames rhs_names(Term t, Plies::Policy p = Plies::kAll) { return RhsNames(this, t, p); }
  Names names(Symbol::Sort sort, Plies::Policy p = Plies::kAll) const { return Names(this, sort, p); }

  ClausesWithTerm clauses_with_term(Term t, Plies::Policy p = Plies::kAll) const { return ClausesWithTerm(this, t, p); }

  bool relevance_filter() const { return !plies_.empty() && last_ply().relevant.filter; }
  RelevantTerms relevant_terms(Plies::Policy p = Plies::kAll) const { return RelevantTerms(this, p); }
  RelevantClauses relevant_clauses(Plies::Policy p = Plies::kAll) const { return RelevantClauses(this, p); }

 private:
  template<typename T>
  struct Groundings {
   public:
    struct Assignments {
     public:
      struct NotX {
        explicit NotX(Term x) : x(x) {}

        bool operator()(Term y) const { return x != y; }

       private:
        const Term x;
      };

      struct DomainCodomain {
        DomainCodomain(const Grounder* owner, Plies::Policy policy) : owner(owner), policy(policy) {}

        std::pair<Term, Names> operator()(const Term x) const {
          return std::make_pair(x, owner->names(x.sort(), policy));
        }

       private:
        const Grounder* const owner;
        const Plies::Policy policy;
      };

      typedef internal::filter_iterator<SortedTermSet::all_values_iterator, NotX> vars_iterator;
      typedef internal::transform_iterator<vars_iterator, DomainCodomain> var_names_iterator;
      typedef internal::mapping_iterator<Term, Names::iterator> iterator;

      Assignments(const Grounder* owner, const SortedTermSet* vars, Plies::Policy policy, Term x, Term n)
          : vars(vars), owner(owner), policy(policy), x(x), n(n) {}

      iterator begin() const {
        auto p = NotX(x);
        auto f = DomainCodomain(owner, policy);
        auto b = vars->begin();
        auto e = vars->end();
        return iterator(var_names_iterator(vars_iterator(b, e, p), f), var_names_iterator(vars_iterator(e, e, p), f));
      }
      iterator end() const { return iterator(); }

     private:
      const SortedTermSet* const vars;
      const Grounder* const owner;
      const Plies::Policy policy;
      const Term x;
      const Term n;
    };

    struct Ground {
      Ground(Term::Factory* tf, const T* obj, Term x, Term n) : tf_(tf), obj(obj), x(x), n(n) {}
      T operator()(const typename Assignments::iterator::value_type& assignment) const {
        auto substitution = [this, &assignment](Term y) { return x == y ? internal::Just(n) : assignment(y); };
        return obj->Substitute(substitution, tf_);
      }
     private:
      Term::Factory* const tf_;
      const T* const obj;
      const Term x;
      const Term n;
    };

    typedef internal::transform_iterator<typename Assignments::iterator, Ground> iterator;

    Groundings(const Grounder* owner, const T* obj, const SortedTermSet* vars, Term x, Term n, Plies::Policy p)
        : x(x), n(n), assignments(owner, vars, p, x, n), ground(owner->tf_, obj, x, n) {}

    iterator begin() const { return iterator(assignments.begin(), ground); }
    iterator end()   const { return iterator(assignments.end(), ground); }

   private:
    Term x;
    Term n;
    Assignments assignments;
    Ground ground;
  };

  template<typename T>
  Groundings<T> groundings(const T* o, const SortedTermSet* vars, Plies::Policy p = Plies::kAll) const {
    return groundings(o, vars, Term(), Term(), p);
  }

  template<typename T>
  Groundings<T> groundings(const T* o, const SortedTermSet* vars, Term x, Term n, Plies::Policy p = Plies::kAll) const {
    return Groundings<T>(this, o, vars, x, n, p);
  }

  Ply& new_ply() {
    const bool filter = relevance_filter();
    plies_.push_back(Ply());
    Ply& p = plies_.back();
    p.clauses.shallow_setup = setup_->shallow_copy();
    p.relevant.filter = filter;
    return p;
  }

  Plies plies(Plies::Policy p = Plies::kAll) const { return Plies(this, p); }

  Ply& last_ply() { assert(!plies_.empty()); return plies_.back(); }
  const Ply& last_ply() const { assert(!plies_.empty()); return plies_.back(); }

  void pop_ply() {
    assert(!plies_.empty());
    Ply& p = last_ply();
    for (const Term n : p.names.plus_max) {
      name_pool_.Return(n);
    }
    for (const Term n : p.names.plus_new) {
      name_pool_.Return(n);
    }
    plies_.pop_back();
  }

  bool IsUngroundedLhsRhs(const Ungrounded<Literal>& ua, Plies::Policy p) const {
    assert(ua.val.lhs().function() && !ua.val.lhs().sort().rigid());
    for (const Ply& p : plies(p)) {
      if (p.lhs_rhs.ungrounded.count(ua) > 0) {
        return true;
      }
    }
    return false;
  }

  bool IsLhsRhs(Literal a) const {
    assert(a.primitive());
    for (const Ply& p : plies_) {
      auto it = p.lhs_rhs.map.find(a.lhs());
      if (it != p.lhs_rhs.map.end() && it->second.count(a.rhs()) > 0) {
        return true;
      }
    }
    return false;
  }

  bool IsRelevantTerm(Term t) const {
    assert(t.ground() && t.function() && !t.sort().rigid());
    if (!relevance_filter()) {
      return true;
    }
    for (const Ply& p : plies_) {
      if (p.relevant.terms.count(t) > 0) {
        return true;
      }
    }
    return false;
  }

  bool IsRelevantClause(ClauseIndex i) const {
    assert(relevance_filter());
    for (const Ply& p : plies_) {
      if (!p.relevant.filter || p.relevant.clauses.count(i) > 0) {
        return true;
      }
    }
    return false;
  }


  size_t nMaxPlusNames(Symbol::Sort sort) const {
    size_t n_names = 0;
    for (const Ply& p : plies_) {
      n_names += p.names.plus_max.n_values(sort);
    }
    return n_names;
  }

  bool IsOccurringName(Term n) const {
    assert(n.name());
    for (const Ply& p : plies_) {
      if (p.names.mentioned.contains(n) || p.names.plus_mentioned.contains(n)) {
        return true;
      }
    }
    return false;
  }

  bool IsPlusName(Term n) const {
    assert(n.name());
    for (const Ply& p : plies_) {
      if (p.names.plus_max.contains(n) || p.names.plus_new.contains(n)) {
        return true;
      }
    }
    return false;
  }

  void CreateMaxPlusNames(const Formula::SortCount& sc) {
    Ply& p = last_ply();
    for (const Symbol::Sort sort : sc.keys()) {
      const size_t need_total = sc[sort];
      if (need_total > 0) {
        const size_t have_already = nMaxPlusNames(sort);
        for (size_t i = have_already; i < need_total; ++i) {
          p.names.plus_max.insert(name_pool_.Create(sort));
        }
      }
    }
  }

  void CreateMaxPlusNames(const SortedTermSet& vars, size_t plus) {
    Ply& p = last_ply();
    for (const Symbol::Sort sort : vars.keys()) {
      size_t need_total = vars.n_values(sort);
      if (need_total > 0) {
        need_total += plus;
        const size_t have_already = nMaxPlusNames(sort);
        for (size_t i = have_already; i < need_total; ++i) {
          p.names.plus_max.insert(name_pool_.Create(sort));
        }
      }
    }
  }

  void CreateNewPlusNames(const SortedTermSet& ts) {
    Ply& p = last_ply();
    for (const Symbol::Sort sort : ts.keys()) {
      size_t need_total = ts.n_values(sort);
      for (size_t i = 0; i < need_total; ++i) {
        p.names.plus_new.insert(name_pool_.Create(sort));
      }
    }
  }

  void UpdateWithTerm(const Clause& c, ClauseIndex i) {
    assert(c.ground());
    assert(c.primitive());
    Ply& p = last_ply();
    for (const Literal a : c) {
      const Term t = a.lhs();
      assert(t.primitive());
      p.clauses.with_term[t].insert(i);
    }
  }

  void UpdateLhsRhs(Literal a) {
    assert(a.ground());
    if (a.lhs().function() && !a.lhs().sort().rigid() && !IsLhsRhs(a)) {
      const Term t = a.lhs();
      const Term n = a.rhs();
      assert(t.ground() && n.name());
      Ply& p = last_ply();
      p.lhs_rhs.map[t].insert(n);
    }
  }

  void UpdateLhsRhs(const Clause& c) {
    for (const Literal a : c) {
      UpdateLhsRhs(a);
    }
  }

  void UpdateRelevantTerms(Term t) {
    assert(t.ground());
    assert(relevance_filter());
    if (t.primitive() && !IsRelevantTerm(t)) {
      last_ply().relevant.terms.insert(t);
    }
  }

  bool UpdateRelevantTerms(const Clause& c) {
    assert(c.ground());
    assert(!c.valid());
    assert(relevance_filter());
    if (c.any([this](const Literal a) { return IsRelevantTerm(a.lhs()); })) {
      c.all([this](const Literal a) {
        UpdateRelevantTerms(a.lhs());
        return true;
      });
      return true;
    } else {
      return false;
    }
  }

  void CloseRelevance(Plies::Policy policy) {
    Ply& p = last_ply();
    assert(p.relevant.filter);
    auto r = relevant_terms();
    std::unordered_set<Term> queue(r.begin(), r.end());
    const Setup& s = *setup_;
    while (!queue.empty()) {
      const auto it = queue.begin();
      const Term t = *it;
      queue.erase(it);
      for (const ClauseIndex i : clauses_with_term(t, policy)) {
        const internal::Maybe<Clause> c = s.clause(i);
        if (c) {
          assert(c.val.MentionsLhs(t));
          if (policy != Plies::kAll || IsRelevantClause(i)) {
            const bool inserted = p.relevant.clauses.insert(i).second;
            if (inserted) {
              p.relevant.clauses.insert(i);
              for (const Literal a : c.val) {
                assert(a.primitive());
                const Term t = a.lhs();
                if (policy != Plies::kAll || !IsRelevantTerm(t)) {
                  const bool inserted = p.relevant.terms.insert(t).second;
                  if (!inserted) {
                    queue.insert(t);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  bool InconsistencyCheck(const Ply& p, const Clause& c) {
    return !p.do_not_add_if_inconsistent || !c.unit() || !setup_->Subsumes(Clause{c[0].flip()});
  }

  template<typename UnaryFunction, typename UnaryPredicate>
  void ForEachGrounding(UnaryFunction range, UnaryPredicate pred, Setup::Result* add_result = nullptr) {
    typedef decltype(range(std::declval<Ply>()).begin()) iterator;
    typedef typename iterator::value_type::value_type value_type;
    for (const Ply& p : plies_) {
      for (const Ungrounded<value_type>& u : range(p)) {
        for (const value_type& g : groundings(&u.val, &u.vars)) {
          assert(g.ground());
          pred(g, p, add_result);
          if (add_result && *add_result == Setup::kInconsistent) {
            return;
          }
        }
      }
    }
  }

  template<typename UnaryFunction, typename UnaryPredicate>
  void ForEachNewGrounding(UnaryFunction range, UnaryPredicate pred, Setup::Result* add_result = nullptr) {
    typedef decltype(range(std::declval<Ply>()).begin()) iterator;
    typedef typename iterator::value_type::value_type value_type;
    for (const Ply& p : plies(Plies::kOld)) {
      for (const Ungrounded<value_type>& u : range(p)) {
        for (const Term x : u.vars) {
          for (const Term n : names(x.sort(), Plies::kNew)) {
            for (const value_type& g : groundings(&u.val, &u.vars, x, n)) {
              assert(g.ground());
              pred(g, p, add_result);
              if (add_result && *add_result == Setup::kInconsistent) {
                return;
              }
            }
          }
        }
      }
    }
    const Ply& p = last_ply();
    for (const Ungrounded<value_type>& u : range(p)) {
      for (const value_type& g : groundings(&u.val, &u.vars)) {
        pred(g, p, add_result);
        if (add_result && *add_result == Setup::kInconsistent) {
          return;
        }
      }
    }
  }

  static void update_result(Setup::Result* add_result, Setup::Result r) {
    if (add_result) {
      switch (r) {
        case Setup::kOk:
          assert(*add_result != Setup::kInconsistent);
          *add_result = r;
          break;
        case Setup::kSubsumed:
          assert(*add_result != Setup::kInconsistent);
          break;
        case Setup::kInconsistent:
          *add_result = r;
          break;
      }
    }
  }

  Setup::Result Reground(bool minimize = false) {
    // Ground old clauses for names from last ply.
    // Ground new clauses for all names.
    // Add f(.)=n, f(.)/=n pairs from newly grounded clauses to lhs_rhs.
    Setup::Result add_result = Setup::kSubsumed;
    Ply& p = last_ply();
    Setup& s = *setup_;
    ForEachNewGrounding(
        [](const Ply& p) -> auto& { return p.clauses.ungrounded; },
        [this, &s](const Clause& c, const Ply& p, Setup::Result* add_result) {
          if (!c.valid() && InconsistencyCheck(p, c)) {
            const Setup::Result r = s.AddClause(c);
            update_result(add_result, r);
          }
        },
        &add_result);
    if (add_result == Setup::kInconsistent) {
      return add_result;
    }
    if (plies_.size() == 1) {
      s.Minimize();
    } else if (minimize) {
      p.clauses.shallow_setup.Minimize();
    }
    for (ClauseIndex i : p.clauses.shallow_setup.new_clauses()) {
      const internal::Maybe<Clause> c = s.clause(i);
      if (c) {
        UpdateLhsRhs(c.val);
        UpdateWithTerm(c.val, i);
      }
    }
    ForEachNewGrounding(
        [](const Ply& p) -> auto& { return p.lhs_rhs.ungrounded; },
        [this](const Literal a, const Ply&, Setup::Result*) {
          UpdateLhsRhs(a);
        });
    if (p.relevant.filter) {
      ForEachNewGrounding(
          [](const Ply& p) { return p.relevant.ungrounded; },
          [this](const Term t, const Ply&, Setup::Result*) {
            UpdateRelevantTerms(t);
          });
      CloseRelevance(Plies::kNew);
    }
    return add_result;
  }

  void MergePlies() {
    assert(!plies_.empty());
    auto p = plies_.begin();
    if (p == plies_.end()) {
      return;
    }
    for (auto it = std::next(p); it != plies_.end(); ++it) {
      assert(!it->do_not_add_if_inconsistent);
      it->clauses.shallow_setup.Immortalize();
      std::move(it->clauses.ungrounded.begin(), it->clauses.ungrounded.end(), std::inserter(p->clauses.ungrounded, p->clauses.ungrounded.end()));
      std::move(it->clauses.with_term.begin(), it->clauses.with_term.end(), std::inserter(p->clauses.with_term, p->clauses.with_term.end()));
      p->names.mentioned.insert(it->names.mentioned);
      p->names.plus_max.insert(it->names.plus_max);
      p->names.plus_new.insert(it->names.plus_new);
      p->names.plus_mentioned.insert(it->names.plus_mentioned);
      p->relevant.filter |= it->relevant.filter;
      std::move(it->relevant.ungrounded.begin(), it->relevant.ungrounded.end(), std::inserter(p->relevant.ungrounded, p->relevant.ungrounded.end()));
      std::move(it->relevant.terms.begin(), it->relevant.terms.end(), std::inserter(p->relevant.terms, p->relevant.terms.end()));
      std::move(it->relevant.clauses.begin(), it->relevant.clauses.end(), std::inserter(p->relevant.clauses, p->relevant.clauses.end()));
      std::move(it->lhs_rhs.ungrounded.begin(), it->lhs_rhs.ungrounded.end(), std::inserter(p->lhs_rhs.ungrounded, p->lhs_rhs.ungrounded.end()));
      for (auto& lhs_rhs : it->lhs_rhs.map) {
        auto lhs = p->lhs_rhs.map.find(lhs_rhs.first);
        if (lhs == p->lhs_rhs.map.end()) {
          p->lhs_rhs.map.insert(std::move(lhs_rhs));
        } else {
          std::move(lhs_rhs.second.begin(), lhs_rhs.second.end(), std::inserter(lhs->second, lhs->second.end()));
        }
      }
    }
#if 0
    if (minimize) {
      p->clauses.full_setup->Minimize();
      p->clauses.shallow_setup = p->clauses.full_setup->shallow_copy();
      // TODO re-compute p->relevant.clauses and p->clauses.with_term, for the indices may have changed
    }
#endif

    plies_.erase(plies_.begin(), p);
    plies_.erase(std::next(p), plies_.end());
    assert(plies_.size() == 1);
  }

  Term::Factory* const tf_;
  NamePool name_pool_;
  VariablePool var_pool_;
  Ply::List plies_;
  std::unique_ptr<Setup> setup_;
};

}  // namespace limbo

#endif  // LIMBO_GROUNDER_H_

