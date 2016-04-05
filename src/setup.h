// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <algorithm>
#include <forward_list>
#include <map>
#include <vector>
#include "./clause.h"
#include "./iter.h"
#include "./range.h"
#include <iostream>
#include "print.h"

namespace lela {

class Setup {
 public:
  typedef int Index;
  typedef std::vector<Clause> ClauseVec;
  typedef std::vector<Index> IndexVec;
  typedef std::multimap<Term, Index> TermMap;

  Setup() = default;
  explicit Setup(const Setup* parent) : parent_(parent), del_(parent_->del_) {}
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  struct ParentSetup {
    const Setup* operator()(const Setup* level) { return level->parent_; }
  };

  struct EnabledClause {
    explicit EnabledClause(const Setup* owner) : owner_(owner) {}
    bool operator()(Index i) const { return !owner_->disabled(i); }
    const Setup* owner_;
  };

  struct UniqueTerm {
    bool operator()(Term t) { const bool b = term_ != t; term_ = t; return b; }
    Term term_;
  };

  struct UnitRange {
    Range<IndexVec::const_iterator> operator()(const Setup* owner) const {
      assert(owner->units_.begin() != owner->units_.end() || owner->units_.begin() == owner->units_.end());
      return MakeRange(owner->units_.begin(), owner->units_.end());
    }
  };

  struct TermOccurrenceRange {
    TermOccurrenceRange() {}
    explicit TermOccurrenceRange(Term t) : term_(t) {}
    Range<TermMap::const_iterator> operator()(const Setup* owner) const {
      return owner->term_occurs(term_);
    }
    Term term_;
  };

  struct TermRange {
    TermRange() {}
    explicit TermRange(Term t) : term_(t) {}
    Range<TermMap::const_iterator> operator()(const Setup* owner) const {
      return MakeRange(owner->occurs_.begin(), owner->occurs_.end());
    }
    Term term_;
  };

  struct GetTerm {
    Term operator()(const TermMap::value_type& pair) const { return pair.first; }
  };

  struct GetClause {
    Index operator()(const TermMap::value_type& pair) const { return pair.second; }
  };

  void AddClause(const Clause& c) {
    assert(!sealed);
    assert(c.primitive());
    cs_.push_back(c);
  }

  void Init() {
    UpdateIndexes();
    PropagateUnits();
    UpdateIndexes();
    Minimize();
    UpdateIndexes();
#ifndef NDEBUG
    sealed = true;
#endif
  }

  bool PossiblyInconsistent() const {
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
            return true;
          }
        }
      }
    }
    return false;
  }

  bool Implies(const Clause& c) const {
    for (Index i : clauses()) {
      if (clause(i).Subsumes(c)) {
        return true;
      }
    }
    return false;
  }

  const Clause& clause(Index i) const {
    assert(0 <= i && i <= to());
    return i >= from() ? cs_[i - from()] : parent_->clause(i);
  }

  Range<FilterIterator<UniqueTerm, TransformIterator<GetTerm, LevelIterator<Setup, ParentSetup, TermRange>>>> primitive_terms() const {
    return MakeRange(filter_iterator(UniqueTerm(), transform_iterator(GetTerm(), level_iterator(ParentSetup(), TermRange(), this))),
                     filter_iterator(UniqueTerm(), transform_iterator(GetTerm(), LevelIterator<Setup, ParentSetup, TermRange>())));
  }

  Range<FilterIterator<EnabledClause, DecrementingIterator<Index>>> clauses() const {
    return MakeRange(filter_iterator(EnabledClause(this), decrementing_iterator(to())),
                     filter_iterator(EnabledClause(this), decrementing_iterator(-1)));
  }

  Range<FilterIterator<EnabledClause, TransformIterator<GetClause, LevelIterator<Setup, ParentSetup, TermOccurrenceRange>>>> clauses_with(Term t) const {
    return MakeRange(filter_iterator(EnabledClause(this), transform_iterator(GetClause(), level_iterator(ParentSetup(), TermOccurrenceRange(t), this))),
                     filter_iterator(EnabledClause(this), transform_iterator(GetClause(), LevelIterator<Setup, ParentSetup, TermOccurrenceRange>())));
  }

  Range<FilterIterator<EnabledClause, LevelIterator<Setup, ParentSetup, UnitRange>>> unit_clauses() const {
    return MakeRange(filter_iterator(EnabledClause(this), level_iterator(ParentSetup(), UnitRange(), this)),
                     filter_iterator(EnabledClause(this), LevelIterator<Setup, ParentSetup, UnitRange>()));
  }

 private:
  void UpdateIndexes() {
    assert(!sealed);
    occurs_.clear();
    units_.clear();
    for (Index i = from(); i <= to(); ++i) {
      if (!disabled(i)) {
        for (Literal a : clause(i)) {
          if (a.lhs().function()) {
            auto r = term_occurs(a.lhs());
            if (r.empty() || std::prev(r.last)->second != i) {
              occurs_.insert(r.last, std::make_pair(a.lhs(), i));
            }
          }
        }
        if (clause(i).unit()) {
          units_.push_back(i);
        }
      }
    }
  }

  void PropagateUnits() {
    assert(!sealed);
    for (Index i : unit_clauses()) {
      assert(clause(i).unit());
      const Literal a = *clause(i).begin();
      assert(a.primitive());
      for (Index j : clauses_with(a.lhs())) {
        Maybe<Clause> c = clause(j).PropagateUnit(a);
        if (c) {
          cs_.push_back(c.val);
          // need to update occurs_ here!
        }
      }
    }
  }

  void Minimize() {
    assert(!sealed);
    for (Index i : clauses()) {
      if (clause(i).valid()) {
        Disable(i);
      } else {
        for (Index j : clauses()) {
          if (i != j && clause(j).Subsumes(clause(i))) {
            Disable(i);
          }
        }
      }
    }
  }

  // Disable() marks a clause as inactive, disabled() indicates whether a clause
  // is inactive, clause() returns a clause; the indexing is global.
  void Disable(Index i) { assert(0 <= i && i <= to()); del_[i] = true; }
  bool disabled(Index i) const { assert(0 <= i && i <= to()); return del_[i]; }

  // from() and to() represent the global indeces this setup represents.
  Index from() const { return from_; }
  Index to() const { return from() + cs_.size() - 1; }

  Range<TermMap::const_iterator> term_occurs(Term t) const {
    // equal_range type sucks
    return MakeRange(occurs_.lower_bound(t), occurs_.upper_bound(t));
  }

  const Setup* parent_ = 0;
  const Index from_ = !parent_ ? 0 : parent_->to() + 1;
#ifndef NDEBUG
  bool sealed = false;
#endif

  // cs_ contains all clauses added in this setup, and occurs_ is their index,
  // and units_ contains the indexes of unit clauses in cs_; the indexing in cs_
  // and units_ is local.
  ClauseVec cs_;
  TermMap occurs_;
  IndexVec units_;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  IntMap<bool, false> del_;
};

}  // namespace lela

#endif  // SRC_SETUP_H_

