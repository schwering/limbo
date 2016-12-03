// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
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

#ifndef LELA_GROUNDER_H_
#define LELA_GROUNDER_H_

#include <cassert>

#include <algorithm>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lela/clause.h>
#include <lela/formula.h>
#include <lela/setup.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>

namespace lela {

class Grounder {
 public:
  typedef std::unordered_set<Term> TermSet;
  typedef std::unordered_set<Literal> LiteralSet;

  class SortedTermSet : public internal::IntMap<Symbol::Sort, TermSet> {
   public:
    using internal::IntMap<Symbol::Sort, TermSet>::IntMap;

    size_t insert(Term t) {
      auto p = (*this)[t.sort()].insert(t);
      return p.second ? 1 : 0;
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
  };

  Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
  Grounder(const Grounder&) = delete;
  Grounder(const Grounder&&) = delete;
  Grounder& operator=(const Grounder) = delete;

  typedef std::list<Clause>::const_iterator clause_iterator;
  typedef internal::joined_ranges<clause_iterator, clause_iterator> clause_range;

  clause_range clauses() const {
    return internal::join_ranges(processed_clauses_.cbegin(), processed_clauses_.cend(),
                                 unprocessed_clauses_.cbegin(), unprocessed_clauses_.cend());
  }

  void AddClause(const Clause& c) {
    assert(std::all_of(c.begin(), c.end(),
                       [](Literal a) { return a.quasiprimitive() || !(a.lhs().function() && a.rhs().function()); }));
    if (c.valid()) {
      return;
    }
    names_changed_ |= AddMentionedNames(Mentioned<Term, SortedTermSet>([](Term t) { return t.name(); }, c));
    names_changed_ |= AddPlusNames(PlusNames(c));
    AddSplitTerms(Mentioned<Term, TermSet>([](Term t) { return t.quasiprimitive(); }, c));
    AddAssignLiterals(Mentioned<Literal, LiteralSet>([](Literal a) { return a.quasiprimitive(); }, c));
    unprocessed_clauses_.push_front(c);
  }

  template<typename T>
  void PrepareForQuery(size_t k, const Formula::Reader<T>& phi) {
    names_changed_ |= AddMentionedNames(Mentioned<Term, SortedTermSet>([](Term t) { return t.name(); }, phi));
    names_changed_ |= AddPlusNames(PlusNames(phi));
    names_changed_ |= AddPlusNames(PlusSplitNames(k, phi));
    AddSplitTerms(SplitTerms(k, phi));
    AddAssignLiterals(AssignLiterals(k, phi));
  }

  const Setup& Ground() const { return const_cast<Grounder*>(this)->Ground(); }

  const Setup& Ground() {
    if (names_changed_) {
      // Re-ground all clauses, i.e., all clauses are considered unprocessed and all old setups are forgotten.
      unprocessed_clauses_.splice(unprocessed_clauses_.begin(), processed_clauses_);
      setups_.clear();
      assert(processed_clauses_.empty());
      assert(setups_.empty());
    }
    if (!unprocessed_clauses_.empty() || setups_.empty()) {
      // Ground the unprocessed clauses in a new setup, which inherits from the last setup for efficiency.
      Setup* parent = !setups_.empty() ? &setups_.front() : nullptr;
      if (!parent) {
        setups_.push_front(Setup());
      } else {
        setups_.push_front(Setup(parent));
      }
      Setup* s = &setups_.front();
      assert(s != parent);
      for (const Clause& c : unprocessed_clauses_) {
        if (c.ground()) {
          assert(c.primitive());
          if (!c.valid()) {
            s->AddClause(c);
          }
        } else {
          const TermSet vars = Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, c);
          for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
            const Clause ci = c.Substitute(mapping, tf_);
            if (!ci.valid()) {
              assert(ci.primitive());
              s->AddClause(ci);
            }
          }
        }
      }
      s->Init();
      processed_clauses_.splice(processed_clauses_.begin(), unprocessed_clauses_);
      names_changed_ = false;
    }
    assert(!setups_.empty());
    return setups_.front();
  }

  const SortedTermSet& Names() const {
    return names_;
  }

  TermSet SplitTerms() const {
    return Ground(splits_);
  }

  template<typename T>
  TermSet RelevantSplitTerms(size_t k, const Formula::Reader<T>& phi) {
    PrepareForQuery(k, phi);
    const Setup& s = Ground();
    TermSet splits;
    TermSet queue = Ground(SplitTerms(k, phi));
    while (!queue.empty()) {
      const Term t = *queue.begin();
      queue.erase(queue.begin());
      auto p = splits.insert(t);
      if (p.second) {
        for (Setup::Index i : s.clauses_with(t)) {
          TermSet next = Mentioned<Term, TermSet>([](Term t) { return t.function(); }, s.clause(i));
          queue.insert(next.begin(), next.end());
        }
      }
    }
    return splits;
  }

