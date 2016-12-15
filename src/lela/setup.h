// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Setups are collections of primitive clauses. Setups are immutable except
// for AddClause().
//
// The typical lifecycle is to create a Setup object, use AddClause() to
// populate it, evaluate queries with Subsumes(), Consistent(), and
// LocallyConsistent(), and possibly Spawn() new setups.
//
// AddClause() ensures that the setup is closed under unit propagation and
// minimized under subsumption. If AddClause() is called after Spawn(), the
// behaviour is undefined.
//
// Consistent() and LocallyConsistent() perform a sound but incomplete
// consistency checks. The former only investigates clauses that share a
// literal and the transitive closure thereof.
//
// Subsumes() checks whether the clause is subsumed by any clause in the setup
// after doing unit propagation; it is hence a sound but incomplete test for
// entailment.
//
// Spawn() creates a new copy of the setup. The copy is lightweight and its
// livetime is limited by its parent's lifetime. If AddClause() is called after
// Spawn(), the behaviour is undefined.
//
// A Setup consists of a list of clauses, a list of unit clauses, a list of
// buckets, and a list of bits. The bits indicate which elements of the list
// of clauses are marked as deleted (because they are subsumed by another
// clause). A list of unit clauses is managed additionally to the general list
// of clauses to speed up unit propagation. A bucket groups a subsequences of
// a fixed length of the clause list and combines their BloomSets. Iterating
// over the bucket list instead of the clause list thus allows to soundly
// filter non-candidates for unit propagation, subsumption, and similar tasks.
//
// To facilitate fast copying, every setup is linked to its ancestor. For the
// lists of clauses, unit clauses, and buckets, a child setup only stores its
// difference (that is, its suffix it adds over its parent). Indices for these
// lists hence need to be seen global, not just for the respective suffix
// locally stored in a Setup object. By contrast to these three lists, the bit
// list that masks, which clauses are deleted, is copied for every child setup
// (because the child may remove clauses of their ancestors).

#ifndef LELA_SETUP_H_
#define LELA_SETUP_H_

#include <cassert>

#include <bitset>
#include <algorithm>
#include <unordered_set>
#include <vector>

#include <lela/clause.h>
#include <lela/internal/intmap.h>
#include <lela/internal/iter.h>

static int POS = 0;
static int NEG = 0;
static int TRUE_POS = 0;
static uint64_t COUNT = 0;

namespace lela {

class Setup {
 public:
  typedef int Index;
  typedef Index ClauseIndex;
  typedef Index UnitIndex;
  typedef Index BucketIndex;

  Setup() = default;

  Setup Spawn() const {
    return Setup(this);
  }

  void AddClause(const Clause& c) {
#ifndef NDEBUG
    assert(!spawned_);
#endif
    AddUnprocessedClause(c);
    ProcessClauses();
  }

  bool Subsumes(const Clause& c) const {
    if (c.valid()) {
      return true;
    }
    for (BucketIndex b : buckets()) {
      std::uint64_t x = *reinterpret_cast<const uint64_t*>(&bucket_intersection(b));
      COUNT += std::bitset<64>(x).count();
      std::cout << std::bitset<64>(x).count() << " " << std::bitset<64>(x) << std::endl;
      //std::cout << std::bitset<64>(*reinterpret_cast<const uint64_t*>(&bucket_union(b))) << std::endl;
      //std::cout << std::endl;
      if (c.lhs_bloom().PossiblyIncludes(bucket_intersection(b))) {
        ++POS;
        for (ClauseIndex i : bucket_clauses(b)) {
          if (clause(i).Subsumes(c)) {
            ++TRUE_POS;
            return true;
          }
        }
      }
      else ++NEG;
    }
    return false;
  }

  bool Consistent() const {
    if (contains_empty_clause_) {
      return false;
    }
    std::unordered_set<Term> primitive_terms;
    for (ClauseIndex i : clauses()) {
      for (Literal a : clause(i)) {
        primitive_terms.insert(a.lhs());
      }
    }
    return std::all_of(primitive_terms.begin(), primitive_terms.end(), [this](Term t) { return LocallyConsistent(t); });
  }

