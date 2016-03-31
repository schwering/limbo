// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <algorithm>
#include <forward_list>
#include <map>
#include <vector>
#include <boost/iterator/filter_iterator.hpp>
#include "./clause.h"
#include "./range.h"

namespace lela {

class Setup {
 public:
  typedef int Index;

  class IndexIterator : std::iterator<std::input_iterator_tag, Index> {
   public:
    static IndexIterator end() { return IndexIterator(-1); }

    explicit IndexIterator(Index max_index) : index_(max_index) {}

    bool operator==(IndexIterator it) const { return index_ == it.index_; }
    bool operator!=(IndexIterator it) const { return !(*this == it); }

    const Index& operator*() const { assert(index_ >= 0); return index_; }

    IndexIterator& operator++() { --index_; return *this; }

    bool empty() const { return index_ == -1; }

   private:
    Index index_;
  };

  template<typename UnaryFunction>
  class LevelIterator : std::iterator<std::input_iterator_tag, Index> {
   public:
    static LevelIterator end() { return LevelIterator(); }

    LevelIterator(UnaryFunction range_generator, const Setup* owner) : range_generator_(range_generator), current_(owner) {
      operator++();
    }

    bool operator==(LevelIterator it) const { return current_ == it.current_ && range_ == it.range_; }
    bool operator!=(LevelIterator it) const { return !(*this == it); }

    Index operator*() const { return *range_.first; }

    LevelIterator& operator++() {
      if (!range_.empty()) {
        ++range_.first;
      } else {
        while (current_ && (range_ = range_generator_.range(current_)).empty()) {
          current_ = current_->parent_;
        }
      }
      return *this;
    }

    bool empty() const { return !current_; }

   private:
    LevelIterator() : current_(0) {}

    UnaryFunction range_generator_;
    const Setup* current_;
    Range<typename UnaryFunction::iterator_type> range_;
  };

  struct TermOccurrenceRange {
    typedef std::vector<Index>::const_iterator iterator_type;

    TermOccurrenceRange() {}
    TermOccurrenceRange(Term t) : term_(t) {}

    Range<iterator_type> range(const Setup* owner) const {
      assert(owner);
      auto it = owner->occurs_.find(term_);
      return MakeRange(it->second.begin(), it->second.end());
    }

    Term term_;
  };

  struct UnitRange {
    typedef std::vector<Index>::const_iterator iterator_type;

    UnitRange() {}

    Range<iterator_type> range(const Setup* owner) const {
      return MakeRange(owner->units_.begin(), owner->units_.end());
    }
  };

  template<typename Iter>
  class UnmaskedIterator : std::iterator<std::input_iterator_tag, Clause> {
   public:
    static UnmaskedIterator end() { return UnmaskedIterator(0, Iter::end()); }

    explicit UnmaskedIterator(const Setup* owner, Iter it) : owner_(owner), iter_(it) { Skip(); }

    bool operator==(UnmaskedIterator it) const { return iter_ == it.iter_; }
    bool operator!=(UnmaskedIterator it) const { return !(*this == it); }

    Index index() const { return *iter_; }
    const Clause& clause() const { assert(!owner_->Mask(index())); return owner_->clause(index()); }

    const Clause& operator*() const { return clause(); }
    const Clause* operator->() const { return &clause(); }

    UnmaskedIterator& operator++() {
      assert(*iter_ >= 0);
      ++iter_;
      Skip();
      return *this;
    }

    bool empty() const { return iter_.empty(); }

   private:
    void Skip() {
      while (iter_ != Iter::end() && owner_->Mask(*iter_)) {
        assert(*iter_ >= 0);
        ++iter_;
      }
    }

    const Setup* const owner_;
    Iter iter_;
  };

  void Minimize() {
    auto r = clauses();
    for (auto it = r.begin(); it != r.end(); ++it) {
      const Index i = it.index();
      for (auto jt = r.begin(); jt != r.end(); ++jt) {
        const Index j = jt.index();
        if (i != j && clause(i).Subsumes(clause(j))) {
          Mask(j);
        }
      }
    }
  }

  void PropagateUnits() {
  }

