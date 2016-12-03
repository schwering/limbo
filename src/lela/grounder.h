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
#include <unordered_map>
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
  template<typename T, typename Comparator = typename T::Comparator>
  class SortedVector : public std::vector<T> {
   public:
    typedef std::vector<T> parent;
    using std::vector<T>::vector;

    size_t Add(T add) {
      parent::push_back(add);
      return 1;
    }

    size_t Add(const SortedVector& add) {
      parent::insert(parent::end(), add.begin(), add.end());
      return add.size();
    }

    size_t MakeSet() {
      std::sort(parent::begin(), parent::end(), Comparator());
      auto it = std::unique(parent::begin(), parent::end());
      size_t n = std::distance(it, parent::end());
      parent::erase(it, parent::end());
      return n;
    }
  };

  typedef SortedVector<Term> TermSet;
  typedef SortedVector<Literal> LiteralSet;

  class SortedTermSet : public internal::IntMap<Symbol::Sort, TermSet> {
   public:
    using internal::IntMap<Symbol::Sort, TermSet>::IntMap;

    size_t Add(Term t) {
      return (*this)[t.sort()].Add(t);
    }

    size_t Add(const TermSet& terms) {
      size_t n = 0;
      for (Term t : terms) {
        n += Add(t);
      }
      return n;
    }

    size_t Add(const SortedTermSet& terms) {
      size_t n = 0;
      for (const Symbol::Sort sort : terms.keys()) {
        n += (*this)[sort].Add(terms[sort]);
      }
      return n;
    }

    size_t MakeSet() {
      size_t n = 0;
      for (const Symbol::Sort sort : keys()) {
        n += (*this)[sort].MakeSet();
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
    TermSet splits = Ground(SplitTerms(k, phi));
    TermSet new_splits;
    internal::IntMap<Setup::Index, bool> marked;
    marked.set_null_value(false);
    for (size_t i = 0; i < splits.size(); ++i) {
      const Term t = splits[i];
      for (Setup::Index i : s.clauses_with(t)) {
        if (!marked[i]) {
          splits.Add(Mentioned<Term, TermSet>([](Term t) { return t.function(); }, s.clause(i)));
          marked[i] = true;
        }
      }
    }
    splits.MakeSet();
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
        needles.Add(t);
      }
      return true;
    });
    needles.MakeSet();
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

  template<typename UnaryPredicate, typename BinaryPredicate>
  void Flatten(Term t, UnaryPredicate outer_p, BinaryPredicate inner_p) {
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
    for (size_t i = 0; i < terms->size(); ++i) {
      Term t = (*terms)[i];
      if (!t.quasiprimitive()) {
        auto p = [terms](Term tt, Term = Term()) { terms->Add(tt); };
        Flatten(t, p, p);
      }
    }
    auto it = std::remove_if(terms->begin(), terms->end(), [](Term t) { return !t.quasiprimitive(); });
    terms->erase(it, terms->end());
    terms->MakeSet();
  }

  void Flatten(LiteralSet* lits) {
    for (size_t i = 0; i < lits->size(); ++i) {
      Literal a = (*lits)[i];
      if (a.rhs().function()) {
        Term x = tf_->CreateTerm(sf_->CreateVariable(a.rhs().sort()));
        lits->Add(Literal::Eq(a.rhs(), x));
        lits->Add(Literal::Eq(a.lhs(), x));
      } else if (!a.lhs().quasiprimitive()) {
        auto outer_p = [lits, a](Term new_lhs) {
          lits->Add(a.pos() ? Literal::Eq(new_lhs, a.rhs()) : Literal::Neq(new_lhs, a.rhs()));
        };
        auto inner_p = [lits](Term arg, Term x) {
          lits->Add(Literal::Eq(arg, x));
        };
        Flatten(a.lhs(), outer_p, inner_p);
      }
    }
    auto it = std::remove_if(lits->begin(), lits->end(), [](Literal a) { return !a.quasiprimitive(); });
    lits->erase(it, lits->end());
    lits->MakeSet();
  }

  template<typename T>
  SortedVector<T> Ground(const SortedVector<T>& ungrounded) const {
    SortedVector<T> grounded;
    for (T u : ungrounded) {
      assert(u.quasiprimitive());
      const TermSet vars = Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, u);
      for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
        T g = u.Substitute(mapping, tf_);
        assert(g.primitive());
        grounded.Add(g);
      }
      grounded.MakeSet();
    }
    return grounded;
  }

  template<typename T>
  SortedVector<T> PartiallyGround(const SortedVector<T>& ungrounded) const {
    SortedVector<T> grounded;
    for (T u : ungrounded) {
      assert(u.quasiprimitive());
      const TermSet vars = Mentioned<Term, TermSet>([](Term t) { return t.variable(); }, u);
      SortedTermSet terms = names_;
      terms.Add(vars);
      for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
        T g = u.Substitute(mapping, tf_);
        assert(g.primitive());
        grounded.Add(g);
      }
      grounded.MakeSet();
    }
    return grounded;
  }

  bool AddMentionedNames(const SortedTermSet& names) {
    size_t added = names_.Add(names);
    size_t remed = names_.MakeSet();
    assert(added >= remed);
    return added > remed;
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
          added += names_[sort].Add(tf_->CreateTerm(sf_->CreateName(sort)));
        }
      }
    }
    size_t remed = names_.MakeSet();
    assert(added >= remed);
    return added > remed;
  }

  bool AddSplitTerms(const TermSet& terms) {
    size_t added = splits_.Add(terms);
    size_t remed = splits_.MakeSet();
    assert(added >= remed);
    return added > remed;
  }

  bool AddAssignLiterals(const LiteralSet& lits) {
    size_t added = 0;
    for (Literal a : lits) {
      added += assigns_.Add(a.pos() ? a : a.flip());
    }
    size_t remed = assigns_.MakeSet();
    assert(added >= remed);
    return added > remed;
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

