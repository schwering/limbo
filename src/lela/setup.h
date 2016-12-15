// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
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

#ifndef LELA_SETUP_H_
#define LELA_SETUP_H_

#include <cassert>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lela/clause.h>
#include <lela/internal/intmap.h>
#include <lela/internal/iter.h>

namespace lela {

class Setup {
 public:
  typedef int Index;
  typedef std::unordered_multimap<Term, Index> TermMap;

  Setup() = default;
  explicit Setup(const Setup* parent) : parent_(parent), del_(parent_->del_) { assert(parent_->sealed_); }

  Index AddClause(const Clause& c) {
    assert(!sealed_);
    assert(c.primitive());
    assert(!c.valid());
    if (parent() != nullptr && parent()->Subsumes(c)) {
      return -1;
    }
    clauses_.push_back(c);
    const Index k = last_clause() - 1;
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

  bool Subsumes(const Clause& c) const {
    assert(!c.valid());
    for (Index b = 0; b < last_bucket(); ++b) {
      if (c.lhs_bloom().PossiblyIncludes(bucket(b))) {
        for (Index i : bucket_clauses(b)) {
          if (clause(i).Subsumes(c)) {
            return true;
          }
        }
      }
    }
    return false;
  }

  bool Consistent() const {
    if (Subsumes(Clause{})) {
      return false;
    }
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

  bool LocallyConsistent(Literal a) const {
    if (Subsumes(Clause{})) {
      return false;
    }
    if (a.valid()) {
      return true;
    }
    if (a.invalid()) {
      return false;
    }
    assert(a.primitive());
    const Term t = a.lhs();
    assert(t.function());
    std::vector<Literal> ls{a};
    for (Index i : clauses_with(t)) {
      for (Literal b : clause(i)) {
        assert(b.rhs().name());
        if (t == b.lhs()) {
          ls.push_back(b);
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
    return std::any_of(c.begin(), c.end(), [this](Literal a) { return LocallyConsistent(a); });
  }

  const Clause& clause(Index i) const {
    assert(0 <= i && i < last_clause());
    return i >= first_clause() ? clauses_[i - first_clause()] : parent()->clause(i);
  }


  struct Setups {
    struct setup_iterator {
      typedef std::ptrdiff_t difference_type;
      typedef const Setup value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::input_iterator_tag iterator_category;

      explicit setup_iterator(const Setup* setup) : setup_(setup) {}

      bool operator==(const setup_iterator& it) const { return setup_ == it.setup_; }
      bool operator!=(const setup_iterator& it) const { return !(*this == it); }

      reference operator*() const { return *setup_; }
      pointer operator->() const { return setup_; }

      setup_iterator& operator++() { setup_ = setup_->parent(); return *this; }
      setup_iterator operator++(int) { auto it = setup_iterator(setup_); operator++(); return it; }

     private:
      const Setup* setup_;
    };

    explicit Setups(const Setup* owner) : owner_(owner) {}

    setup_iterator begin() const { return setup_iterator(owner_); }
    setup_iterator end()   const { return setup_iterator(nullptr); }

   private:
    const Setup* owner_;
  };

  Setups setups() const { return Setups(this); }


  struct AddOffset {
    AddOffset() : owner_(nullptr) {}
    explicit AddOffset(const Setup* owner) : owner_(owner) {}
    Index operator()(Index i) const { return i + (owner_ == nullptr ? 0 : owner_->last_clause()); }
   private:
    const Setup* owner_;
  };

  struct EnabledClause {
    explicit EnabledClause(const Setup* owner) : owner_(owner) {}
    bool operator()(Index i) const { return owner_->enabled(i); }
   private:
    const Setup* owner_;
  };

  internal::filter_iterators<internal::int_iterator<Index, AddOffset>, EnabledClause> clauses() const {
    return internal::filter_range(internal::int_range(0, 0, AddOffset(0), AddOffset(this)), EnabledClause(this));
  }

#if 0
  struct Clauses {
    struct AddOffset {
      AddOffset() : owner_(nullptr) {}
      explicit AddOffset(const Setup* owner) : owner_(owner) {}
      Index operator()(Index i) const { return i + (owner_ == nullptr ? 0 : owner_->last_clause()); }
     private:
      const Setup* owner_;
    };

    struct EnabledClause {
      explicit EnabledClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) const { return owner_->enabled(i); }
     private:
      const Setup* owner_;
    };

    typedef internal::int_iterator<Index, AddOffset> every_clause_iterator;
    typedef internal::filter_iterator<every_clause_iterator, EnabledClause> clause_iterator;

    explicit Clauses(const Setup* owner) : owner_(owner) {}

    clause_iterator begin() const {
      auto first = every_clause_iterator(0);
      auto last  = every_clause_iterator(0, AddOffset(owner_));
      return clause_iterator(first, last, EnabledClause(owner_));
    }
    clause_iterator end() const {
      auto last = every_clause_iterator(0, AddOffset(owner_));
      return clause_iterator(last, last, EnabledClause(owner_));
    }

   private:
    const Setup* owner_;
  };

  Clauses clauses() const { return Clauses(this); }
#endif


  struct UnitClauses {
    struct UnitClause {
      explicit UnitClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) const { return owner_->clause(i).unit(); }
     private:
      const Setup* owner_;
    };

    typedef internal::filter_iterator<Clauses::clause_iterator, UnitClause> unit_clause_iterator;

    explicit UnitClauses(const Setup* owner) : owner_(owner) {}

    unit_clause_iterator begin() const {
      auto c = owner_->clauses();
      return unit_clause_iterator(c.begin(), c.end(), UnitClause(owner_));
    }
    unit_clause_iterator end() const {
      auto c = owner_->clauses();
      return unit_clause_iterator(c.end(), c.end(), UnitClause(owner_));
    }

   private:
    const Setup* owner_;
  };

  UnitClauses unit_clauses() const { return UnitClauses(this); }


  struct PrimitiveTerms {
    struct TermPairs {
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
      bool operator()(const TermMap::value_type& pair) { return owner_->enabled(pair.second); }
     private:
      const Setup* owner_;
    };

    struct GetTerm {
      Term operator()(const TermMap::value_type& pair) const { return pair.first; }
    };

    struct UniqueTerm {
      bool operator()(Term t) { const bool b = (last_ != t); last_ = t; return b; }
     private:
      Term last_;
    };

    typedef Setups::setup_iterator setup_iterator;
    typedef internal::transform_iterator<setup_iterator, GetTermPairs> nested_term_pairs_iterator;
    typedef internal::flatten_iterator<nested_term_pairs_iterator> every_term_pair_iterator;
    typedef internal::filter_iterator<every_term_pair_iterator, EnabledClause> term_pair_iterator;
    typedef internal::transform_iterator<term_pair_iterator, GetTerm> every_term_iterator;
    typedef internal::filter_iterator<every_term_iterator, UniqueTerm> term_iterator;

    explicit PrimitiveTerms(const Setup* owner) : owner_(owner) {}

    term_iterator begin() const {
      auto first = nested_term_pairs_iterator(owner_->setups().begin(), GetTermPairs());
      auto last  = nested_term_pairs_iterator(owner_->setups().end(), GetTermPairs());
      auto first_pair = every_term_pair_iterator(first, last);
      auto last_pair  = every_term_pair_iterator(last, last);
      auto first_filtered_pair = term_pair_iterator(first_pair, last_pair, EnabledClause(owner_));
      auto last_filtered_pair  = term_pair_iterator(last_pair, last_pair, EnabledClause(owner_));
      auto first_term = every_term_iterator(first_filtered_pair, GetTerm());
      auto last_term  = every_term_iterator(last_filtered_pair, GetTerm());
      return term_iterator(first_term, last_term, UniqueTerm());
    }

    term_iterator end() const {
      auto last = nested_term_pairs_iterator(owner_->setups().end(), GetTermPairs());
      auto last_pair = every_term_pair_iterator(last, last);
      auto last_filtered_pair = term_pair_iterator(last_pair, last_pair, EnabledClause(owner_));
      auto last_term = every_term_iterator(last_filtered_pair, GetTerm());
      return term_iterator(last_term, last_term, UniqueTerm());
    }

   private:
    const Setup* owner_;
  };

  PrimitiveTerms primitive_terms() const { return PrimitiveTerms(this); }


  struct ClausesWith {
    struct TermPairs {
      struct EqualsTerm {
        EqualsTerm() = default;
        explicit EqualsTerm(Term t) : term_(t) {}
        bool operator()(TermMap::value_type p) const { return p.first == term_; }
       private:
        Term term_;
      };

      typedef TermMap::const_local_iterator bucket_iterator;
      typedef internal::filter_iterator<bucket_iterator, EqualsTerm> iterator;

      TermPairs(const Setup* owner, Term t) {
        // We avoid TermMap::equal_range() because it has linear complexity in
        // a std::unordered_map.
        const size_t bucket = owner->occurs_.bucket(t);
        bucket_iterator begin = owner->occurs_.begin(bucket);
        bucket_iterator end = owner->occurs_.end(bucket);
        begin_ = iterator(begin, end, EqualsTerm(t));
        end_   = iterator(end, end, EqualsTerm(t));
      }

      iterator begin() const { return begin_; }
      iterator end()   const { return end_; }

     private:
      iterator begin_;
      iterator end_;
    };

    struct GetTermPairs {
      explicit GetTermPairs(Term t) : term_(t) {}
      TermPairs operator()(const Setup& setup) const { return TermPairs(&setup, term_); }
     private:
      Term term_;
    };

    struct GetClauseIndex {
      Index operator()(const TermMap::value_type& pair) const { return pair.second; }
    };

    struct EnabledNoDupeClause {
      explicit EnabledNoDupeClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) { const bool b = (last_ != i); last_ = i; return b && owner_->enabled(i); }
     private:
      const Setup* owner_;
      Index last_ = -1;
    };

    typedef Setups::setup_iterator setup_iterator;
    typedef internal::transform_iterator<setup_iterator, GetTermPairs> nested_term_pairs_iterator;
    typedef internal::flatten_iterator<nested_term_pairs_iterator> every_term_pair_iterator;
    typedef internal::transform_iterator<every_term_pair_iterator, GetClauseIndex> every_clause_with_term_iterator;
    typedef internal::filter_iterator<every_clause_with_term_iterator, EnabledNoDupeClause> clause_with_term_iterator;

    ClausesWith(const Setup* owner, Term t) : owner_(owner), term_(t) {}

    clause_with_term_iterator begin() const {
      auto first = nested_term_pairs_iterator(owner_->setups().begin(), GetTermPairs(term_));
      auto last  = nested_term_pairs_iterator(owner_->setups().end(), GetTermPairs(term_));
      auto first_filter = every_clause_with_term_iterator(every_term_pair_iterator(first, last), GetClauseIndex());
      auto last_filter  = every_clause_with_term_iterator(every_term_pair_iterator(last, last), GetClauseIndex());
      return clause_with_term_iterator(first_filter, last_filter, EnabledNoDupeClause(owner_));
    }

    clause_with_term_iterator end() const {
      auto last = nested_term_pairs_iterator(owner_->setups().end(), GetTermPairs(term_));
      auto last_filter = every_clause_with_term_iterator(every_term_pair_iterator(last, last), GetClauseIndex());
      return clause_with_term_iterator(last_filter, last_filter, EnabledNoDupeClause(owner_));
    }

   private:
    const Setup* owner_;
    Term term_;
  };

  ClausesWith clauses_with(Term t) const { return ClausesWith(this, t); }


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
    for (Index i = first_clause(); i < last_clause(); ++i) {
      AddOccurrences(i);
    }
  }

  void AddOccurrences(Index i) {
    std::unordered_set<Term> funs;
    for (const Literal a : clause(i)) {
      assert(a.lhs().primitive());
      if (funs.insert(a.lhs()).second) {
        occurs_.insert(std::make_pair(a.lhs(), i));
      }
    }
  }

  void Minimize() {
    assert(!sealed_);
    // We only need to remove clauses subsumed by this setup, as the parent is
    // already assumed to be minimal and does not subsume the new clauses.
    for (int i = first_clause(); i < last_clause(); ++i) {
      if (!disabled(i)) {
        RemoveSubsumed(i);
      }
    }
  }

  void RemoveSubsumed(const Index i) {
    const Clause& c = clause(i);
#if 0
    for (Index j : clauses()) {
      if (i != j && c.Subsumes(clause(j))) {
#if 0
        Disable(j);
#else
        if (clause(j).Subsumes(c)) {
          Disable(std::max(i, j));  // because that's better for the occurs_ index
        } else {
          Disable(j);
        }
#endif
      }
    }
#else
    if (c.empty()) {
      for (Index j : clauses()) {
        assert(c.Subsumes(clause(j)));
        if (i != j) {
          Disable(j);
        }
      }
    } else {
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
        internal::Maybe<Clause> c = clause(j).PropagateUnit(a);
        if (c && !Subsumes(c.val)) {
          const Index k = AddClause(c.val);
          assert(k >= 0);
          AddOccurrences(k);  // keep occurrence index up to date
          RemoveSubsumed(k);  // keep setup minimal
        }
      }
    }
  }

