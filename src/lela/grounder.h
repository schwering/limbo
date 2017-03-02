// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A Grounder determines how many standard names need to be substituted for
// variables in a proper+ knowledge base and in queries.
//
// AddClause() and PrepareForQuery() determine the names and split terms that
// need to be considered when proving whether the added clauses entail a
// query.
//
// The Ground() method aims to avoid unnecessary regrounding of all clauses.
// To this end, we distinguish internally between processed and unprocessed
// clauses. A call to Ground() only grounds the unprocessed clauses and adds
// them to a new Setup, which inherits the other clauses from the previous
// result of Ground(). The unprocessed clauses include those which were added
// with AddClause(). In case new names have been added due to AddClause() or
// PrepareForQuery(), the unprocessed clauses include all added clauses.
//
// Sometimes names are used temporarily in queries. For that purpose, Grounder
// offers CreateName() and ReturnName() as a layer on-top of Term::Factory and
// Symbol::Factory. Returning such temporary names for later re-use may avoid
// bloating up the setups.

#ifndef LELA_GROUNDER_H_
#define LELA_GROUNDER_H_

#include <cassert>

#include <algorithm>
#include <list>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <lela/clause.h>
#include <lela/formula.h>
#include <lela/setup.h>

#include <lela/internal/hash.h>
#include <lela/internal/ints.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>

namespace lela {

class Grounder {
 public:
  typedef internal::size_t size_t;
  typedef Formula::split_level split_level;
  typedef Formula::TermSet TermSet;
  typedef std::unordered_set<Literal, Literal::LhsHash> LiteralSet;

  struct LhsSymbolHasher {
    internal::hash32_t operator()(const LiteralSet& set) const {
      assert(!set.empty());
      assert(std::all_of(set.begin(), set.end(), [&set](Literal a) { return
             std::all_of(set.begin(), set.end(), [a](Literal b) { return
                         a.lhs().symbol() == b.lhs().symbol(); }); }));
      return set.begin()->lhs().symbol().hash();
    }
  };

  struct LiteralSetHash {
    internal::hash32_t operator()(const LiteralSet& set) const {
      internal::hash32_t h = 0;
      for (Literal a : set) {
        h ^= a.hash();
      }
      return h;
    }
  };

  typedef std::unordered_set<LiteralSet, LiteralSetHash> LiteralAssignmentSet;

  class SortedTermSet : public internal::IntMap<Symbol::Sort, TermSet> {
   public:
    typedef TermSet::value_type value_type;

    using internal::IntMap<Symbol::Sort, TermSet>::IntMap;

    size_t insert(Term t) {
      auto p = (*this)[t.sort()].insert(t);
      return p.second ? 1 : 0;
    }

    void erase(Term t) {
      (*this)[t.sort()].erase(t);
    }

    size_t insert(const TermSet& terms) {
      size_t n = 0;
      for (Term t : terms) {
        n += insert(t);
      }
      return n;
    }

    size_t insert(const SortedTermSet& terms) {
      size_t n = 0;
      for (const TermSet& set : terms.values()) {
        for (Term t : set) {
          n += insert(t);
        }
      }
      return n;
    }

    bool contains(Term t) const {
      const TermSet& ts = (*this)[t.sort()];
      return ts.find(t) != ts.end();
    }
  };

  Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
  Grounder(const Grounder&) = delete;
  Grounder& operator=(const Grounder&) = delete;
  Grounder(Grounder&&) = default;
  Grounder& operator=(Grounder&&) = default;

  typedef std::list<Clause>::const_iterator clause_iterator;
  typedef internal::joined_iterators<clause_iterator, clause_iterator> clause_range;

  clause_range clauses() const {
    return internal::join_ranges(processed_clauses_.cbegin(), processed_clauses_.cend(),
                                 unprocessed_clauses_.cbegin(), unprocessed_clauses_.cend());
  }

  void AddClause(const Clause& c) {
    assert(std::all_of(c.begin(), c.end(),
                       [](Literal a) { return a.quasiprimitive() || (!a.lhs().function() && !a.rhs().function()); }));
    if (c.valid()) {
      return;
    }
    names_changed_ |= AddMentionedNames(Mentioned<SortedTermSet>([](Term t) { return t.name(); }, c));
    names_changed_ |= AddPlusNames(PlusNames(c));
    AddSplitTerms(Mentioned<TermSet>([](Term t) { return t.quasiprimitive(); }, c));
    AddAssignmentLiterals(Mentioned<LiteralSet>([](Literal a) { return a.quasiprimitive(); }, c));
    unprocessed_clauses_.push_front(c);
  }