  Range<UnmaskedIterator<IndexIterator>> clauses() const {
    return MakeRange(UnmaskedIterator<IndexIterator>(this, IndexIterator(to())),
                     UnmaskedIterator<IndexIterator>::end());
  }

  Range<UnmaskedIterator<LevelIterator<TermOccurrenceRange>>> clauses_with(Term t) const {
    return MakeRange(UnmaskedIterator<LevelIterator<TermOccurrenceRange>>(this, LevelIterator<TermOccurrenceRange>(TermOccurrenceRange(t), this)),
                     UnmaskedIterator<LevelIterator<TermOccurrenceRange>>::end());
  }

  Range<UnmaskedIterator<LevelIterator<UnitRange>>> unit_clauses() const {
    return MakeRange(UnmaskedIterator<LevelIterator<UnitRange>>(this, LevelIterator<UnitRange>(UnitRange(), this)),
                     UnmaskedIterator<LevelIterator<UnitRange>>::end());
  }

 protected:
  class BitMap : public std::vector<bool> {
   public:
    using std::vector<bool>::vector;

    reference operator[](size_type pos) {
      if (pos >= size()) {
        resize(pos + 1, false);
      }
      return std::vector<bool>::operator[](pos);
    }

    bool operator[](size_type pos) const {
      return pos < size() ? std::vector<bool>::operator[](pos) : false;
    }
  };

  // Mask() masks a clause as inactive, clause() returns a clause; the indexing
  // is global.
  bool Mask(Index i) const { assert(0 <= i && i <= to()); return del_[i]; }
  const Clause& clause(Index i) const { assert(0 <= i && i <= to()); return i >= from() ? cs_[i] : parent_->clause(i); }

  // from() and to() represent the global indeces this setup represents.
  Index from() const { return from_; }
  Index to() const { return from() + cs_.size(); }
  const Index from_ = parent_ ? 0 : parent_->from_ + parent_->cs_.size() + 1;

  Setup* parent_;

  // cs_ contains all clauses added in this setup, and occurs_ is their index;
  // the indexing in cs_ is (obviously) local.
  // from() and to() 
  std::vector<Clause> cs_;
  std::map<Term, std::vector<Index>> occurs_;
  std::vector<Index> units_;

  // del_ masks all deleted clauses; the number is global, because a clause
  // active in the parent setup may be inactive in this one.
  BitMap del_;
};

#if 0
  void PropagateUnit() {
    std::vector<Literal> units;
    for (const Clause& c : cs_) {
      if (c.unit()) {
        units.push_back(c.front());
      }
    }
    std::vector<Literal> removed;
    for (Literal a : units) {
      assert(a.primitive());
      for (auto c : occurs_[a.lhs()]) {
        c->PropagateUnit(a, std::back_inserter(removed));
        for (Literal b : removed) {
          occurs_[b.lhs()].Disable(c);
        }
        removed.clear();
      }
    }
  }

  bool PossiblyInconsistent() const {
    std::vector<Literal> ls;
    for (auto kv : occurs_) {
      ls.clear();
      Term t = kv.first;
      assert(t.function());
      for (const Clause* c : kv.second) {
        for (Literal a : *c) {
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
    for (const Clause& d : cs_) {
      if (d.Subsumes(c)) {
        return true;
      }
    }
    return false;
  }

  void AddClause(const Clause& c) {
    assert(c.primitive() && !c.valid());
    cs_.Push(c);
    for (Literal a : c) {
      if (a.lhs().function()) {
        auto& cs = occurs_[a.lhs()];
        if (cs.empty() || cs_.top_ref() != cs.top()) {
          cs.Push(cs_.top_ref());
        }
      }
    }
  }

  void RemoveClause(ClauseList::ConstIterator before, ClauseList::ConstRef c) {
    for (Literal a : *c) {
      if (a.lhs().function()) {
        occurs_[a.lhs()].Disable(c);
      }
      assert(a.rhs().name());
    }
    assert(&*std::next(before) == c);
    cs_.EraseAfter(before);
  }
#endif

}  // namespace lela

#endif  // SRC_SETUP_H_

