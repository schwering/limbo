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
// clauses which are automatically removed once the lifecycle of ShallowCopy
// ends. This allows for very cheap backtracking. Note that anything that is
// added to a ShallowCopy also occurs in the original Setup. During the
// lifecycle of any ShallowCopies, Minimize() must not be called, as it leads
// to undefined behaviour.
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

#ifndef LIMBO_SETUP_H_
#define LIMBO_SETUP_H_

#include <cassert>

#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/literal.h>
#include <limbo/term.h>

#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>

namespace limbo {

class Setup {
 public:
  typedef internal::size_t size_t;

  enum Result { kOk, kSubsumed, kInconsistent };

  template<typename UnaryFunction = internal::Identity>
  struct ClauseRange {
    typedef internal::int_iterator<size_t, UnaryFunction> iterator;

    explicit ClauseRange(size_t last, UnaryFunction func = UnaryFunction()) : last(last), func(func) {}
    iterator begin() const { return iterator(0, func); }
    iterator end()   const { return iterator(last, func); }

   private:
    size_t last;
    UnaryFunction func;
  };

  class ShallowCopy {
   public:
    struct GlobalIndex {
      GlobalIndex(const Setup* s, bool empty_clause, size_t n_units, size_t n_clauses)
          : s(s), empty_clause(empty_clause), n_units(n_units), n_clauses(n_clauses) {}

      size_t operator()(size_t i) const {
        if (i >= s->empty_clause_ - empty_clause + s->units_.size() - n_units) {
          return empty_clause + n_units + n_clauses + i;
        }
        if (i >= s->empty_clause_ - empty_clause) {
          return empty_clause + n_units + i;
        }
        return i;
      }

     private:
      const Setup* s = nullptr;
      bool empty_clause;
      size_t n_units;
      size_t n_clauses;
    };

    ShallowCopy() = default;
    ShallowCopy(const ShallowCopy&) = delete;
    ShallowCopy& operator=(const ShallowCopy&) = delete;
    ShallowCopy(ShallowCopy&& c) : setup_(c.setup_), data_(c.data_) { c.setup_ = nullptr; }
    ShallowCopy& operator=(ShallowCopy&& c) {
      setup_ = c.setup_;
      data_ = c.data_;
      c.setup_ = nullptr;
      return *this;
    }
    ~ShallowCopy() { Kill(); }

    void Kill() {
      if (setup_) {
        assert(data_.empty_clause + data_.n_clauses + data_.n_units == 0 || setup_->saved_-- > 0);
        setup_->empty_clause_ = data_.empty_clause;
        setup_->units_.Resize(data_.n_units);
        setup_->clauses_.Resize(data_.n_clauses);
        setup_ = nullptr;
      }
    }

    void Immortalize() { setup_ = nullptr; }

    Setup& setup() { return *setup_; }
    const Setup& setup() const { return *setup_; }

    Result AddClause(Clause c) { return setup_->AddClause(c); }
    Result AddUnit(Literal a) { return setup_->AddUnit(a); }

    void Minimize() {
      assert(data_.saved == setup_->saved_);
      setup_->Minimize(data_.n_clauses, data_.n_units);
      assert(data_.n_clauses <= setup_->clauses_.size());
      assert(data_.n_units <= setup_->units_.size());
    }

    ClauseRange<GlobalIndex> new_clauses() const {
      const size_t last =
          setup_->empty_clause_ - data_.empty_clause +
          setup_->units_.size() - data_.n_units +
          setup_->clauses_.size() - data_.n_clauses;
      return ClauseRange<GlobalIndex>(last, GlobalIndex(setup_, data_.empty_clause, data_.n_units, data_.n_clauses));
    }

   private:
    friend Setup;

    struct Data {
      Data() = default;
      Data(bool ec, size_t nc, size_t nu) : empty_clause(ec), n_clauses(nc), n_units(nu) {}
      bool empty_clause = false;
      size_t n_clauses = 0;
      size_t n_units = 0;
#ifndef NDEBUG
      size_t saved = 0;
#endif
    };

    explicit ShallowCopy(Setup* s) : setup_(s), data_(Data(s->empty_clause_, s->clauses_.size(), s->units_.size())) {
      assert(data_.empty_clause + data_.n_clauses + data_.n_units == 0 || ++setup_->saved_ > 0);
#ifndef NDEBUG
      data_.saved = s->saved_;
#endif
    }

    Setup* setup_ = nullptr;
    Data data_;
  };

  Setup() = default;
  Setup(const Setup&) = delete;
  Setup& operator=(const Setup&) = delete;
  Setup(Setup&&) = default;
  Setup& operator=(Setup&&) = default;

