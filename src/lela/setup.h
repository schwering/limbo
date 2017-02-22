// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Setups are collections of primitive clauses, which are added with
// AddClause() and AddUnit(), where the former is more lightweight.
// A setup is not automatically minimal wrt unit propagation and subsumption;
// to ensure minimality, call Minimize().
//
// The typical lifecycle is to create a Setup object, use AddClause() to
// populate it, evaluate queries with Subsumes(), Determines(), Consistent(),
// and LocallyConsistent().
//
// Additionally, ShallowCopy() can be used to add further clauses or unit
// clauses which are automatically removed once the lifecycle of Offspring
// ends. This allows for very cheap backtracking. Note that anything that is
// added to a WeakCopy also occurs in the original Setup. During the lifecycle
// of any WeakCopies, Minimize() must not be called, as it leads to undefined
// behaviour.
//
// Subsumes() checks whether the clause is subsumed by any clause in the setup
// after doing unit propagation; it is hence a sound but incomplete test for
// entailment.
//
// Determines() returns for a given term t a name n such that the setup
// entails [t=n], if such a name exists. In case the setup contains the empty
// clause, the null term is returned, that is, n.null() holds, to indicate
// that the setup entails [t=n] for arbitrary n.
//
// Consistent() and LocallyConsistent() perform a sound but incomplete
// consistency checks. The former only investigates clauses that share a one
// of a given set of primitive terms. Typically one wants this set of terms
// to be transitively closed under the terms occurring in setup clauses. It
// is the users responsibility to make sure this condition holds.
//
// The setup is implemented using watched literals: the empty clause and unit
// clauses are stored separately from clauses with >= 2 literals, and for each
// of these non-degenerated clauses two literals that are not subsumed by any
// unit clause are watched. The watched-literals scheme has two advantages.
// Firstly, it facilitates lazy unit propagation: a new literal is only tested
// to be complementary to either of the watched literals, for only in this case
// the clause can reduce to a unit clause after propagation. Only when the
// clause reduces to a unit clause after propagation with all unit clauses, the
// resulting unit clause is stored -- all non-unit results of unit propagation
// are not stored and re-computed later on demand. The second advantage is that
// since no clause added with AddClause() or AddUnit() is deleted during unit
// propagation, backtracking can be implemented very cheaply: we just need to
// adjust the pointers the last unit clause and the last clause to remove all
// clauses that were added after the point we want to backtrack to. Note that
// the watched literals may have been adjusted after this point; however, they
// in particular satisfy their invariant of not being subsumed by any unit
// clause at this earlier point, so we do not need to adjust them.
//
// The copy constructor and assignment operators are deleted, not for technical
// reasons, but because it may likely lead to complications with the linked
// structure of setups and therefore hints at a programming error.

#ifndef LELA_SETUP_H_
#define LELA_SETUP_H_

#include <cassert>

#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lela/clause.h>
#include <lela/internal/intmap.h>
#include <lela/internal/ints.h>
#include <lela/internal/iter.h>

namespace lela {

class Setup {
 public:
  typedef internal::size_t size_t;

  enum Result { kOk, kSubsumed, kInconsistent };

  class ShallowCopy {
   public:
    ShallowCopy(const ShallowCopy&) = delete;
    ShallowCopy& operator=(const ShallowCopy&) = delete;
    ShallowCopy(ShallowCopy&&) = default;
    ShallowCopy& operator=(ShallowCopy&&) = default;
    ~ShallowCopy() { Die(); }

    operator const Setup&() const { return *setup_; }
    const Setup* operator->() const { return setup_; }
    const Setup& operator*() const { return *setup_; }

    Result AddUnit(Literal a) { return setup_->AddUnit(a); }

    void Die() {
      if (setup_) {
        setup_->empty_clause_ = empty_clause_;
        setup_->units_.Resize(n_units_);
        setup_->clauses_.Resize(n_clauses_);
        setup_ = nullptr;
        assert(setup_->saved_-- > 0);
      }
    }