  void PrepareForQuery(split_level k, const Formula& phi) {
    assert(phi.objective());
    names_changed_ |= AddMentionedNames(Mentioned<SortedTermSet>([](Term t) { return t.name(); }, phi));
    names_changed_ |= AddPlusNames(PlusNames(phi));
    if (k > 0) {
      AddSplitTerms(Mentioned<TermSet>([](Term t) { return t.function(); }, phi));
      AddAssignmentLiterals(Mentioned<LiteralSet>([](Literal a) { return a.lhs().function(); }, phi));
    }
  }

  void PrepareForQuery(split_level k, Term lhs) {
    names_changed_ |= AddMentionedNames(Mentioned<SortedTermSet>([](Term t) { return t.name(); }, lhs));
    names_changed_ |= AddPlusNames(PlusNames(lhs));
    if (k > 0) {
      AddSplitTerms(Mentioned<TermSet>([](Term t) { return t.function(); }, lhs));
    }
  }

  const Setup& Ground() const { return const_cast<Grounder*>(this)->Ground(); }

  const Setup& Ground() {
    if (names_changed_) {
      // Re-ground all clauses, i.e., all clauses are considered unprocessed and all old setups are forgotten.
      unprocessed_clauses_.splice(unprocessed_clauses_.begin(), processed_clauses_);
      setup_ = internal::Nothing;
      assert(processed_clauses_.empty());
    }
    if (!unprocessed_clauses_.empty() || !setup_) {
      if (!setup_) {
        setup_ = internal::Just(Setup());
      }
      for (const Clause& c : unprocessed_clauses_) {
        if (c.ground()) {
          assert(c.primitive());
          if (!c.valid()) {
            setup_.val.AddClause(c);
          }
        } else {
          const TermSet vars = Mentioned<TermSet>([](Term t) { return t.variable(); }, c);
          for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
            const Clause ci = c.Substitute(mapping, tf_);
            if (!ci.valid()) {
              assert(ci.primitive());
              setup_.val.AddClause(ci);
            }
          }
        }
      }
      processed_clauses_.splice(processed_clauses_.begin(), unprocessed_clauses_);
      names_changed_ = false;
      setup_.val.Minimize();
    }
    assert(bool(setup_));
    return setup_.val;
  }

  const SortedTermSet& Names() const { return names_; }

  Term CreateName(Symbol::Sort sort) {
    TermSet& ns = owned_names_[sort];
    if (ns.empty()) {
      return tf_->CreateTerm(sf_->CreateName(sort));
    } else {
      auto it = ns.begin();
      const Term n = *it;
      ns.erase(it);
      return n;
    }
  }

  void ReturnName(Term n) {
    assert(n.name());
    owned_names_.insert(n);
  }

  TermSet SplitTerms() const {
    return Ground(splits_);
  }

  TermSet RelevantSplitTerms(const Formula& phi) {
    assert(phi.objective());
    const Setup& s = Ground();
    TermSet queue = Ground(Mentioned<TermSet>([](Term t) { return t.function(); }, phi));
    for (auto it = queue.begin(); it != queue.end(); ) {
      if (s.Determines(*it)) {
        it = queue.erase(it);
      } else {
        ++it;
      }
    }
    TermSet splits;
    RelevantClosure(s, &queue, &splits, [](const Clause& c, Term t, TermSet* queue) {
      if (c.MentionsLhs(t)) {
        TermSet next = Mentioned<TermSet>([](Term t) { return t.function(); }, c);
        queue->insert(next.begin(), next.end());
        return true;
      } else {
        return false;
      }
    });
    return splits;
  }

  TermSet RelevantSplitTerms(Term lhs) {
    const Setup& s = Ground();
    if (s.Determines(lhs)) {
      return TermSet();
    }
    TermSet queue({lhs});
    TermSet splits;
    RelevantClosure(s, &queue, &splits, [](const Clause& c, Term t, TermSet* queue) {
      if (c.MentionsLhs(t)) {
        TermSet next = Mentioned<TermSet>([](Term t) { return t.function(); }, c);
        queue->insert(next.begin(), next.end());
        return true;
      } else {
        return false;
      }
    });
    return splits;
  }

  LiteralAssignmentSet LiteralAssignments() const {
    return LiteralAssignments(assigns_);
  }