  const Setup* parent() const { return parent_; }

  // first_clause() and last_clause() are the global inclusive/exclusive
  // indices of clauses_; clause(i) returns the clause for global index i.
  Index first_clause() const { return first_clause_; }
  Index last_clause()  const { return first_clause_ + clauses_.size(); }

  // first_unit() and last_unit() are the global inclusive/exclusive
  // indices of unit; unit(i) returns the unit clause for global index i.
  Index first_unit() const { return first_unit_; }
  Index last_unit()  const { return first_unit_ + units_.size(); }

  Literal unit(Index i) const {
    assert(0 <= i && i < last_unit());
    return i >= first_unit() ? units_[i - first_unit()] : parent()->unit(i);
  }

  // first_bucket() and last_bucket() are the global inclusive/exclusive
  // indices of buckets_; bucket(i) returns the bucket for global index i;
  // bucket_clauses(i) returns the range of clauses that are grouped in
  // the global bucket index i.
  Index first_bucket() const { return first_bucket_; }
  Index last_bucket()  const { return first_bucket_ + (buckets_.size() + BUCKET_SIZE - 1) / BUCKET_SIZE; }

  internal::BloomSet<Term> bucket(Index i) const {
    assert(0 <= i && i < last_bucket());
    return i >= first_bucket() ? buckets_[i - first_bucket()] : parent()->bucket(i);
  }

