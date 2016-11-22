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
#include <map>
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
  class TermSet : public std::vector<Term> {
   public:
    using std::vector<Term>::vector;

    size_t Add(Term t) {
      push_back(t);
      return 1;
    }

    size_t Add(const TermSet& terms) {
      insert(end(), terms.begin(), terms.end());
      return terms.size();
    }

    size_t MakeSet() {
      std::sort(begin(), end(), Term::Comparator());
      auto it = std::unique(begin(), end());
      size_t n = std::distance(it, end());
      erase(it, end());
      return n;
    }
  };

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

      std::map<Term, TermRange>::iterator begin() { return map_.begin(); }
      std::map<Term, TermRange>::iterator end() { return map_.end(); }

     private:
      std::map<Term, TermRange> map_;
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
      std::map<Term, TermRange>::iterator meta_iter_;
    };

    Assignments(const TermSet& vars, const SortedTermSet* substitutes) : vars_(vars), substitutes_(substitutes) {}

    assignment_iterator begin() const { return assignment_iterator(vars_, substitutes_); }
    assignment_iterator end() const { return assignment_iterator(); }

   private:
    const TermSet vars_;
    const SortedTermSet* substitutes_;
  };

  template<typename T>
  struct Groundings {
    struct SubstituteTerm {
      SubstituteTerm() = default;
      SubstituteTerm(T orig, Term::Factory* tf) : orig_(orig), tf_(tf) {}
      T operator()(const Assignments::Assignment& assignment) const { return orig_.Substitute(assignment, tf_); }
     private:
      T orig_;
      Term::Factory* tf_;
    };

    typedef internal::transform_iterator<Assignments::assignment_iterator, SubstituteTerm> iterator;

    Groundings(const T& orig, const SortedTermSet* substitutes, Term::Factory* tf)
        : assignments_(MentionedTerms<TermSet>([](Term term) { return term.variable(); }, orig), substitutes),
          substitute_(orig, tf) {}

    iterator begin() const { return iterator(assignments_.begin(), substitute_); }
    iterator end()   const { return iterator(assignments_.end(), substitute_); }

   private:
    Assignments assignments_;
    SubstituteTerm substitute_;
  };

  explicit Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
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
    names_changed_ |= AddMentionedNames(MentionedTerms<SortedTermSet>([](Term t) { return t.name(); }, c));
    names_changed_ |= AddPlusNames(PlusNames(c));
    AddSplitTerms(MentionedTerms<TermSet>([](Term t) { return t.quasiprimitive(); }, c));
    unprocessed_clauses_.push_front(c);
  }

  template<typename T>
  void PrepareForQuery(size_t k, const Formula::Reader<T>& phi) {
    names_changed_ |= AddMentionedNames(MentionedTerms<SortedTermSet>([](Term t) { return t.name(); }, phi));
    names_changed_ |= AddPlusNames(PlusNames(phi));
    names_changed_ |= AddPlusNames(PlusSplitNames(k, phi));
    AddSplitTerms(SplitTerms(k, phi));
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
          const TermSet vars = MentionedTerms<TermSet>([](Term t) { return t.variable(); }, c);
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

  template<typename Iter>
  struct GroundedTerms {
    typedef typename Iter::value_type value_type;
    struct MakeGrounding {
      MakeGrounding() = default;
      MakeGrounding(const SortedTermSet* substitutes, Term::Factory* tf) : substitutes_(substitutes), tf_(tf) {}
      Groundings<value_type> operator()(const value_type& obj) const {
        return Groundings<value_type>(obj, substitutes_, tf_);
      }
     private:
      const SortedTermSet* substitutes_;
      Term::Factory* tf_;
    };
    typedef internal::transform_iterator<Iter, MakeGrounding> grounding_iterator;
    typedef internal::flatten_iterator<grounding_iterator> ground_iterator;
    //typedef internal::filter_iterator<ground_iterator, internal::unique_filter<value_type>> unique_ground_iterator;
    typedef ground_iterator unique_ground_iterator;

    GroundedTerms(Iter begin, Iter end, const SortedTermSet* substitutes, Term::Factory* tf)
        : ground_(substitutes, tf),
#if 0
          begin_(unique_ground_iterator(ground_iterator(grounding_iterator(begin, ground_),
                                                        grounding_iterator(begin, ground_)),
                                        ground_iterator(grounding_iterator(end,   ground_),
                                                        grounding_iterator(end,   ground_)),
                                        internal::unique_filter<value_type>())),
          end_(  unique_ground_iterator(ground_iterator(grounding_iterator(end,   ground_),
                                                        grounding_iterator(end,   ground_)),
                                        ground_iterator(grounding_iterator(end,   ground_),
                                                        grounding_iterator(end,   ground_)),
                                        internal::unique_filter<value_type>())) {}
#else
          begin_(ground_iterator(grounding_iterator(begin, ground_),
                                 grounding_iterator(begin, ground_))),
          end_(  ground_iterator(grounding_iterator(end,   ground_),
                                 grounding_iterator(end,   ground_))) {}
#endif

    unique_ground_iterator begin() const { return begin_; }
    unique_ground_iterator end()   const { return end_; }

   private:
    MakeGrounding ground_;
    unique_ground_iterator begin_;
    unique_ground_iterator end_;
  };

  GroundedTerms<TermSet::const_iterator> SplitTerms() const {
    return GroundedTerms<TermSet::const_iterator>(splits_.begin(), splits_.end(), &names_, tf_);
  }

  //TermSet SplitTerms() const {
  //  return GroundTerms(splits_);
  //}

  template<typename T>
  TermSet RelevantSplitTerms(size_t k, const Formula::Reader<T>& phi) {
    PrepareForQuery(k, phi);
    const Setup& s = Ground();
    TermSet splits = GroundTerms(SplitTerms(k, phi));
    TermSet new_splits;
    internal::IntMap<Setup::Index, bool> marked;
    marked.set_null_value(false);
    for (size_t i = 0; i < splits.size(); ++i) {
      const Term t = splits[i];
      for (Setup::Index i : s.clauses_with(t)) {
        if (!marked[i]) {
          splits.Add(MentionedTerms<TermSet>([](Term t) { return t.function(); }, s.clause(i)));
          marked[i] = true;
        }
      }
    }
    splits.MakeSet();
    return splits;
  }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(GrounderTest, Ground_SplitTerms_Names);
#endif

  typedef internal::IntMap<Symbol::Sort, size_t> PlusMap;

  template<typename R, typename T, typename UnaryPredicate>
  static R MentionedTerms(const UnaryPredicate p, const T& obj) {
    R terms;
    obj.Traverse([p, &terms](Term t) {
      if (p(t)) {
        terms.Add(t);
      } return true;
    });
    terms.MakeSet();
    return terms;
  }

  static PlusMap PlusNames(const TermSet& vars) {
    PlusMap plus;
    for (const Term var : vars) {
      ++plus[var.sort()];
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    assert(std::all_of(c.begin(), c.end(),
                       [](Literal a) { return a.quasiprimitive() || !(a.lhs().function() && a.rhs().function()); }));
    PlusMap plus = PlusNames(MentionedTerms<TermSet>([](Term t) { return t.variable(); }, c));
    // The following fixes Lemma 8 in the LBF paper. The problem is that
    // for KB = {[c = x]}, unit propagation should yield the empty clause;
    // but this requires that x is grounded by more than one name. It suffices
    // to ground variables by p+1 names, where p is the maximum number of
    // variables in any clause.
    // PlusNames() computes p for a given clause; it is hence p+1 where p
    // is the number of variables in that clause. To avoid unnecessary
    // grounding, we leave p=0 in case there are no variables.
    for (const Symbol::Sort sort : plus.keys()) {
      if (plus[sort] > 0) {
        ++plus[sort];
      }
    }
    return plus;
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
        *cur = PlusNames(MentionedTerms<TermSet>([](Term t) { return t.variable(); }, phi.head().clause().val));
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
  TermSet SplitTerms(size_t k, const Formula::Reader<T>& phi) {
    if (k == 0) {
      // Grounding the split terms could fail in case k == 0 because there
      // might be no split names.
      return TermSet();
    }
    TermSet terms = MentionedTerms<TermSet>([](Term t) { return t.function(); }, phi);
    Flatten(&terms);
    return terms;
  }

  void Flatten(TermSet* terms) {
    for (size_t i = 0; i < terms->size(); ++i) {
      Term old = (*terms)[i];
      if (!old.quasiprimitive()) {
        Symbol::Factory* sf = sf_;
        Term::Factory* tf = tf_;
        // We could save some instantiations here by substituting the same
        // variables for the identical terms that occur more than once.
        Term sub = old.Substitute([terms, old, sf, tf](Term t) {
          return (t != old && t.function())
              ? internal::Just(tf->CreateTerm(sf->CreateVariable(t.sort())))
              : internal::Nothing;
        }, tf_);
        assert(sub.quasiprimitive());
        terms->Add(sub);
      }
    }
    auto it = std::remove_if(terms->begin(), terms->end(), [](Term t) { return !t.quasiprimitive(); });
    terms->erase(it, terms->end());
    terms->MakeSet();
  }

  TermSet GroundTerms(const TermSet& terms) const {
    TermSet grounded_terms;
    for (Term t : terms) {
      assert(t.quasiprimitive());
      const TermSet vars = MentionedTerms<TermSet>([](Term t) { return t.variable(); }, t);
      for (const Assignments::Assignment& mapping : Assignments(vars, &names_)) {
        Term tt = t.Substitute(mapping, tf_);
        assert(tt.primitive());
        grounded_terms.Add(tt);
      }
      grounded_terms.MakeSet();
    }
    return grounded_terms;
  }

  template<typename T>
  static PlusMap PlusSplitNames(size_t k, const Formula::Reader<T>& phi) {
    // When a term t only occurs in the form of literals (t = n), (t = x), or
    // their duals and negations, then splitting does not necessitate an
    // additional name. However, when t is an argument of another term or when
    // it occurs in literals of the form (t = t') or its dual or negation, then
    // we might need a name for every split. For instance, (c != c) shall come
    // out false at any split level. It is false at split level 0 and 1. At
    // split level >= 2 we need to make sure that we split over at least 2
    // names to find that (c != c) is false.
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

  Symbol::Factory* const sf_;
  Term::Factory* const tf_;
  PlusMap plus_;
  TermSet splits_;
  SortedTermSet names_;
  bool names_changed_ = false;
  std::list<Clause> processed_clauses_;
  std::list<Clause> unprocessed_clauses_;
  std::list<Setup> setups_;
};

}  // namespace lela

#endif  // LELA_GROUNDER_H_

