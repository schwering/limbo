// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <algorithm>
#include <forward_list>
#include <map>
#include <vector>
#include <boost/iterator/filter_iterator.hpp>
#include "./clause.h"
#include "./iter.h"
#include "./range.h"

namespace lela {

class Setup {
 public:
  typedef int Index;
  typedef std::vector<Clause> ClauseVec;
  typedef std::vector<Index> IndexVec;
  typedef std::map<Term, IndexVec> TermMap;

  struct NextLevel {
    NextLevel() {}
    NextLevel(const Setup* owner) : level_(owner) {}
    const Setup* operator()() { level_ = level_->parent_; return level_; }
    const Setup* level_;
  };

  struct Masked {
    Masked() {}
    Masked(const Setup* owner) : owner_(owner) {}
    bool operator()(Index i) const { return owner_->masked(i); }
    const Setup* owner_;
  };

  struct UnitRange {
    Range<IndexVec::const_iterator> operator()(const Setup* owner) const { return MakeRange(owner->units_.begin(), owner->units_.end()); }
  };

  struct TermOccurrenceRangeGen {
    TermOccurrenceRangeGen(Term t) : term_(t) {}
    Range<IndexVec::const_iterator> operator()(const Setup* owner) const {
      auto it = owner->occurs_.find(term_);
      return MakeRange(it->second.begin(), it->second.end());
    }
    Term term_;
  };

  struct GetClause {
    GetClause(const Setup* owner) : owner_(owner) {}
    const Clause& operator()(Index i) const { return owner_->clause(i); }
    const Setup* owner_;
  };

  struct GetTerm {
    Term operator()(const TermMap::value_type& pair) const { return pair.first; }
  };

  void AddClause(const Clause& c) {
    assert(!sealed);
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
    for (Term t : terms()) {
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

  Range<TransformIterator<GetTerm, TermMap::const_iterator>> terms() const {
    return MakeRange(transform_iterator(GetTerm(), occurs_.begin()),
                     transform_iterator(GetTerm(), occurs_.end()));
  }

  Range<FilterIterator<Masked, DecrementingIterator<Index>>> clauses() const {
    return MakeRange(filter_iterator(Masked(this), decrementing_iterator(to())),
                     filter_iterator(Masked(this), decrementing_iterator(-1)));
  }

  Range<FilterIterator<Masked, LevelIterator<Setup, NextLevel, TermOccurrenceRangeGen>>> clauses_with(Term t) const {
    return MakeRange(filter_iterator(Masked(this), level_iterator(NextLevel(), TermOccurrenceRangeGen(t), this)),
                     filter_iterator(Masked(this), level_iterator(NextLevel(), TermOccurrenceRangeGen(t), this)));
  }

  Range<FilterIterator<Masked, LevelIterator<Setup, NextLevel, UnitRange>>> unit_clauses() const {
    return MakeRange(filter_iterator(Masked(this), level_iterator(NextLevel(), UnitRange(), this)),
                     filter_iterator(Masked(this), LevelIterator<Setup, NextLevel, UnitRange>()));
  }

 private:
  void UpdateIndexes() {
    assert(!sealed);
    occurs_.clear();
    units_.clear();
    for (Index i = from(); i <= to(); ++i) {
      for (Literal a : clause(i)) {
        if (a.lhs().function()) {
          auto& cs = occurs_[a.lhs()];
          if (cs.empty() || cs.back() != i) {
            cs.push_back(i);
          }
        }
      }
      if (clause(i).unit()) {
        units_.push_back(i);
      }
    }
  }

  void PropagateUnits() {
    assert(!sealed);
    for (Index i : unit_clauses()) {
      assert(clause(i).unit());
      const Literal a = clause(i).front();
      assert(a.primitive());
      for (Index j : clauses_with(a.lhs())) {
        Maybe<Clause> c = clause(j).PropagateUnit(a);
        if (c) {
          cs_.push_back(c.val);
          if (c.val.unit()) {
            auto r1 = unit_clauses();
            auto r2 = MakeRange(transform_iterator(GetClause(this), r1.begin()), transform_iterator(GetClause(this), r1.end()));
            if (std::find(r2.begin(), r2.end(), c.val) == r2.end()) {
              const Index k = cs_.size() - 1;
              units_.push_back(k);
            }
          }
        }
      }
    }
  }

  void Minimize() {
    assert(!sealed);
    for (Index i : clauses()) {
      if (clause(i).valid()) {
        Mask(i);
      } else {
        for (Index j : clauses()) {
          if (i != j && clause(i).Subsumes(clause(j))) {
            Mask(j);
          }
        }
      }
    }
  }

  // Mask() masks a clause as inactive, masked() indicates whether a clause is
  // inactive, clause() returns a clause; the indexing is global.
  void Mask(Index i) { assert(0 <= i && i <= to()); del_[i] = true; }
  bool masked(Index i) const { assert(0 <= i && i <= to()); return del_[i]; }
  const Clause& clause(Index i) const { assert(0 <= i && i <= to()); return i >= from() ? cs_[i] : parent_->clause(i); }

  // from() and to() represent the global indeces this setup represents.
  Index from() const { return from_; }
  Index to() const { return from() + cs_.size(); }
  const Index from_ = parent_ ? 0 : parent_->from_ + parent_->cs_.size() + 1;

  Setup* parent_;
#ifndef NDEBUG
  bool sealed = false;
#endif

  // cs_ contains all clauses added in this setup, and occurs_ is their index;
  // the indexing in cs_ is (obviously) local.
  // from() and to() 
  ClauseVec cs_;
  TermMap occurs_;
  IndexVec units_;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  IntMap<bool> del_;
};

}  // namespace lela

#endif  // SRC_SETUP_H_

