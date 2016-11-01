// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Setups are collections of primitive clauses. Setups are immutable after
// Init() is called.
//
// The typical lifecycle is to create a Setup object, use AddClause() to
// populate it, then call Init(). From then on, behaviour after another
// AddClause() is undefined. After calling Init(), the setup is closed under
// unit propagation and minimized under subsumption.
//
// Consistent() and LocallyConsistent() perform a sound but incomplete
// consistency checks. The former only investigates clauses that share a
// literal and the transitive closure thereof.
//
// Subsumes() checks whether the clause is subsumed by any clause in the setup
// after doing unit propagation; it is hence a sound but incomplete test for
// entailment.
//
// To facilitate fast copying, every setup is linked to its ancestor. The
// clauses of the linked setups are referred to by numbering, starting with the
// root setup. Clauses are deleted by masking them. Consequently, the numbering
// may not be continuous, and deletion has no effect to the ancestors.
//
// Heavy use of iteratores is made to access all clauses, all unit clauses, all
// clauses mentioning a specific primitive term, and all primitive terms. The
// former two are realized with an index from primitive terms to the clauses
// that mention the term. (Note that adding clauses does not invalidate the
// iterators.)
//
// Due to the stratified structure of setups, primitive_terms() usually contains
// duplicates (precisely when a term occurs in clauses stored in two different
// Setup objects).

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <cassert>
#include <algorithm>
#include <map>
#include <vector>
#include "./clause.h"
#include "./intmap.h"
#include "./iter.h"

namespace lela {

class Setup {
 public:
  typedef int Index;
  typedef std::vector<Clause> ClauseVec;
  typedef std::multimap<Term, Index> TermMap;

  Setup() = default;
  explicit Setup(const Setup* parent) : parent_(parent), del_(parent_->del_) { assert(parent_->sealed_); }

  Index AddClause(const Clause& c) {
    assert(!sealed_);
    assert(c.primitive());
    cs_.push_back(c);
    const Index k = last() - 1;
    assert(clause(k) == c);
    return k;
  }

  void Init() {
    InitOccurrences();
    Minimize();
    PropagateUnits();
#ifndef NDEBUG
    sealed_ = true;
#endif
  }