   private:
    friend Setup;

    explicit ShallowCopy(Setup* s) :
        setup_(s), empty_clause_(s->empty_clause_), n_clauses_(s->clauses_.size()), n_units_(s->units_.size()) {}

    Setup* setup_;
    bool empty_clause_;
    size_t n_clauses_;
    size_t n_units_;
  };

  Setup() = default;
  Setup(const Setup&) = delete;
  Setup& operator=(const Setup&) = delete;
  Setup(Setup&&) = default;
  Setup& operator=(Setup&&) = default;

  ShallowCopy shallow_copy() const { return ShallowCopy(const_cast<Setup*>(this)); }

  void Minimize() {
    assert(saved_ == 0);
    if (empty_clause_) {
      clauses_.Resize(0);
      units_.Resize(0);
      return;
    }
    for (size_t i = 0; i < units_.size(); ++i) {
      Literal a = units_[i];
      if (!a.pos()) {
        units_.Erase(i);
        Result r = units_.Add(a);
        assert(r != kInconsistent), (void) r;
      }
    }
    for (size_t i = clauses_.size(); i > 0; --i) {
      Clause c;
      std::swap(c, clauses_[i - 1]);
      c.PropagateUnits(units_.set());
      assert(!c.empty());
      assert(c.size() >= 2 ||
             any_of(units_.vec().begin(), units_.vec().end(), [&c](Literal a) { return a.Subsumes(c.first()); }));
      clauses_.Erase(i - 1);
      if (c.size() >= 2 && !Subsumes(c)) {
        clauses_.Add(c);
      }
    }
    units_.SealOriginalUnits();  // units_.set() have been eliminated from all clauses, so not needed in AddUnit()
  }

  Result AddClause(Clause c) {
    assert(c.primitive());
    assert(saved_ == 0);
    units_.UnsealOriginalUnits();  // undo units_.SealOriginalUnits() called by Minimize()
    c.PropagateUnits(units_.set());
    if (c.size() == 0) {
      empty_clause_ = true;
      return kInconsistent;
    } else if (c.size() == 1) {
      Result r = AddUnit(c.first());
      empty_clause_ |= r == kInconsistent;
      return r;
    } else {
      clauses_.Add(c);
      return kOk;
    }
  }

  Result AddUnit(Literal a) {
    assert(a.primitive());
    if (empty_clause_) {
      return kInconsistent;
    }
    size_t n_propagated = units_.size();
    Result r = units_.Add(a);
    empty_clause_ = r == kInconsistent;
    for (; n_propagated < units_.size() && r != kInconsistent; ++n_propagated) {
      a = units_[n_propagated];
      for (size_t i = 0; i < clauses_.size() && r != kInconsistent; ++i) {
        if (Literal::Complementary(clauses_.watched(i).a, a) ||
            Literal::Complementary(clauses_.watched(i).b, a)) {
          Clause c = clauses_[i];
          c.PropagateUnits(units_.set());
          if (c.size() == 0) {
            r = kInconsistent;
            empty_clause_ = true;
          } else if (c.size() == 1) {
            r = units_.Add(c.first());
            empty_clause_ = r == kInconsistent;
          } else {
            clauses_.Watch(i, c.first(), c.last());
          }
        }
      }
    }
    return r;
  }

  bool Subsumes(const Clause& c) const {
    assert(c.ground());
    if (empty_clause_) {
      return true;
    }
    if (c.empty()) {
      return false;
    }
    if (!c.primitive()) {
      return c.valid();
    }
    for (size_t i = 0; i < units_.size(); ++i) {
      if (Clause::Subsumes(units_[i], c)) {
        return true;
      }
    }
    if (c.unit() && c.first().pos()) {
      return false;
    }
    return ClausesSubsume(c);
  }