  LiteralAssignmentSet RelevantLiteralAssignments(const Formula& phi) {
    assert(phi.objective());
    const Setup& s = Ground();
    LiteralSet queue;
    AddAssignmentLiteralsTo(Ground(Mentioned<LiteralSet>([](Literal a) { return a.lhs().function(); }, phi)), &queue);
    for (auto it = queue.begin(); it != queue.end(); ) {
      if (s.Determines(it->lhs())) {
        it = queue.erase(it);
      } else {
        ++it;
      }
    }
    LiteralSet assigns;
    RelevantClosure(s, &queue, &assigns, [this, &assigns](const Clause& c, Literal a, LiteralSet* queue) {
      if (c.MentionsLhs(a.lhs())) {
        AddAssignmentLiteralsTo(Mentioned<LiteralSet>([](Literal a) { return a.lhs().function(); }, c), queue);
        return true;
      } else {
        return false;
      }
    });
    return LiteralAssignments(assigns);
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(GrounderTest, Ground_SplitTerms_Names);
  FRIEND_TEST(GrounderTest, Assignments);
#endif

  typedef internal::IntMap<Symbol::Sort, size_t> PlusMap;

  class Assignments {
   public:
    class TermRange {
     public:
      TermRange() = default;
      explicit TermRange(const TermSet* terms) : terms_(terms) { Reset(); }

      bool operator==(const TermRange r) const { return terms_ == r.terms_ && begin_ == r.begin_; }
      bool operator!=(const TermRange r) const { return !(*this == r); }

      TermSet::const_iterator begin() const { return begin_; }
      TermSet::const_iterator end()   const { return terms_->end(); }

      bool empty() const { return begin_ == terms_->end(); }

      void Reset() { begin_ = terms_->begin(); }
      void Next() { ++begin_; }

     private:
      const TermSet* terms_;
      TermSet::const_iterator begin_;
    };

    class Assignment {
     public:
      internal::Maybe<Term> operator()(Term x) const {
        auto it = map_.find(x);
        if (it != map_.end()) {
          auto r = it->second;
          assert(!r.empty());
          const Term t = *r.begin();
          return internal::Just(t);
        } else {
          return internal::Nothing;
        }
      }

      bool operator==(const Assignment& a) const { return map_ == a.map_; }
      bool operator!=(const Assignment& a) const { return !(*this == a); }

      TermRange& operator[](Term t) { return map_[t]; }

      std::unordered_map<Term, TermRange>::iterator begin() { return map_.begin(); }
      std::unordered_map<Term, TermRange>::iterator end() { return map_.end(); }

     private:
      std::unordered_map<Term, TermRange> map_;
    };

    struct assignment_iterator {
      typedef std::ptrdiff_t difference_type;
      typedef const Assignment value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::input_iterator_tag iterator_category;

      // These iterators are really heavy-weight, especially comparison is
      // unusually expensive. To abbreviate the usual comparison with end(),
      // we hence reset the substitutes_ pointer to nullptr once the end is reached.
      assignment_iterator() {}
      assignment_iterator(const TermSet& vars, const SortedTermSet* substitutes) : substitutes_(substitutes) {
        for (const Term var : vars) {
          assert(var.symbol().variable());
          TermRange r(&((*substitutes_)[var.sort()]));
          assignment_[var] = r;
          assert(!r.empty());
          assert(var.sort() == r.begin()->sort());
        }
        meta_iter_ = assignment_.end();
      }

      bool operator==(const assignment_iterator& it) const {
        return substitutes_ == it.substitutes_ &&
              (substitutes_ == nullptr || (assignment_ == it.assignment_ &&
                                           *meta_iter_ == *it.meta_iter_));
      }
      bool operator!=(const assignment_iterator& it) const { return !(*this == it); }

      reference operator*() const { return assignment_; }

      assignment_iterator& operator++() {
        for (meta_iter_ = assignment_.begin(); meta_iter_ != assignment_.end(); ++meta_iter_) {
          TermRange& r = meta_iter_->second;
          assert(meta_iter_->first.symbol().variable());
          assert(!r.empty());
          r.Next();
          if (!r.empty()) {
            break;
          } else {
            r.Reset();
            assert(!r.empty());
            assert(meta_iter_->first.sort() == r.begin()->sort());
          }
        }
        if (meta_iter_ == assignment_.end()) {
          substitutes_ = nullptr;
          assert(*this == assignment_iterator());
        }
        return *this;
      }

     private:
      const SortedTermSet* substitutes_ = nullptr;
      Assignment assignment_;
      std::unordered_map<Term, TermRange>::iterator meta_iter_;
    };

    Assignments(const TermSet& vars, const SortedTermSet* substitutes) : vars_(vars), substitutes_(substitutes) {}

    assignment_iterator begin() const { return assignment_iterator(vars_, substitutes_); }
    assignment_iterator end() const { return assignment_iterator(); }

   private:
    const TermSet vars_;
    const SortedTermSet* substitutes_;
  };

  struct PairHasher {
    internal::hash32_t operator()(const std::pair<const Setup*, Literal>& p) const {
      typedef internal::uptr_t uptr_t;
      typedef internal::u32 u32;
      const uptr_t addr = reinterpret_cast<uptr_t>(p.first);
      return internal::jenkins_hash(static_cast<u32>(addr)) ^
             internal::jenkins_hash(static_cast<u32>(addr >> 32)) ^
             p.second.hash();
    }
  };

  template<typename T, typename U>
  void Ground(const T ungrounded, U* grounded_set) const {
    assert(ungrounded.quasiprimitive());
    const TermSet vars = Mentioned<TermSet>([](Term t) { return t.variable(); }, ungrounded);
    for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
      auto grounded = ungrounded.Substitute(mapping, tf_);
      assert(grounded.primitive());
      grounded_set->insert(grounded);
    }
  }