  bool Consistent() const {
    std::vector<Literal> ls;
    for (Term t : primitive_terms()) {
      ls.clear();
      assert(t.function());
      for (Index i : clauses_with(t)) {
        for (Literal a : clause(i)) {
          assert(a.rhs().name());
          if (t == a.lhs()) {
            ls.push_back(a);
          }
        }
      }
      for (auto it = ls.begin(); it != ls.end(); ++it) {
        for (auto jt = std::next(it); jt != ls.end(); ++jt) {
          assert(Literal::Complementary(*it, *jt) == Literal::Complementary(*jt, *it));
          if (Literal::Complementary(*it, *jt)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  bool Subsumes(const Clause& c) const {
    for (Index i : clauses()) {
      if (clause(i).Subsumes(c)) {
        return true;
      }
    }
    return false;
  }

  bool LocallyConsistent(Literal l) const {
    if (Subsumes(Clause{})) {
      return false;
    }
    if (l.valid()) {
      return true;
    }
    if (l.invalid()) {
      return false;
    }
    assert(l.primitive());
    const Term t = l.lhs();
    assert(t.function());
    std::vector<Literal> ls{l};
    for (Index i : clauses_with(t)) {
      for (Literal a : clause(i)) {
        assert(a.rhs().name());
        if (t == a.lhs()) {
          ls.push_back(a);
        }
      }
    }
    for (auto it = ls.begin(); it != ls.end(); ++it) {
      for (auto jt = std::next(it); jt != ls.end(); ++jt) {
        assert(Literal::Complementary(*it, *jt) == Literal::Complementary(*jt, *it));
        if (Literal::Complementary(*it, *jt)) {
          return false;
        }
      }
    }
    return true;
  }

  bool LocallyConsistent(const Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this] (Literal l) { return LocallyConsistent(l); });
  }

  const Clause& clause(Index i) const {
    assert(0 <= i && i < last());
    return i >= first() ? cs_[i - first()] : parent_->clause(i);
  }


  struct Setups {
    struct setup_iterator {
      typedef std::ptrdiff_t difference_type;
      typedef Setup value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::input_iterator_tag iterator_category;

      explicit setup_iterator(const Setup* setup) : setup_(setup) {}

      bool operator==(const setup_iterator& it) const { return setup_ == it.setup_; }
      bool operator!=(const setup_iterator& it) const { return !(*this == it); }

      const value_type& operator*() const { return *setup_; }
      const value_type* operator->() const { return setup_; }

      setup_iterator& operator++() { setup_ = setup_->parent(); return *this; }

     private:
      const Setup* setup_;
    };

    explicit Setups(const Setup* owner) : owner_(owner) {}
    setup_iterator begin() const { return setup_iterator(owner_); }
    setup_iterator end() const { return setup_iterator(nullptr); }

   private:
    const Setup* owner_;
  };

  Setups setups() const { return Setups(this); }


  struct Clauses {
    struct IndexPlusTo {
      IndexPlusTo() : owner_(nullptr) {}
      explicit IndexPlusTo(const Setup* owner) : owner_(owner) {}
      Index operator()() const { return owner_ != nullptr ? owner_->last() : 0; }
     private:
      const Setup* owner_;
    };

    struct EnabledClause {
      explicit EnabledClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) const { return owner_->enabled(i); }
     private:
      const Setup* owner_;
    };

    typedef incr_iterator<IndexPlusTo> every_clause_iterator;
    typedef filter_iterator<EnabledClause, incr_iterator<IndexPlusTo>> clause_iterator;

    explicit Clauses(const Setup* owner) : owner_(owner) {}

    clause_iterator begin() const {
      auto first = every_clause_iterator(IndexPlusTo(0));
      auto last  = every_clause_iterator(IndexPlusTo(owner_));
      return clause_iterator(EnabledClause(owner_), first, last);
    }
    clause_iterator end() const {
      auto last = every_clause_iterator(IndexPlusTo(owner_));
      return clause_iterator(EnabledClause(owner_), last, last);
    }

   private:
    const Setup* owner_;
  };

  Clauses clauses() const { return Clauses(this); }


  struct UnitClauses {
    struct UnitClause {
      explicit UnitClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) const { return owner_->clause(i).unit(); }
     private:
      const Setup* owner_;
    };

    typedef filter_iterator<UnitClause, Clauses::clause_iterator> unit_clause_iterator;

    explicit UnitClauses(const Setup* owner) : owner_(owner) {}

    unit_clause_iterator begin() const {
      auto c = owner_->clauses();
      return unit_clause_iterator(UnitClause(owner_), c.begin(), c.end());
    }
    unit_clause_iterator end() const {
      auto c = owner_->clauses();
      return unit_clause_iterator(UnitClause(owner_), c.end(), c.end());
    }

   private:
    const Setup* owner_;
  };

  UnitClauses unit_clauses() const { return UnitClauses(this); }


  struct PrimitiveTerms {
    struct TermPairs {
      typedef TermMap::const_iterator value_type;
      explicit TermPairs(const Setup* owner) : owner_(owner) {}
      TermMap::const_iterator begin() const { assert(owner_); return owner_->occurs_.begin(); }
      TermMap::const_iterator end()   const { assert(owner_); return owner_->occurs_.end(); }
     private:
      const Setup* owner_;
    };

    struct GetTermPairs {
      TermPairs operator()(const Setup& setup) const { return TermPairs(&setup); }
    };

    struct EnabledClause {
      explicit EnabledClause(const Setup* owner) : owner_(owner) {}
      bool operator()(const TermMap::value_type& pair) const { return owner_->enabled(pair.second); }
     private:
      const Setup* owner_;
    };

    struct GetTerm {
      Term operator()(const TermMap::value_type& pair) const { return pair.first; }
    };

    struct UniqueTerm {
      bool operator()(Term t) { const bool b = term_ != t; term_ = t; return b; }
     private:
      Term term_;
    };

    typedef Setups::setup_iterator setup_iterator;
    typedef transform_iterator<GetTermPairs, setup_iterator> term_pairs_iterator;
    typedef nested_iterator<term_pairs_iterator> every_term_pair_iterator;
    typedef filter_iterator<EnabledClause, every_term_pair_iterator> term_pair_iterator;
    typedef transform_iterator<GetTerm, term_pair_iterator> every_term_iterator;
    typedef filter_iterator<UniqueTerm, every_term_iterator> term_iterator;

    explicit PrimitiveTerms(const Setup* owner) : owner_(owner) {}

    term_iterator begin() const {
      auto first = term_pairs_iterator(GetTermPairs(), owner_->setups().begin());
      auto last  = term_pairs_iterator(GetTermPairs(), owner_->setups().end());
      auto first_pair = every_term_pair_iterator(first, last);
      auto last_pair  = every_term_pair_iterator(last, last);
      auto first_filtered_pair = term_pair_iterator(EnabledClause(owner_), first_pair, last_pair);
      auto last_filtered_pair  = term_pair_iterator(EnabledClause(owner_), last_pair, last_pair);
      auto first_term = every_term_iterator(GetTerm(), first_filtered_pair);
      auto last_term  = every_term_iterator(GetTerm(), last_filtered_pair);
      return term_iterator(UniqueTerm(), first_term, last_term);
    }

    term_iterator end() const {
      auto last  = term_pairs_iterator(GetTermPairs(), owner_->setups().end());
      auto last_pair  = every_term_pair_iterator(last, last);
      auto last_filtered_pair  = term_pair_iterator(EnabledClause(owner_), last_pair, last_pair);
      auto last_term  = every_term_iterator(GetTerm(), last_filtered_pair);
      return term_iterator(UniqueTerm(), last_term, last_term);
    }

   private:
    const Setup* owner_;
  };

  PrimitiveTerms primitive_terms() const { return PrimitiveTerms(this); }


  struct ClausesWith {
    struct TermPairs {
      typedef TermMap::const_iterator value_type;
      TermPairs(const Setup* owner, Term t) : owner_(owner), term_(t) {}
      TermMap::const_iterator begin() const { assert(owner_); return owner_->occurs_.lower_bound(term_); }
      TermMap::const_iterator end()   const { assert(owner_); return owner_->occurs_.upper_bound(term_); }
     private:
      const Setup* owner_;
      Term term_;
    };

    struct GetTermPairs {
      explicit GetTermPairs(Term t) : term_(t) {}
      TermPairs operator()(const Setup& setup) const { return TermPairs(&setup, term_); }
     private:
      Term term_;
    };

    struct GetClause {
      Index operator()(const TermMap::value_type& pair) const { return pair.second; }
    };

    typedef Clauses::EnabledClause EnabledClause;

    typedef Setups::setup_iterator setup_iterator;
    typedef transform_iterator<GetTermPairs, setup_iterator> term_pairs_iterator;
    typedef nested_iterator<term_pairs_iterator> every_term_pair_iterator;
    typedef transform_iterator<GetClause, every_term_pair_iterator> every_clause_term_occurrence_iterator;
    typedef filter_iterator<EnabledClause, every_clause_term_occurrence_iterator> clause_term_occurrence_iterator;

    explicit ClausesWith(const Setup* owner, Term term) : owner_(owner), term_(term) {}

    clause_term_occurrence_iterator begin() const {
      auto first = term_pairs_iterator(GetTermPairs(term_), owner_->setups().begin());
      auto last  = term_pairs_iterator(GetTermPairs(term_), owner_->setups().end());
      auto first_filter = every_clause_term_occurrence_iterator(GetClause(), every_term_pair_iterator(first, last));
      auto last_filter  = every_clause_term_occurrence_iterator(GetClause(), every_term_pair_iterator(last, last));
      return clause_term_occurrence_iterator(EnabledClause(owner_), first_filter, last_filter);
    }

    clause_term_occurrence_iterator end() const {
      auto last = term_pairs_iterator(GetTermPairs(term_), owner_->setups().end());
      auto last_filter = every_clause_term_occurrence_iterator(GetClause(), every_term_pair_iterator(last, last));
      return clause_term_occurrence_iterator(EnabledClause(owner_), last_filter, last_filter);
    }

   private:
    const Setup* owner_;
    Term term_;
  };

  ClausesWith clauses_with(Term term) const { return ClausesWith(this, term); }


  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const {
    for (Index i : clauses()) {
      clause(i).Traverse(f);
    }
  }

 private:
  void InitOccurrences() {
    assert(!sealed_);
    occurs_.clear();
    for (Index i = first(); i < last(); ++i) {
      UpdateOccurrences(i);
    }
  }

  void UpdateOccurrences(Index i) {
    for (const Literal a : clause(i)) {
      assert(a.primitive());
      if (a.lhs().function()) {
        auto r = occurs_.equal_range(a.lhs());
        if (r.first == r.second || std::prev(r.second)->second != i) {
          occurs_.insert(r.second, std::make_pair(a.lhs(), i));
        }
      }
    }
  }

  void Minimize() {
    assert(!sealed_);
    // We only need to remove clauses subsumed by this setup, as the parent is
    // already assumed to be minimal.
    for (int i = first(); i < last(); ++i) {
      if (clause(i).valid()) {
        Disable(i);
      } else {
        RemoveSubsumed(i);
      }
    }
  }

  void RemoveSubsumed(const Index i) {
    const Clause& c = clause(i);
#if 0
    for (Index j : clauses()) {
      if (i != j && c.Subsumes(clause(j))) {
        Disable(j);
#if 0
        if (clause(j).Subsumes(c)) {
          Disable(std::max(i, j));  // because that's better for the occurs_ index
        } else {
          Disable(j);
        }
#endif
      }
    }
#else
    for (const Literal a : c) {
      assert(a.primitive());
      const Term t = a.lhs();
      assert(t.primitive());
      for (Index j : clauses_with(t)) {
        if (i != j && c.Subsumes(clause(j))) {
          Disable(j);
        }
      }
    }
#endif
  }

  void PropagateUnits() {
    assert(!sealed_);
    for (Index i : unit_clauses()) {
      assert(clause(i).unit());
      const Literal a = *clause(i).begin();
      assert(a.primitive());
      for (Index j : clauses_with(a.lhs())) {
        Maybe<Clause> c = clause(j).PropagateUnit(a);
        if (c && !Subsumes(c.val)) {
          const Index k = AddClause(c.val);
          UpdateOccurrences(k);  // keep occurrence index up to date
          RemoveSubsumed(k);  // keep setup minimal
        }
      }
    }
  }

  const Setup* root() const { return parent_ != nullptr ? parent_->parent() : this; }
  const Setup* parent() const { return parent_; }

  // Disable() marks a clause as inactive, disabled() and enabled() indicate
  // whether a clause is inactive and active, respectivel, clause() returns a
  // clause; the indexing is global. Note that del_ is copied from the parent
  // setup.
  void Disable(Index i) { assert(0 <= i && i < last()); del_[i] = true; }
  bool disabled(Index i) const { assert(0 <= i && i < last()); return del_[i]; }
  bool enabled(Index i) const { return !disabled(i); }

  // first() and last() represent the global indices this setup represents;
  // first() is inclusive and last() is exclusive (usual C++ iterator
  // terminology).
  Index first() const { return first_; }
  Index last()  const { return first_ + cs_.size(); }

  const Setup* parent_ = nullptr;
  const Index first_ = parent_ != nullptr ? parent_->last() : 0;
#ifndef NDEBUG
  bool sealed_ = false;
#endif

  // cs_ contains all clauses added in this setup, and occurs_ is their index;
  // the indexing in cs_ is local.
  ClauseVec cs_;
  TermMap occurs_;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  IntMap<Index, bool, false> del_;
};

}  // namespace lela

#endif  // SRC_SETUP_H_

