// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// A Grounder determines how many standard names need to be substituted for
// variables in a proper+ knowledge base and in queries.

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

    void Add(Term t) {
      push_back(t);
    }

    void Add(const TermSet& terms) {
      insert(end(), terms.begin(), terms.end());
    }

    void MakeSet() {
      std::sort(begin(), end(), Term::Comparator());
      erase(std::unique(begin(), end()), end());
    }
  };

  class SortedTermSet : public internal::IntMap<Symbol::Sort, TermSet> {
   public:
    using internal::IntMap<Symbol::Sort, TermSet>::IntMap;

    void Add(Term t) {
      (*this)[t.sort()].Add(t);
    }

    void Add(const TermSet& terms) {
      for (Term t : terms) {
        Add(t);
      }
    }

    void Add(const SortedTermSet& terms) {
      for (const Symbol::Sort sort : terms.keys()) {
        (*this)[sort].Add(terms[sort]);
      }
    }

    void MakeSet() {
      for (const Symbol::Sort sort : keys()) {
        (*this)[sort].MakeSet();
      }
    }
  };

  explicit Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}
  Grounder(const Grounder&) = delete;
  Grounder(const Grounder&&) = delete;
  Grounder& operator=(const Grounder) = delete;

  const std::list<Clause>& kb() const { return cs_; }

  void AddClause(const Clause& c) {
    assert(std::all_of(c.begin(), c.end(), [](Literal a) { return a.quasiprimitive() || a.idiotic(); }));
    if (c.valid()) {
      return;
    }
    AddMentionedNames(MentionedTerms<SortedTermSet>([](Term t) { return t.name(); }, c));
    AddPlusNames(PlusNames(c));
    AddSplitTerms(MentionedTerms<TermSet>([](Term t) { return t.quasiprimitive(); }, c));
    cs_.push_front(c);
  }

  template<typename T>
  void PrepareFor(size_t k, const Formula::Reader<T>& phi) {
    AddMentionedNames(MentionedTerms<SortedTermSet>([](Term t) { return t.name(); }, phi));
    AddPlusNames(PlusNames(phi));
    TermSet terms = MentionedTerms<TermSet>([](Term t) { return t.function(); }, phi);
    Flatten(&terms);
    AddSplitTerms(terms);
    AddPlusNames(PlusSplitNames(k, terms));
  }

  Setup Ground() const {
    Setup s;
    for (const Clause& c : cs_) {
      if (c.ground()) {
        assert(c.primitive());
        if (!c.valid()) {
          s.AddClause(c);
        }
      } else {
        const TermSet vars = MentionedTerms<TermSet>([](Term t) { return t.variable(); }, c);
        for (const Assignments::Assignment& mapping : Assignments(&names_, &vars)) {
          const Clause ci = c.Substitute(mapping, tf_);
          if (!ci.valid()) {
            assert(ci.primitive());
            s.AddClause(ci);
          }
        }
      }
    }
    s.Init();
    return s;
  }

  const SortedTermSet& Names() const {
    return names_;
  }

  TermSet SplitTerms() const {
    TermSet terms;
    for (Term t : splits_) {
      assert(t.quasiprimitive());
      const TermSet vars = MentionedTerms<TermSet>([](Term t) { return t.variable(); }, t);
      for (const Assignments::Assignment& mapping : Assignments(&names_, &vars)) {
        Term tt = t.Substitute(mapping, tf_);
        assert(tt.primitive());
        terms.Add(tt);
      }
      terms.MakeSet();
    }
    return terms;
  }

 private:
  typedef internal::IntMap<Symbol::Sort, size_t> PlusMap;

  struct Assignments {
    struct NameRange {
      NameRange() = default;
      explicit NameRange(const TermSet* names) : names_(names) { Reset(); }

      bool operator==(const NameRange r) const { return names_ == r.names_ && begin_ == r.begin_; }
      bool operator!=(const NameRange r) const { return !(*this == r); }

      TermSet::const_iterator begin() const { return begin_; }
      TermSet::const_iterator end()   const { return names_->end(); }

      bool empty() const { return begin_ == names_->end(); }

      void Reset() { begin_ = names_->begin(); }
      void Next() { ++begin_; }

     private:
      const TermSet* names_;
      TermSet::const_iterator begin_;
    };

    struct Assignment {
      internal::Maybe<Term> operator()(Term v) const {
        auto it = map_.find(v);
        if (it != map_.end()) {
          auto r = it->second;
          assert(!r.empty());
          const Term name = *r.begin();
          assert(name.name());
          return internal::Just(name);
        } else {
          return internal::Nothing;
        }
      }

      bool operator==(const Assignment& a) const { return map_ == a.map_; }
      bool operator!=(const Assignment& a) const { return !(*this == a); }

      NameRange& operator[](Term t) { return map_[t]; }

      std::map<Term, NameRange>::iterator begin() { return map_.begin(); }
      std::map<Term, NameRange>::iterator end() { return map_.end(); }

     private:
      std::map<Term, NameRange> map_;
    };

    struct assignment_iterator {
      typedef std::ptrdiff_t difference_type;
      typedef const Assignment value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::input_iterator_tag iterator_category;

      // These iterators are really heavy-weight, especially comparison is
      // unusually expensive. To abbreviate the usual comparison with end(),
      // we hence reset the names_ pointer to nullptr once the end is reached.
      assignment_iterator() {}
      assignment_iterator(const SortedTermSet* names, const TermSet& vars) : names_(names) {
        for (const Term var : vars) {
          assert(var.symbol().variable());
          NameRange r(&((*names_)[var.sort()]));
          assignment_[var] = r;
          assert(!r.empty());
          assert(var.sort() == r.begin()->sort());
        }
        meta_iter_ = assignment_.end();
      }

      bool operator==(const assignment_iterator& it) const {
        return names_ == it.names_ &&
              (names_ == nullptr || (assignment_ == it.assignment_ &&
                                     *meta_iter_ == *it.meta_iter_));
      }
      bool operator!=(const assignment_iterator& it) const { return !(*this == it); }

      reference operator*() const { return assignment_; }

      assignment_iterator& operator++() {
        for (meta_iter_ = assignment_.begin(); meta_iter_ != assignment_.end(); ++meta_iter_) {
          NameRange& r = meta_iter_->second;
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
          names_ = nullptr;
          assert(*this == assignment_iterator());
        }
        return *this;
      }

     private:
      const SortedTermSet* names_ = nullptr;
      Assignment assignment_;
      std::map<Term, NameRange>::iterator meta_iter_;
    };

    Assignments(const SortedTermSet* names, const TermSet* vars) : names_(names), vars_(vars) {}

    assignment_iterator begin() const { return assignment_iterator(names_, *vars_); }
    assignment_iterator end() const { return assignment_iterator(); }

   private:
    const SortedTermSet* names_;
    const TermSet* vars_;
  };

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
    assert(std::all_of(c.begin(), c.end(), [](Literal a) { return a.quasiprimitive() || a.idiotic(); }));
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

  PlusMap PlusSplitNames(size_t k, const TermSet& terms) {
    PlusMap plus;
    for (Term t : terms) {
      if (plus[t.sort()] < k) {
        plus[t.sort()] = k;
      }
    }
    return plus;
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

  void AddMentionedNames(const SortedTermSet& names) {
    names_.Add(names);
    names_.MakeSet();
  }

  void AddPlusNames(const PlusMap& plus) {
    for (const Symbol::Sort sort : plus.keys()) {
      size_t n = plus[sort];
      if (plus_[sort] < n) {
        plus_[sort] = n;
        while (n-- > 0) {
          names_[sort].Add(tf_->CreateTerm(sf_->CreateName(sort)));
        }
      }
    }
    names_.MakeSet();
  }

  void AddSplitTerms(const TermSet& terms) {
    splits_.Add(terms);
    splits_.MakeSet();
  }

  std::list<Clause> cs_;
  PlusMap plus_;
  TermSet splits_;
  SortedTermSet names_;
  Symbol::Factory* const sf_;
  Term::Factory* const tf_;
};

}  // namespace lela

#endif  // LELA_GROUNDER_H_