  bool Consistent() const {
    if (empty_clause_) {
      return false;
    }
    std::unordered_set<Literal, Literal::LhsHash> lits;
    for (size_t i : clauses()) {
      const Clause c = clause(i);
      lits.insert(c.begin(), c.end());
    }
    return ConsistentSet(lits);
  }

  bool LocallyConsistent(const std::unordered_set<Term>& ts) const {
    assert(std::all_of(ts.begin(), ts.end(), [](Term t) { return t.primitive(); }));
#if defined(BLOOM)
    internal::BloomSet<Term> bs;
    for (Term t : ts) {
      bs.Add(t);
    }
#endif
    std::unordered_set<Literal, Literal::LhsHash> lits;
    for (size_t i : clauses()) {
      const Clause c = clause(i);
      if (
#if defined(BLOOM)
          bs.PossiblyOverlaps(c.lhs_bloom()) &&
#endif
          std::any_of(c.begin(), c.end(), [&ts](Literal a) { return ts.find(a.lhs()) != ts.end(); })) {
        lits.insert(c.begin(), c.end());
      }
    }
    return ConsistentSet(lits);
  }

  bool contains_empty_clause() const { return empty_clause_; }

  const std::vector<Literal>& units() const { return units_.vec(); }

  internal::Maybe<Term> Determines(Term lhs) const {
    assert(lhs.primitive());
    return empty_clause_ ? internal::Just(Term()) : units_.Determines(lhs);
  }

  struct ClauseRange {
    explicit ClauseRange(const Setup& s) : last((s.empty_clause_ ? 1 : 0) + s.units_.size() + s.clauses_.size()) {}
    internal::int_iterator<size_t> begin() const { return internal::int_iterator<size_t>(0); }
    internal::int_iterator<size_t> end()   const { return internal::int_iterator<size_t>(last); }
   private:
    size_t last;
  };

  ClauseRange clauses() const { return ClauseRange(*this); }

  Clause clause(size_t i) const {
    if (i == 0 && empty_clause_) {
      return Clause();
    }
    i -= empty_clause_ ? 1 : 0;
    if (i < units_.size()) {
      return Clause(units_[i]);
    }
    i -= units_.size();
    Clause c = clauses_[i];
    c.PropagateUnits(units_.set());
    return c;
  }

 private:
  friend ShallowCopy;

  struct Watched {
    Watched() = default;
    Watched(Literal a, Literal b) : a(a), b(b) { assert(a < b); }

    Literal a;
    Literal b;
  };

  class Clauses {
   public:
    const Clause& operator[](size_t i) const { return clauses_[i]; }
    Clause& operator[](size_t i) { return clauses_[i]; }

    Watched watched(size_t i) const { return watched_[i]; }
    Watched& watched(size_t i) { return watched_[i]; }

    void Add(const Clause& c) {
      assert(c.size() >= 2);
      watched_.push_back(Watched(c.first(), c.last()));
      clauses_.push_back(c);
    }

    void Add(Clause&& c) {
      assert(c.size() >= 2);
      watched_.push_back(Watched(c.first(), c.last()));
      clauses_.push_back(std::forward<Clause>(c));
    }

    void Watch(size_t i, Literal a, Literal b) {
      assert(a < b);
      watched_[i] = Watched(a, b);
    }

    size_t size() const {
      assert(clauses_.size() == watched_.size());
      return clauses_.size();
    }

    void Erase(size_t i) {
      std::swap(clauses_[i], clauses_.back());
      std::swap(watched_[i], watched_.back());
      Resize(clauses_.size() - 1);
    }

    void Resize(size_t n) {
      clauses_.resize(n);
      watched_.resize(n);
    }

   private:
    std::vector<Clause> clauses_;
    std::vector<Watched> watched_;
  };

  class Units {
   public:
    Literal operator[](size_t i) const { return vec_[i]; }

    size_t size() const {
      assert(vec_.size() >= set_.size());
      return vec_.size();
    }