  ShallowCopy shallow_copy() { return ShallowCopy(this); }

  void Minimize() {
    Minimize(0, 0);
    units_.SealOriginalUnits();  // units_.set() have been eliminated from all clauses, so not needed in AddUnit()
  }

  Result AddClause(Clause c) {
    assert(c.primitive());
    assert(!c.valid());
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
    assert(!a.valid() && !a.invalid());
    if (empty_clause_) {
      return kInconsistent;
    }
    size_t n_propagated = units_.size();
    const Result r = units_.Add(a);
    empty_clause_ = r == kInconsistent;
    for (; n_propagated < units_.size() && !empty_clause_; ++n_propagated) {
      a = units_[n_propagated];
      for (size_t i = 0; i < clauses_.size() && !empty_clause_; ++i) {
        if (Literal::Complementary(clauses_.watched(i).a, a) ||
            Literal::Complementary(clauses_.watched(i).b, a)) {
          Clause c = clauses_[i];
          c.PropagateUnits(units_.set());
          if (c.size() == 0) {
            empty_clause_ = true;
          } else if (c.size() == 1) {
            empty_clause_ = units_.Add(c.first()) == kInconsistent;
          } else {
            clauses_.Watch(i, c.first(), c.last());
          }
        }
      }
    }
    return empty_clause_ ? kInconsistent : r;
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
#ifdef BLOOM
    internal::BloomSet<Term> bs;
    for (Term t : ts) {
      bs.Add(t);
    }
#endif
    std::unordered_set<Literal, Literal::LhsHash> lits;
    for (size_t i : clauses()) {
      const Clause c = clause(i);
      if (
#ifdef BLOOM
          bs.PossiblyOverlaps(c.lhs_bloom()) &&
#endif
          std::any_of(c.begin(), c.end(), [&ts](Literal a) { return ts.find(a.lhs()) != ts.end(); })) {
        lits.insert(c.begin(), c.end());
      }
    }
    return ConsistentSet(lits);
  }

  bool contains_empty_clause() const { return empty_clause_; }

  const std::unordered_set<Literal, Literal::LhsHash>& units() const { return units_.set(); }
  const std::vector<Clause>& non_units() const { return clauses_.vec(); }

  internal::Maybe<Term> Determines(Term lhs) const {
    assert(lhs.primitive());
    return empty_clause_ ? internal::Just(Term()) : units_.Determines(lhs);
  }

  ClauseRange<> clauses() const { return ClauseRange<>(empty_clause_ + units_.size() + clauses_.size()); }

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

    const std::vector<Clause>& vec() const { return clauses_; }

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
      const auto orig_end = vec_.begin() + n_orig_;
      const auto orig_begin = std::lower_bound(vec_.begin(), orig_end, Literal::Min(a.lhs()));
      for (auto it = orig_begin; it != orig_end && a.lhs() == it->lhs(); ++it) {
        if (Literal::Complementary(a, *it)) {
          return kInconsistent;
        }
        if (it->Subsumes(a)) {
          return kSubsumed;
        }
      }
      if (set_.bucket_count() > 0) {
        const auto bucket = set_.bucket(a);
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
      assert(n == 0 || n >= n_orig_);
      for (size_t i = n; i < vec_.size(); ++i) {
        set_.erase(vec_[i]);
      }
      vec_.resize(n);
      if (n == 0) {
        n_orig_ = 0;
      }
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
      const auto orig_end = vec_.begin() + n_orig_;
      const auto orig_begin = std::lower_bound(vec_.begin(), orig_end, Literal::Min(t));
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
      const size_t bucket = lits.bucket(a);
      const auto begin = lits.begin(bucket);
      const auto end   = lits.end(bucket);
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

  void Minimize(size_t n_clauses, size_t n_units) {
    assert(n_clauses + n_units > 0 || saved_ == 0);
    if (empty_clause_) {
      clauses_.Resize(n_clauses);
      units_.Resize(n_units);
      return;
    }
    for (size_t i = n_units; i < units_.size(); ++i) {
      const Literal a = units_[i];
      if (!a.pos()) {
        units_.Erase(i);
        Result r = units_.Add(a);
        assert(r != kInconsistent), (void) r;
      }
    }
    for (size_t i = clauses_.size(); i > n_clauses; --i) {
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
  }

  bool empty_clause_ = false;
  Units units_;
  Clauses clauses_;
#ifndef NDEBUG
  mutable size_t saved_ = 0;
#endif
};

}  // namespace limbo

#endif  // LIMBO_SETUP_H_