  std::list<LiteralSet> AssignLiterals() const {
    LiteralSet lits = PartiallyGround(assigns_);
    auto r = internal::transform_range(lits.begin(), lits.end(), [this](Literal a) {
      const LiteralSet singleton{a};
      return a.ground() ? singleton : Ground(singleton);
    });
    return std::list<LiteralSet>(r.begin(), r.end());
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(GrounderTest, Ground_SplitTerms_Names);
  FRIEND_TEST(GrounderTest, Assignments);
#endif

  struct Assignments {
    struct TermRange {
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

    struct Assignment {
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

  typedef internal::IntMap<Symbol::Sort, size_t> PlusMap;

  template<typename Needle, typename Collection, typename Haystack, typename UnaryPredicate>
  static Collection Mentioned(const UnaryPredicate p, const Haystack& obj) {
    Collection needles;
    obj.Traverse([p, &needles](const Needle& t) {
      if (p(t)) {
        needles.insert(t);
      }
      return true;
    });
    return needles;
  }

  static PlusMap PlusNames(const TermSet& vars) {
    PlusMap plus;
    for (const Term var : vars) {
      ++plus[var.sort()];
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    PlusMap plus = PlusNames(Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, c));
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

  template<typename T>
  static PlusMap PlusNames(const Formula::Reader<T>& phi) {
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

  template<typename T>
  static void PlusNames(const Formula::Reader<T>& phi, PlusMap* cur, PlusMap* max) {
    switch (phi.head().type()) {
      case Formula::Element::kClause:
        *cur = PlusNames(phi.head().clause().val);
        *max = *cur;
        break;
      case Formula::Element::kNot:
        PlusNames(phi.arg(), cur, max);
        break;
      case Formula::Element::kOr: {
        PlusMap lcur, lmax;
        PlusMap rcur, rmax;
        PlusNames(phi.left(), &lcur, &lmax);
        PlusNames(phi.right(), &rcur, &rmax);
        *cur = PlusMap::Zip(lcur, rcur, [](size_t lp, size_t rp) { return lp + rp; });
        *max = PlusMap::Zip(lmax, rmax, [](size_t lp, size_t rp) { return std::max(lp, rp); });
        *max = PlusMap::Zip(*max, *cur, [](size_t mp, size_t cp) { return std::max(mp, cp); });
        break;
      }
      case Formula::Element::kExists:
        PlusNames(phi.arg(), cur, max);
        Symbol::Sort sort = phi.head().var().val.sort();
        if ((*cur)[sort] > 0) {
          --(*cur)[sort];
        }
        break;
    }
  }

  template<typename T>
  static PlusMap PlusSplitNames(size_t k, const Formula::Reader<T>& phi) {
    // When a term t only occurs in the form of literals (t = n), (t = x), or
    // their duals and negations, then splitting does not necessitate an
    // additional name. However, when t is an argument of another term or when
    // it occurs in literals of the form (t = t') or its dual or negation, then
    // we might need a name for every split.
    // Note that in general we really need k plus names. For instance,
    // (c = d) is not valid. To see that for k >= 2, splitting needs to consider
    // at least 2 <= k names, not just one.
    PlusMap plus;
    phi.Traverse([&plus, k](Literal a) {
      auto f = [&plus, k](Term t) {
        if (t.function()) {
          plus[t.sort()] = k;
        }
        return true;
      };
      if (a.lhs().function() && a.rhs().function()) {
        f(a.lhs());
        f(a.rhs());
      }
      for (Term t : a.lhs().args()) {
        t.Traverse(f);
      }
      for (Term t : a.rhs().args()) {
        t.Traverse(f);
      }
      return true;
    });
    return plus;
  }

  template<typename T>
  TermSet SplitTerms(size_t k, const Formula::Reader<T>& phi) {
    if (k == 0) {
      return TermSet();
    }
    TermSet terms = Mentioned<Term, TermSet>([](Term t) { return t.function(); }, phi);
    Flatten(&terms);
    return terms;
  }

  template<typename T>
  LiteralSet AssignLiterals(size_t k, const Formula::Reader<T>& phi) {
    if (k == 0) {
      return LiteralSet();
    }
    LiteralSet lits = Mentioned<Literal, LiteralSet>([](Literal a) {
      return a.lhs().function() || a.rhs().function();
    }, phi);
    Flatten(&lits);
    return lits;
  }

  template<typename BinaryPredicate, typename UnaryPredicate>
  void Flatten(Term t, BinaryPredicate inner_p, UnaryPredicate outer_p) {
    if (!t.quasiprimitive()) {
      Term::Vector args = t.args();
      for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].function()) {
          Term x = tf_->CreateTerm(sf_->CreateVariable(args[i].sort()));
          inner_p(args[i], x);
          for (size_t j = i; j < args.size(); ++j) {
            if (args[j] == args[i]) {
              args[j] = x;
            }
          }
        }
      }
      outer_p(tf_->CreateTerm(t.symbol(), args));
    }
  }

  void Flatten(TermSet* terms) {
    TermSet queue;
    for (Term t : *terms) {
      if (!t.quasiprimitive()) {
        queue.insert(t);
      }
    }
    while (!queue.empty()) {
      Term t = *queue.begin();
      queue.erase(queue.begin());
      assert(!t.quasiprimitive());
      auto p = [terms, &queue](Term tt, Term = Term()) {
        if (tt.quasiprimitive()) {
          terms->insert(tt);
        } else {
          queue.insert(tt);
        }
      };
      Flatten(t, p, p);
    }
    for (auto it = terms->begin(); it != terms->end(); ) {
      if (it->quasiprimitive()) {
        ++it;
      } else {
        it = terms->erase(it);
      }
    }
  }

  void Flatten(LiteralSet* lits) {
    LiteralSet queue;
    for (Literal a : *lits) {
      if (!a.quasiprimitive()) {
        queue.insert(a);
      }
    }
    auto save = [lits, &queue](Literal a) { if (a.quasiprimitive()) { lits->insert(a); } else { queue.insert(a); } };
    while (!queue.empty()) {
      Literal a = *queue.begin();
      queue.erase(queue.begin());
      assert(!a.quasiprimitive());
      if (a.rhs().function()) {
        Term x = tf_->CreateTerm(sf_->CreateVariable(a.rhs().sort()));
        save(Literal::Neq(a.rhs(), x));
        save(a.pos() ? Literal::Eq(a.lhs(), x) : Literal::Neq(a.lhs(), x));
      } else if (!a.lhs().quasiprimitive()) {
        auto inner_p = [save](Term arg, Term x) {
          save(Literal::Neq(arg, x));
        };
        auto outer_p = [save, a](Term new_lhs) {
          save(a.pos() ? Literal::Eq(new_lhs, a.rhs()) : Literal::Neq(new_lhs, a.rhs()));
        };
        Flatten(a.lhs(), inner_p, outer_p);
      }
    }
    for (auto it = lits->begin(); it != lits->end(); ) {
      if (it->quasiprimitive()) {
        ++it;
      } else {
        it = lits->erase(it);
      }
    }
  }

  template<typename T>
  std::unordered_set<T> Ground(const std::unordered_set<T>& ungrounded) const {
    std::unordered_set<T> grounded;
    for (T u : ungrounded) {
      assert(u.quasiprimitive());
      const TermSet vars = Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, u);
      for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
        T g = u.Substitute(mapping, tf_);
        assert(g.primitive());
        grounded.insert(g);
      }
    }
    return grounded;
  }

  template<typename T>
  std::unordered_set<T> PartiallyGround(const std::unordered_set<T>& ungrounded) const {
    std::unordered_set<T> grounded;
    for (T u : ungrounded) {
      assert(u.quasiprimitive());
      const TermSet vars = Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, u);
      SortedTermSet terms = names_;
      terms.insert(vars);
      for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
        T g = u.Substitute(mapping, tf_);
        assert(g.primitive());
        grounded.insert(g);
      }
    }
    return grounded;
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
          added += names_.insert(tf_->CreateTerm(sf_->CreateName(sort)));
        }
      }
    }
    return added > 0;
  }

  bool AddSplitTerms(const TermSet& terms) {
    size_t added = 0;
    for (Term t : terms) {
      added += splits_.insert(t).second ? 1 : 0;
    }
    return added > 0;
  }

  bool AddAssignLiterals(const LiteralSet& lits) {
    size_t added = 0;
    for (Literal a : lits) {
      added += assigns_.insert(a.pos() ? a : a.flip()).second ? 1 : 0;
    }
    return added > 0;
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
  std::list<Setup> setups_;
};

}  // namespace lela

#endif  // LELA_GROUNDER_H_