  internal::int_iterators<Index> bucket_clauses(Index i) const {
    assert(0 <= i && i < last_bucket());
    if (i >= first_bucket()) {
      const Index first = (i - first_bucket()) / BUCKET_SIZE;
      const Index last  = std::min(first + BUCKET_SIZE, last_clause());
      return internal::int_range(first, last);
    } else {
      return parent()->bucket_clauses(i);
    }
  }

  // Disable() marks a clause as inactive, disabled() and enabled() indicate
  // whether a clause is inactive and active, respectivel, clause() returns a
  // clause; the indexing is global. Note that del_ is copied from the parent
  // setup.
  void Disable(Index i) { assert(0 <= i && i < last_clause()); del_[i] = true; }
  bool disabled(Index i) const { assert(0 <= i && i < last_clause()); return del_[i]; }
  bool enabled(Index i) const { return !disabled(i); }

  const Setup* parent_ = nullptr;
#ifndef NDEBUG
  bool sealed_ = false;
#endif

  // clauses_ contains all clauses added in this setup; the indexing is local.
  std::vector<Clause> clauses_;
  Index first_clause_ = parent_ != nullptr ? parent_->last_clause() : 0;

  // units_ additionally contains all unit clauses_ added in this setup; the
  // indexing is local.
  std::vector<Literal> units_;
  Index first_unit_ = parent_ != nullptr ? parent_->last_unit() : 0;

  // lhs_blooms_[i] is the intersection clauses_[j] for i in [B*i,...,B*(i+1)),
  // where B is BUCKET_SIZE.
  std::vector<internal::BloomSet<Term>> buckets_;
  Index first_bucket_ = parent_ != nullptr ? parent_->last_bucket() : 0;
  static constexpr Index BUCKET_SIZE = 8;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  internal::IntMap<Index, bool> del_;
};

}  // namespace lela

#endif  // LELA_SETUP_H_