  template<typename T>
  T Ground(const T& ungrounded_set) const {
    T grounded_set;
    for (auto ungrounded : ungrounded_set) {
      Ground(ungrounded, &grounded_set);
    }
    return grounded_set;
  }

  template<typename Collection, typename Haystack, typename UnaryPredicate>
  static Collection Mentioned(const UnaryPredicate p, const Haystack& obj) {
    Collection needles;
    obj.Traverse([p, &needles](const typename Collection::value_type& t) {
      if (p(t)) {
        needles.insert(t);
      }
      return true;
    });
    return needles;
  }

  static PlusMap PlusNames(Term lhs) {
    // For term queries like KRef lhs, we assume there were a variable (lhs = x).
    // Hence we need two plus names: one for x, and one for the Lemma 8 fix.
    PlusMap plus;
    plus[lhs.sort()] = 2;
    for (const Term var : Mentioned<TermSet>([](Term t) { return t.variable(); }, lhs)) {
      ++plus[var.sort()];
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    PlusMap plus;
    for (const Term var : Mentioned<TermSet>([](Term t) { return t.variable(); }, c)) {
      ++plus[var.sort()];
    }
    // The following fixes Lemma 8 in the LBF paper. The problem is that
    // for KB = {[c = x]}, unit propagation should yield the empty clause;
    // but this requires that x is grounded by more than one name. It suffices
    // to ground variables by p+1 names, where p is the maximum number of
    // variables in any clause.
    // PlusNames() computes p for a given clause; it is hence p+1 where p
    // is the number of variables in that clause.
    PlusMap plus_one;
    c.Traverse([&plus_one](Term t) { plus_one[t.sort()] = 1; return true; });
    return PlusMap::Zip(plus, plus_one, [](size_t lp, size_t rp) { return lp + rp; });
  }

  static PlusMap PlusNames(const Formula& phi) {
    assert(phi.objective());
    // Roughly, we need to add one name for each quantifier. More precisely,
    // it suffices to check for every sort which is the maximal number of
    // different variables occurring freely in any subformula of phi. We do
    // so from the inside to the outside, determining the number of free
    // variables of any sort in cur, and the maximum in max.
    PlusMap max;
    PlusMap cur;
    PlusNames(phi, &cur, &max);
    return max;
  }

  static void PlusNames(const Formula& phi, PlusMap* cur, PlusMap* max) {
    assert(phi.objective());
    switch (phi.type()) {
      case Formula::kAtomic:
        *cur = PlusNames(phi.as_atomic().arg());
        *max = *cur;
        break;
      case Formula::kNot:
        PlusNames(phi.as_not().arg(), cur, max);
        break;
      case Formula::kOr: {
        PlusMap lcur, lmax;
        PlusMap rcur, rmax;
        PlusNames(phi.as_or().lhs(), &lcur, &lmax);
        PlusNames(phi.as_or().rhs(), &rcur, &rmax);
        *cur = PlusMap::Zip(lcur, rcur, [](size_t lp, size_t rp) { return lp + rp; });
        *max = PlusMap::Zip(lmax, rmax, [](size_t lp, size_t rp) { return std::max(lp, rp); });
        *max = PlusMap::Zip(*max, *cur, [](size_t mp, size_t cp) { return std::max(mp, cp); });
        break;
      }
      case Formula::kExists: {
        PlusNames(phi.as_exists().arg(), cur, max);
        Symbol::Sort sort = phi.as_exists().x().sort();
        if ((*cur)[sort] > 0) {
          --(*cur)[sort];
        }
        break;
      }
      case Formula::kKnow:
      case Formula::kCons:
      case Formula::kBel:
      case Formula::kGuarantee:
        break;
    }
  }

  bool AddMentionedNames(const SortedTermSet& names) {
    const size_t added = names_.insert(names);
    return added > 0;
  }

  bool AddPlusNames(const PlusMap& plus) {
    size_t added = 0;
    for (const Symbol::Sort sort : plus.keys()) {
      size_t m = plus_[sort];
      size_t n = plus[sort];
      if (n > m) {
        plus_[sort] = n;
        n -= m;
        while (n-- > 0) {
          added += names_.insert(CreateName(sort));
        }
      }
    }
    return added > 0;
  }

  void AddSplitTerms(const TermSet& terms) {
    size_t added = 0;
    for (Term t : terms) {
      added += splits_.insert(t).second ? 1 : 0;
    }
  }

  template<typename SourceQueue, typename Sink, typename UnaryPredicate>
  void RelevantClosure(const Setup& s, SourceQueue* queue, Sink* sink, UnaryPredicate collect) {
    std::unordered_set<size_t> done;
    RelevantClosure(s, queue, &done, sink, collect);
  }

  template<typename SourceQueue, typename Sink, typename UnaryPredicate>
  void RelevantClosure(const Setup& s,
                       SourceQueue* queue,
                       std::unordered_set<size_t>* done,
                       Sink* sink,
                       UnaryPredicate collect) {
    while (!queue->empty()) {
      const auto elem = *queue->begin();
      queue->erase(queue->begin());
      auto p = sink->insert(elem);
      if (p.second) {
        for (size_t i : s.clauses()) {
          if (done->find(i) != done->end()) {
            continue;
          }
          const Clause c = s.clause(i);
          if (c.unit() && c.first().pos()) {
            continue;
          }
          if (collect(c, elem, queue)) {
            done->insert(i);
          }
        }
      }
    }
  }

  void AddAssignmentLiteralsTo(const LiteralSet& lits, LiteralSet* assigns) {
    for (Literal a : lits) {
      if (!a.pos()) {
        const Term x = tf_->CreateTerm(sf_->CreateVariable(a.rhs().sort()));
        a = Literal::Eq(a.lhs(), x);
      }
      assigns->insert(a);
    }
  }

  void AddAssignmentLiterals(const LiteralSet& lits) {
    AddAssignmentLiteralsTo(lits, &assigns_);
  }

  LiteralAssignmentSet LiteralAssignments(const LiteralSet& assigns) const {
    LiteralSet ground = Ground(assigns);
    auto r = internal::transform_range(ground.begin(), ground.end(), [](Literal a) { return LiteralSet{a}; });
    LiteralAssignmentSet sets(r.begin(), r.end());
    for (Literal a : ground) {
      for (auto it = sets.begin(); it != sets.end(); ) {
        const LiteralSet& set = *it;
        assert(!set.empty());
        const Literal b = *set.begin();
        if (a.lhs().symbol() == b.lhs().symbol() && set.find(a) == set.end() && Literal::Isomorphic(a, b)) {
          if (set.size() == 1) {
            LiteralSet new_set = set;
            new_set.insert(a);
            const float prev_load_factor = sets.load_factor();
            const float prev_size = sets.size();
            sets.insert(new_set);
            if (sets.size() > prev_size && sets.load_factor() <= prev_load_factor) {
              it = sets.begin();
            } else {
              ++it;
            }
          } else {
            LiteralSet new_set = set;
            new_set.insert(a);
            it = sets.erase(it);
            sets.insert(new_set);
          }
        } else {
          ++it;
        }
      }
    }
    return sets;
  }

  Symbol::Factory* const sf_;
  Term::Factory* const tf_;
  PlusMap plus_;
  TermSet splits_;
  LiteralSet assigns_;
  SortedTermSet names_;
  bool names_changed_ = false;
  std::list<Clause> processed_clauses_;
  std::list<Clause> unprocessed_clauses_;
  SortedTermSet owned_names_;
  internal::Maybe<Setup> setup_;
};

}  // namespace lela

#endif  // LELA_GROUNDER_H_