  bool LocallyConsistent(Literal a) const {
    assert(!contains_empty_clause_);
    if (a.invalid()) {
      return false;
    }
    if (a.valid()) {
      return true;
    }
    return LocallyConsistent(a.lhs(), LiteralSet{a});
  }

  bool LocallyConsistent(const Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this](Literal a) { return LocallyConsistent(a); });
  }


  struct Clauses {
    struct EnabledClause {
      explicit EnabledClause(const Setup* owner) : owner_(owner) {}
      bool operator()(ClauseIndex i) const { return owner_->enabled(i); }
     private:
      const Setup* owner_;
    };
    typedef internal::filter_iterators<internal::int_iterator<ClauseIndex>, EnabledClause> range;
    typedef range::iterator iterator;

    Clauses(const Setup* owner, ClauseIndex begin, ClauseIndex end) :
        r_(internal::filter_range(internal::int_range(begin, end), EnabledClause(owner))) {}

    iterator begin() const { return r_.begin(); }
    iterator end()   const { return r_.end(); }

   private:
     range r_;
  };

  Clauses clauses() const {
    return Clauses(this, 0, last_clause());
  }

  const Clause& clause(ClauseIndex i) const {
    assert(0 <= i && i < last_clause());
    const Setup* s = this;
    while (i < s->first_clause()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_clause() <= i && i < s->last_clause());
    return s->clauses_[i - s->first_clause()];
  }


 private:
  struct Bucket {
    explicit Bucket(internal::BloomSet<Term> b) : union_(b), intersection_(b) {}

    void Add(internal::BloomSet<Term> b) {
      union_.Unite(b);
      intersection_.Intersect(b);
    }

    internal::BloomSet<Term> union_;
    internal::BloomSet<Term> intersection_;
  };

  struct LhsHasher {
    size_t operator()(const Literal a) const { return a.lhs().hash(); }
  };

  typedef std::unordered_set<Literal, LhsHasher> LiteralSet;

  explicit Setup(const Setup* parent) :
      parent_(parent),
      contains_empty_clause_(parent_->contains_empty_clause_),
      del_(parent_->del_)
  {
#ifndef NDEBUG
    parent->spawned_ = true;
#endif
  }

  void AddUnprocessedClause(const Clause& c) {
    unprocessed_clauses_.push_back(c);
  }

  void ProcessClauses() {
    while (!unprocessed_clauses_.empty()) {
      Clause c = unprocessed_clauses_.back();
      unprocessed_clauses_.pop_back();
      if (Subsumes(c)) {
        continue;
      }
      c = PropagateUnits(c);
      clauses_.push_back(c);
      const ClauseIndex i = last_clause() - 1;
      assert(c.primitive());
      assert(clause(i) == c);
      if (c.empty()) {
        contains_empty_clause_ = true;
      }
      if (clause_bucket(i) >= last_bucket()) {
        buckets_.resize(buckets_.size() + 1, Bucket(c.lhs_bloom()));
      } else {
        buckets_.back().Add(c.lhs_bloom());
      }
      RemoveSubsumed(i);
      if (c.unit()) {
        const Literal a = c.get(0);
        units_.push_back(a);
        for (BucketIndex b : buckets()) {
          if (bucket_union(b).PossiblyOverlaps(c.lhs_bloom())) {
            //++POS;
            for (ClauseIndex j : bucket_clauses(b)) {
              const internal::Maybe<Clause> d = clause(j).PropagateUnit(a);
              if (d) {
            //++TRUE_POS;
                AddUnprocessedClause(d.val);
              }
            }
          }
          //else ++NEG;
        }
      }
    }
  }

  void RemoveSubsumed(const ClauseIndex i) {
    const Clause& c = clause(i);
    for (BucketIndex b : buckets()) {
      if (bucket_union(b).PossiblyIncludes(c.lhs_bloom())) {
        //++POS;
        for (ClauseIndex j : bucket_clauses(b)) {
          if (i != j && c.Subsumes(clause(j))) {
            //++TRUE_POS;
            Disable(j);
          }
        }
      }
      //else ++NEG;
    }
  }

  Clause PropagateUnits(Clause c) {
    for (UnitIndex u : units()) {
      internal::Maybe<Clause> d = c.PropagateUnit(unit(u));
      if (d) {
        c = d.val;
      }
    }
    return c;
  }


  bool LocallyConsistent(Term t, LiteralSet lits = LiteralSet()) const {
    internal::BloomSet<Term> bs;
    bs.Add(t);
    for (BucketIndex b : buckets()) {
      if (bucket_union(b).PossiblyOverlaps(bs)) {
        //++POS;
        for (ClauseIndex i : bucket_clauses(b)) {
          for (Literal a : clause(i)) {
            if (t == a.lhs()) {
            //++TRUE_POS;
              lits.insert(a);
            }
          }
        }
      }
      //else ++NEG;
    }
    for (const Literal a : lits) {
      size_t b = lits.bucket(a);
      auto begin = lits.begin(b);
      auto end   = lits.end(b);
      for (auto it = begin; it != end; ++it) {
        const Literal b = *it;
        assert(Literal::Complementary(a, b) == Literal::Complementary(b, a));
        if (Literal::Complementary(a, b)) {
          return false;
        }
      }
    }
    return true;
  }


  ClauseIndex first_clause() const { return first_clause_; }
  ClauseIndex last_clause()  const { return first_clause_ + clauses_.size(); }

  UnitIndex first_unit() const { return first_unit_; }
  UnitIndex last_unit()  const { return first_unit_ + units_.size(); }

  BucketIndex first_bucket() const { return first_bucket_; }
  BucketIndex last_bucket()  const { return first_bucket_ + buckets_.size(); }

  void Disable(ClauseIndex i) { assert(0 <= i && i < last_clause()); del_[i] = true; }
  bool enabled(ClauseIndex i) const { assert(0 <= i && i < last_clause()); return !del_[i]; }


  internal::int_iterators<UnitIndex> units() const {
    return internal::int_range(0, last_unit());
  }

  Literal unit(UnitIndex i) const {
    assert(0 <= i && i < last_unit());
    const Setup* s = this;
    while (i < s->first_unit()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_unit() <= i && i < s->last_unit());
    return s->units_[i - s->first_unit()];
  }


  internal::int_iterators<BucketIndex> buckets() const { return internal::int_range(0, last_bucket()); }

  const internal::BloomSet<Term>& bucket_union(BucketIndex i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    return s->buckets_[i - s->first_bucket()].union_;
  }

  const internal::BloomSet<Term>& bucket_intersection(BucketIndex i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    return s->buckets_[i - s->first_bucket()].intersection_;
  }

  Clauses bucket_clauses(BucketIndex i) const {
    assert(0 <= i && i < last_bucket());
    const Setup* s = this;
    while (i < s->first_bucket()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_bucket() <= i && i < s->last_bucket());
    const ClauseIndex first = s->first_clause() + (i - s->first_bucket()) * kBucketSize;
    const ClauseIndex last  = std::min(first + kBucketSize, s->last_clause());
    return Clauses(this, first, last);
  }

  BucketIndex clause_bucket(ClauseIndex i) const {
    assert(0 <= i && i < last_clause());
    const Setup* s = this;
    while (i < s->first_clause()) {
      assert(s->parent_);
      s = s->parent_;
    }
    assert(s->first_clause() <= i && i < s->last_clause());
    return s->first_bucket() + (i - s->first_clause()) / kBucketSize;
  }

  const Setup* parent_ = nullptr;

  bool contains_empty_clause_ = false;

  std::vector<Clause> clauses_;
  ClauseIndex first_clause_ = parent_ != nullptr ? parent_->last_clause() : 0;

  std::vector<Clause> unprocessed_clauses_;

  internal::IntMap<ClauseIndex, bool> del_;

  std::vector<Literal> units_;
  UnitIndex first_unit_ = parent_ != nullptr ? parent_->last_unit() : 0;

  std::vector<Bucket> buckets_;
  BucketIndex first_bucket_ = parent_ != nullptr ? parent_->last_bucket() : 0;
  static constexpr BucketIndex kBucketSize = 8;

#ifndef NDEBUG
  mutable bool spawned_ = false;
#endif
};

}  // namespace lela

#endif  // LELA_SETUP_H_

