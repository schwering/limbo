// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering
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
// consistency checks. The former only investigates clauses that share a one
// of a given set of primitive terms. Typically one wants this set of terms
// to be transitively closed under the terms occurring in setup clauses. It
// is the users responsibility to make sure this condition holds.
//
// Subsumes() checks whether the clause is subsumed by any clause in the setup
// after doing unit propagation; it is hence a sound but incomplete test for
// entailment.
//
// Spawn() creates a new copy of the setup. The clone is lightweight and its
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
// To facilitate fast cloning, every setup is linked to its ancestor. For the
// lists of clauses, unit clauses, and buckets, a child setup only stores its
// difference (that is, its suffix it adds over its parent). Indices for these
// lists hence need to be seen global, not just for the respective suffix
// locally stored in a Setup object. By contrast to these three lists, the bit
// list that masks, which clauses are deleted, is copied for every child setup
// (because the child may remove clauses of their ancestors).
//
// The copy constructor and assignment operators are deleted, not for technical
// reasons, but because it may likely lead to complications with the linked
// structure of setups and therefore hints at a programming error.

#ifndef LELA_SETUP_H_
#define LELA_SETUP_H_

#include <cassert>

#include <algorithm>
#include <unordered_set>
#include <vector>

#include <lela/clause.h>
#include <lela/internal/intmap.h>
#include <lela/internal/ints.h>
#include <lela/internal/iter.h>

#include <iostream>

namespace lela {

namespace format {
namespace output {
std::ostream& operator<<(std::ostream& os, const Literal a);
std::ostream& operator<<(std::ostream& os, const Clause& c);
}
}

class Setup {
 public:
  typedef internal::size_t size_t;
  enum AddResult { kOK, kSubsumed, kInconsistent };

  struct State {
   private:
    friend Setup;

    State(bool empty_clause, size_t n_clauses, size_t n_units) :
        empty_clause(empty_clause),
        n_clauses(n_clauses),
        n_units(n_units) {}

    bool empty_clause;
    size_t n_clauses;
    size_t n_units;
  };

  struct WeakCopy {
    ~WeakCopy() { const_cast<Setup&>(setup_).Restore(state_); }

    const Setup& setup() const { return setup_; }
    operator const Setup&() const { return setup_; }

    AddResult AddUnit(Literal a) { return const_cast<Setup&>(setup_).AddUnit(a); }

   private:
    friend Setup;

    explicit WeakCopy(const Setup& setup) : setup_(setup) {}

    const Setup& setup_;
    State state_ = setup_.Save();
  };

  typedef std::unordered_set<Term> TermSet;

  Setup() = default;
  Setup(const Setup&) = delete;
  Setup& operator=(const Setup&) = delete;
  Setup(Setup&&) = default;
  Setup& operator=(Setup&&) = default;

  State Save() const {
    assert(++saved_ > 0);
    return State(empty_clause_, clauses_.size(), units_.size());
  }

  void Restore(State s) {
    empty_clause_ = s.empty_clause;
    units_.Resize(s.n_units);
    clauses_.Resize(s.n_clauses);
    assert(saved_-- > 0);
  }

