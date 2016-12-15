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
  explicit Setup(const Setup* parent)
      : parent_(parent), contains_empty_clause_(parent_->contains_empty_clause_), del_(parent_->del_) {}

  void AddClause(const Clause& c) {
    AddUnprocessedClause(c);
    ProcessClauses();
  }

#if 0
  void Init() {
    Minimize();
    PropagateUnits();
  }
#endif

  bool Subsumes(const Clause& c) const {
    if (c.valid()) {
      return true;
    }
    for (Index b : buckets()) {
      if (c.lhs_bloom().PossiblyIncludes(bucket_intersection(b))) {
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
    if (contains_empty_clause_) {
      return false;
    }
#if 0
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
#endif
    return true;
  }

  bool LocallyConsistent(Literal a) const {
#if 0
    if (contains_empty_clause_) {
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
#endif
    return true;
  }

  bool LocallyConsistent(const Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this](Literal a) { return LocallyConsistent(a); });
  }

  const Clause& clause(Index i) const {
    assert(0 <= i && i < last_clause());
    const Setup* s = this;
    while (i < s->first_clause()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_clause() <= i && i < s->last_clause());
    return s->clauses_[i - s->first_clause()];
  }

#if 0
  struct AddOffset {
    AddOffset() : owner_(nullptr) {}
    explicit AddOffset(const Setup* owner) : owner_(owner) {}
    Index operator()(Index i) const { return i + (owner_ == nullptr ? 0 : owner_->last_clause()); }
   private:
    const Setup* owner_;
  };
#endif

  struct Clauses {
    struct EnabledClause {
      explicit EnabledClause(const Setup* owner) : owner_(owner) {}
      bool operator()(Index i) const { return owner_->enabled(i); }
     private:
      const Setup* owner_;
    };
    typedef internal::filter_iterators<internal::int_iterator<Index>, EnabledClause> range;
    typedef range::iterator iterator;

    Clauses(const Setup* owner, Index begin, Index end)
        : r_(internal::filter_range(internal::int_range(begin, end), EnabledClause(owner))) {}

    iterator begin() const { return r_.begin(); }
    iterator end()   const { return r_.end(); }

   private:
     range r_;
  };

  Clauses clauses() const {
    return Clauses(this, 0, last_clause());
    //return internal::filter_range(internal::int_range(0, 0, AddOffset(0), AddOffset(this)), EnabledClause(this));
  }

 private:
  void AddUnprocessedClause(const Clause& c) {
    unprocessed_clauses_.push_back(c);
  }

  void ProcessClauses() {
    while (!unprocessed_clauses_.empty()) {
      const Clause c = unprocessed_clauses_.back();
      unprocessed_clauses_.pop_back();
      if (Subsumes(c)) {
        continue;
      }
      clauses_.push_back(c);
      const Index i = last_clause() - 1;
      assert(c.primitive());
      assert(clause(i) == c);
      if (c.empty()) {
        contains_empty_clause_ = true;
      }
      if (bucket_of_clause(i) >= last_bucket()) {
        buckets_.resize(buckets_.size() + 1, Bucket(c.lhs_bloom()));
      } else {
        buckets_.back().Add(c.lhs_bloom());
      }
      RemoveSubsumed(i);
      PropagateUnits(i);
      if (c.unit()) {
        const Literal a = c.get(0);
        units_.push_back(a);
        for (Index b : buckets()) {
          if (bucket_union(b).PossiblyOverlaps(c.lhs_bloom())) {
            for (Index j : bucket_clauses(b)) {
              const internal::Maybe<Clause> d = clause(j).PropagateUnit(a);
              if (d) {
                AddUnprocessedClause(d.val);
              }
            }
          }
        }
      }
    }
  }

  void RemoveSubsumed(const Index i) {
    const Clause& c = clause(i);
    for (Index b : buckets()) {
      if (bucket_union(b).PossiblyIncludes(c.lhs_bloom())) {
        for (Index j : bucket_clauses(b)) {
          if (i != j && c.Subsumes(clause(j))) {
            Disable(j);
          }
        }
      }
    }
  }

#if 0
  void PropagateUnits() {
    Index first = 0;
    Index last;
    for (;;) {
      first += kBucketSize;
      last = std::min(first + kBucketSize, last_unit());
      if (first >= last) {
        break;
      }
      internal::BloomSet<Term> bs;
      for (Index u : internal::int_range(first, last)) {
        bs.Add(unit(u).lhs());
      }
      for (Index b : buckets()) {
        if (bucket_union(b).PossiblyOverlaps(bs)) {
          for (Index i : bucket_clauses(b)) {
            internal::Maybe<Clause> c = internal::Nothing;
            for (Index u : internal::int_range(first, last)) {
              c = (c ? c.val : clause(i)).PropagateUnit(unit(u));
            }
            if (c && !Subsumes(c.val)) {
              const Index k = AddClause(c.val);
              assert(k >= 0);
              RemoveSubsumed(k);
            }
          }
        }
      }
    }
  }
#endif

  void PropagateUnits(const Index i) {
    const Clause& c = clause(i);
    internal::Maybe<Clause> d = internal::Nothing;
    for (Index u : units()) {
      d = (!d ? c : d.val).PropagateUnit(unit(u));
    }
    if (d) {
      AddUnprocessedClause(d.val);
    }
  }

  // first_clause() and last_clause() are the global inclusive/exclusive
  // indices of clauses_; clause(i) returns the clause for global index i.
  Index first_clause() const { return first_clause_; }
  Index last_clause()  const { return first_clause_ + clauses_.size(); }

  // first_unit() and last_unit() are the global inclusive/exclusive
  // indices of unit; unit(i) returns the unit clause for global index i.
  Index first_unit() const { return first_unit_; }
  Index last_unit()  const { return first_unit_ + units_.size(); }

  internal::int_iterators<Index> units() const {
    return internal::int_range(0, last_unit());
  }

  Literal unit(Index i) const {
    assert(0 <= i && i < last_unit());
    const Setup* s = this;
    while (i < s->first_unit()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_unit() <= i && i < s->last_unit());
    return s->units_[i - s->first_unit()];
  }

  // first_bucket() and last_bucket() are the global inclusive/exclusive
  // indices of buckets_; bucket(i) returns the bucket for global index i;
  // bucket_clauses(i) returns the range of clauses that are grouped in
  // the global bucket index i.
  Index first_bucket() const { return first_bucket_; }
  Index last_bucket()  const { return first_bucket_ + (buckets_.size() + kBucketSize - 1) / kBucketSize; }

  internal::int_iterators<Index> buckets() const { return internal::int_range(0, last_bucket()); }

  struct Bucket {
    explicit Bucket(internal::BloomSet<Term> b) : union_(b), intersection_(b) {}

    void Add(internal::BloomSet<Term> b) {
      union_.Unite(b);
      intersection_.Intersect(b);
    }

    internal::BloomSet<Term> union_;
    internal::BloomSet<Term> intersection_;
  };

  const internal::BloomSet<Term>& bucket_union(Index i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    return s->buckets_[i - s->first_bucket()].union_;
  }

  const internal::BloomSet<Term>& bucket_intersection(Index i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    return s->buckets_[i - s->first_bucket()].intersection_;
  }

  Index bucket_of_clause(Index i) const {
    assert(0 <= i && i < last_clause());
    const Setup* s = this;
    while (i < s->first_clause()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_clause() <= i && i < s->last_clause());
    return s->first_bucket() + (i - s->first_clause()) / kBucketSize;
  }

  Clauses bucket_clauses(Index i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    const Index first = s->first_clause() + (i - s->first_bucket()) / kBucketSize;
    const Index last  = std::min(first + kBucketSize, s->last_clause());
    return Clauses(s, first, last);
  }

  // Disable() marks a clause as inactive, disabled() and enabled() indicate
  // whether a clause is inactive and active, respectivel, clause() returns a
  // clause; the indexing is global. Note that del_ is copied from the parent
  // setup.
  void Disable(Index i) { assert(0 <= i && i < last_clause()); del_[i] = true; }
  bool disabled(Index i) const { assert(0 <= i && i < last_clause()); return del_[i]; }
  bool enabled(Index i) const { return !disabled(i); }

  const Setup* parent_ = nullptr;

  bool contains_empty_clause_ = false;

  // clauses_ contains all clauses added in this setup; the indexing is local.
  std::vector<Clause> clauses_;
  std::vector<Clause> unprocessed_clauses_;
  Index first_clause_ = parent_ != nullptr ? parent_->last_clause() : 0;

  // units_ additionally contains all unit clauses_ added in this setup; the
  // indexing is local.
  std::vector<Literal> units_;
  Index first_unit_ = parent_ != nullptr ? parent_->last_unit() : 0;

  // lhs_blooms_[i] is the intersection clauses_[j] for i in [B*i,...,B*(i+1)),
  // where B is kBucketSize.
  std::vector<Bucket> buckets_;
  Index first_bucket_ = parent_ != nullptr ? parent_->last_bucket() : 0;
  static constexpr Index kBucketSize = 32;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  internal::IntMap<Index, bool> del_;
};

}  // namespace lela

#endif  // LELA_SETUP_H_

