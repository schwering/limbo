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

#ifndef LIMBO_GROUNDER_H_
#define LIMBO_GROUNDER_H_

#include <cassert>

#include <algorithm>
#include <list>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <limbo/clause.h>
#include <limbo/formula.h>
#include <limbo/setup.h>

#include <limbo/internal/hash.h>
#include <limbo/internal/intmap.h>
#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>

namespace limbo {

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

  struct GetSort { Symbol::Sort operator()(Term t) const { return t.sort(); } };
  typedef internal::MultiIntSet<Term, GetSort> SortedTermSet;

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
          for (const auto& mapping : Assignments(vars, &names_)) {
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
    struct GetRange {
      GetRange(const SortedTermSet* substitutes) : substitutes_(substitutes) {}
      std::pair<Term, SortedTermSet::PosValues> operator()(const Term x) const {
        return std::make_pair(x, substitutes_->values(x));
      }
     private:
      const SortedTermSet* substitutes_;
    };
    typedef internal::transform_iterator<TermSet::const_iterator, GetRange> domain_codomain_iterator;
    typedef internal::mapping_iterator<Term, TermSet::const_iterator> mapping_iterator;

    Assignments(const TermSet& vars, const SortedTermSet* substitutes) : vars_(vars), substitutes_(substitutes) {}

    mapping_iterator begin() const {
      return mapping_iterator(domain_codomain_iterator(vars_.begin(), GetRange(substitutes_)),
                              domain_codomain_iterator(vars_.end(), GetRange(substitutes_)));
    }
    mapping_iterator end() const { return mapping_iterator(); }

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
    for (const auto& mapping : Assignments(vars, &names_)) {
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
    struct Func {  // a Lambda doesn't have assignment operators
      LiteralSet operator()(Literal a) const { return LiteralSet{a}; }
    };
    auto r = internal::transform_range(ground.begin(), ground.end(), Func());
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

}  // namespace limbo

#endif  // LIMBO_GROUNDER_H_