    Result Add(Literal a) {
      auto orig_end = vec_.begin() + n_orig_;
      auto orig_begin = std::lower_bound(vec_.begin(), orig_end, Literal::Min(a.lhs()));
      for (auto it = orig_begin; it != orig_end && a.lhs() == it->lhs(); ++it) {
        if (Literal::Complementary(a, *it)) {
          return kInconsistent;
        }
        if (it->Subsumes(a)) {
          return kSubsumed;
        }
      }
      if (set_.bucket_count() > 0) {
        auto bucket = set_.bucket(a);
        for (auto it = set_.begin(bucket), end = set_.end(bucket); it != end; ++it) {
          if (Literal::Complementary(a, *it)) {
            return kInconsistent;
          }
          if (it->Subsumes(a)) {
            return kSubsumed;
          }
        }
      }
      assert(set_.find(a) == set_.end());
      assert(std::find(vec_.begin(), vec_.end(), a) == vec_.end());
      set_.insert(a);
      vec_.push_back(a);
      return kOk;
    }

    void Resize(size_t n) {
      assert(n >= n_orig_);
      for (size_t i = n; i < vec_.size(); ++i) {
        set_.erase(vec_[i]);
      }
      vec_.resize(n);
    }

    void Erase(size_t i) {
      assert(n_orig_ == 0);
      set_.erase(vec_[i]);
      std::swap(vec_[i], vec_.back());
      vec_.resize(vec_.size() - 1);
    }

    void SealOriginalUnits() {
      std::sort(vec_.begin(), vec_.end());
      vec_.erase(std::unique(vec_.begin(), vec_.end()), vec_.end());
      n_orig_ = vec_.size();
      set_.clear();
    }

    void UnsealOriginalUnits() {
      for (size_t i = 0; i < n_orig_; ++i) {
        set_.insert(vec_[i]);
      }
      n_orig_ = 0;
    }

    internal::Maybe<Term> Determines(Term t) const {
      assert(t.primitive());
      auto orig_end = vec_.begin() + n_orig_;
      auto orig_begin = std::lower_bound(vec_.begin(), orig_end, Literal::Min(t));
      for (auto it = orig_begin; it != orig_end && t == it->lhs(); ++it) {
        if (it->pos()) {
          return internal::Just(it->rhs());
        }
      }
      if (set_.bucket_count() > 0) {
        auto bucket = set_.bucket(Literal::Min(t));
        for (auto it = set_.begin(bucket), end = set_.end(bucket); it != end; ++it) {
          if (it->lhs() == t && it->pos()) {
            return internal::Just(it->rhs());
          }
        }
      }
      return internal::Nothing;
    }

    const std::vector<Literal>&                          vec() const { return vec_; }
    const std::unordered_set<Literal, Literal::LhsHash>& set() const { return set_; }

   private:
    std::vector<Literal> vec_;
    std::unordered_set<Literal, Literal::LhsHash> set_;
    size_t n_orig_ = 0;
  };

  bool ClausesSubsume(const Clause& d) const {
    assert(d.size() >= 1 && (d.size() >= 2 || !d.first().pos()));
    for (size_t i = 0; i < clauses_.size(); ++i) {
      if (Clause::Subsumes(clauses_.watched(i).a, clauses_.watched(i).b, d)) {
        Clause c = clauses_[i];
        c.PropagateUnits(units_.set());
        if (Clause::Subsumes(c, d)) {
          return true;
        }
      }
    }
    return false;
  }

  static bool ConsistentSet(const std::unordered_set<Literal, Literal::LhsHash>& lits) {
    for (const Literal a : lits) {
      assert(lits.bucket_count() > 0);
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

  bool empty_clause_ = false;
  Units units_;
  Clauses clauses_;
#ifndef NDEBUG
  mutable size_t saved_ = 0;
#endif
};

}  // namespace lela

#endif  // LELA_SETUP_H_