  WeakCopy weak_copy() const { return WeakCopy(*this); }

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
        AddResult r = units_.Add(a);
        assert(r != kInconsistent);
      }
    }
    for (size_t i = clauses_.size(); i > 0; --i) {
      Clause c;
      std::swap(c, clauses_[i - 1]);
      c.PropagateUnits(units_.set());
      assert(!c.empty());
      assert(c.size() >= 2 ||
             any_of(units_.vec().begin(), units_.vec().end(), [&c](Literal a) { return a.Subsumes(c.head()); }));
      clauses_.Erase(i - 1);
      if (c.size() >= 2 && !Subsumes(c)) {
        clauses_.Add(c);
      }
    }
    units_.SealOriginalUnits();  // units_.set() has been eliminated from all clauses, no need to be considered in AddUnit()
  }

  AddResult AddClause(Clause c) {
    assert(saved_ == 0);
    units_.UnsealOriginalUnits();  // undo units_.SealOriginalUnits() called by Minimize()
    c.PropagateUnits(units_.set());
    if (c.size() == 0) {
      empty_clause_ = true;
      return kInconsistent;
    } else if (c.size() == 1) {
      AddResult r = AddUnit(c.head());
      empty_clause_ |= r == kInconsistent;
      return r;
    } else {
      clauses_.Add(c);
      return kOK;
    }
  }

  AddResult AddUnit(Literal a) {
    size_t n_propagated = units_.size();
    AddResult r = units_.Add(a);
    empty_clause_ |= r == kInconsistent;
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
            r = units_.Add(c.head());
            empty_clause_ |= r == kInconsistent;
          } else {
            clauses_.Watch(i, c.head(), c.last());
          }
        }
      }
    }
    return r;
  }

  bool Subsumes(const Clause& d) const {
    if (empty_clause_) {
      return true;
    }
    if (d.empty()) {
      return empty_clause_;
    }
    for (size_t i = 0; i < units_.size(); ++i) {
      if (Clause::Subsumes(units_[i], d)) {
        return true;
      }
    }
    if (d.unit() && d.head().pos()) {
      return false;
    }
    return ClausesSubsume(d);
  }

  bool Consistent() const {
    return true;
  }

  bool LocallyConsistent(const TermSet& ts) const {
    return true;
  }

  bool Determines(Term lhs) const { return units_.Determines(lhs); }

  const std::vector<Literal> units() const { return units_.vec(); }

  struct clause_range {
    explicit clause_range(const Setup& s) : end_((s.empty_clause_ ? 1 : 0) + s.units_.size() + s.clauses_.size()) {}
    internal::int_iterator<size_t> begin() const { return internal::int_iterator<size_t>(0); }
    internal::int_iterator<size_t> end()   const { return internal::int_iterator<size_t>(end_); }
   private:
    size_t end_;
  };

  clause_range clauses() const { return clause_range(*this); }

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
  struct Watched {
    Watched() = default;
    Watched(Literal a, Literal b) : a(a), b(b) { assert(a < b); }
    Watched(const Clause& c) : Watched(c.head(), c.last()) { assert(c.size() >= 2); }
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
      clauses_.push_back(c);
      watched_.push_back(c);
    }

    void Add(Clause&& c) {
      assert(c.size() >= 2);
      clauses_.push_back(std::forward<Clause>(c));
      watched_.push_back(clauses_.back());
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

    AddResult Add(Literal a) {
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
      return kOK;
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

    bool Determines(Term t) const {
      assert(t.primitive());
      auto orig_end = vec_.begin() + n_orig_;
      auto orig_begin = std::lower_bound(vec_.begin(), orig_end, Literal::Min(t));
      for (auto it = orig_begin; it != orig_end && t == it->lhs(); ++it) {
        if (it->pos()) {
          return true;
        }
      }
      if (set_.bucket_count() > 0) {
        auto bucket = set_.bucket(Literal::Min(t));
        for (auto it = set_.begin(bucket), end = set_.end(bucket); it != end; ++it) {
          if (it->pos()) {
            return true;
          }
        }
      }
      return false;
    }

    const std::vector<Literal>&                          vec() const { return vec_; }
    const std::unordered_set<Literal, Literal::LhsHash>& set() const { return set_; }

   private:
    std::vector<Literal> vec_;
    std::unordered_set<Literal, Literal::LhsHash> set_;
    size_t n_orig_ = 0;
  };

  bool ClausesSubsume(const Clause& d) const {
    assert(d.size() >= 1 && (d.size() >= 2 || !d.head().pos()));
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

  bool empty_clause_ = false;
  Units units_;
  Clauses clauses_;
#ifndef NDEBUG
  mutable size_t saved_ = 0;
#endif
};

}  // namespace lela

#endif  // LELA_SETUP_H_

