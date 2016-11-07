// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Substitutes standard names for variables. Corresponds to the gnd() operator.

#ifndef SRC_GROUNDER_H_
#define SRC_GROUNDER_H_

#include <cassert>
#include <algorithm>
#include <list>
#include <map>
#include <utility>
#include <vector>
#include "./clause.h"
#include "./formula.h"
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;
  typedef std::vector<Term> TermSet;

  explicit Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}

  const std::list<Clause>& kb() const { return kb_; }

  void AddClause(const Clause& c) {
    assert(c.quasiprimitive());
    AddMentionedNames(MentionedNames(c));
    AddPlusNames(PlusNames(c));
    AddSplitTerms(MentionedTerms([](Term t) { return t.quasiprimitive(); }, c));
    kb_.push_front(c);
  }

  template<typename T>
  void PrepareFor(const Formula::Reader<T>& phi) {
    AddMentionedNames(MentionedNames(phi));
    AddPlusNames(PlusNames(phi));
    AddSplitTerms(MentionedTerms([](Term t) { return t.quasiprimitive(); }, phi));
  }

  Setup Ground() const {
    Setup s;
    for (const Clause& c : kb_) {
      if (c.ground()) {
        assert(c.primitive());
        if (!c.valid()) {
          s.AddClause(c);
        }
      } else {
        const TermSet vars = MentionedTerms([](Term t) { return t.variable(); }, c);
        for (const Assignments::Assignment& mapping : Assignments(&names_, &vars)) {
          const Clause ci = c.Substitute(mapping, tf_);
          assert(ci.primitive());
          if (!ci.valid()) {
            s.AddClause(ci);
          }
        }
      }
    }
    s.Init();
    return s;
  }

  SortedNames SplitNames() const {
    return names_;
  }

  TermSet SplitTerms() const {
    TermSet terms;
    for (Term t : splits_) {
      assert(t.quasiprimitive());
      TermSet vars = MentionedTerms([](Term t) { return t.variable(); }, t);
      for (const Assignments::Assignment& mapping : Assignments(&names_, &vars)) {
        Term tt = t.Substitute(mapping, tf_);
        assert(tt.primitive());
        if (!std::binary_search(terms.begin(), terms.end(), tt)) {
          terms.push_back(tt);
        }
      }
      MakeSet(&terms);
    }
    return terms;
  }

 private:
  typedef IntMap<Symbol::Sort, size_t, 0> PlusMap;

  static void MakeSet(TermSet* terms) {
    std::sort(terms->begin(), terms->end(), Term::Comparator());
    terms->erase(std::unique(terms->begin(), terms->end()), terms->end());
  }

  template<typename T>
  static SortedNames MentionedNames(const T& obj) {
    SortedNames names;
    obj.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    return names;
  }

  template<typename UnaryPredicate, typename T>
  static TermSet MentionedTerms(const UnaryPredicate p, const T& obj) {
    TermSet terms;
    obj.Traverse([p, &terms](Term t) { if (p(t)) { terms.push_back(t); } return true; });
    MakeSet(&terms);
    return terms;
  }

  static PlusMap PlusNames(const TermSet& vars) {
    PlusMap plus;
    for (const Term var : vars) {
      ++plus[var.symbol().sort()];
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    assert(c.quasiprimitive());
    PlusMap plus = PlusNames(MentionedTerms([](Term t) { return t.variable(); }, c));
    // The following fixes Lemma 8 in the LBF paper. The problem is that
    // for KB = {[c = x]}, unit propagation should yield the empty clause;
    // but this requires that x is grounded by more than one name. It suffices
    // to ground variables by p+1 names, where p is the maximum number of
    // variables in any clause.
    // PlusNames() computes p for a given clause; it is hence p+1 where p
    // is the number of variables in that clause. To avoid unnecessary
    // grounding, we leave p=0 in case there are no variables.
    for (auto p : const_cast<const PlusMap&>(plus)) {
      const Symbol::Sort sort = p.first;
      const size_t n = p.second;
      if (n > 0) {
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
        *cur = PlusNames(MentionedTerms([](Term t) { return t.variable(); }, phi.head().clause().val));
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
        Symbol::Sort sort = phi.head().var().val.symbol().sort();
        if ((*cur)[sort] > 0) {
          --(*cur)[sort];
        }
        break;
    }
  }

  struct Assignments {
    typedef SortedNames::const_iterator name_iterator;
    typedef std::pair<name_iterator, name_iterator> name_range;

    struct Assignment {
      Maybe<Term> operator()(Term v) const {
        auto it = map_.find(v);
        if (it != map_.end()) {
          auto r = it->second;
          assert(r.first != r.second);
          const Term& name = r.first->second;
          assert(name.name());
          return Just(name);
        } else {
          return Nothing;
        }
      }

      bool operator==(const Assignment& a) const { return map_ == a.map_; }
      bool operator!=(const Assignment& a) const { return !(*this == a); }

      name_range& operator[](Term t) { return map_[t]; }

      std::map<Term, name_range>::iterator begin() { return map_.begin(); }
      std::map<Term, name_range>::iterator end() { return map_.end(); }

     private:
      std::map<Term, name_range> map_;
    };

    struct assignment_iterator {
      typedef std::ptrdiff_t difference_type;
      typedef Assignment value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef std::input_iterator_tag iterator_category;

      // These iterators are really heavy-weight, especially comparison is
      // unusually expensive. To abbreviate the usual comparison with end(),
      // we hence reset the names_ pointer to nullptr once the end is reached.
      assignment_iterator() {}
      assignment_iterator(const SortedNames* names, const TermSet& vars) : names_(names) {
        for (const Term var : vars) {
          assert(var.symbol().variable());
          const name_range r = names_->equal_range(var.symbol().sort());
          assert(r.first != r.second);
          assert(var.symbol().sort() == r.first->first);
          assert(var.symbol().sort() == r.first->second.symbol().sort());
          assignment_[var] = r;
        }
        meta_iter_ = assignment_.end();
      }

      bool operator==(const assignment_iterator& it) const { return names_ == it.names_ && (names_ == nullptr || (assignment_ == it.assignment_ && *meta_iter_ == *it.meta_iter_)); }
      bool operator!=(const assignment_iterator& it) const { return !(*this == it); }

      const Assignment& operator*() const { return assignment_; }

      assignment_iterator& operator++() {
        for (meta_iter_ = assignment_.begin(); meta_iter_ != assignment_.end(); ++meta_iter_) {
          const Term var = meta_iter_->first;
          name_range& r = meta_iter_->second;
          assert(var.symbol().variable());
          assert(r.first != r.second);
          ++r.first;
          if (r.first != r.second) {
            break;
          } else {
            r.first = names_->lower_bound(var.symbol().sort());
            assert(r.first != r.second);
            assert(var.symbol().sort() == r.first->first);
            assert(var.symbol().sort() == r.first->second.symbol().sort());
          }
        }
        if (meta_iter_ == assignment_.end()) {
          names_ = nullptr;
          assert(*this == assignment_iterator());
        }
        return *this;
      }

     private:
      const SortedNames* names_ = nullptr;
      Assignment assignment_;
      std::map<Term, name_range>::iterator meta_iter_;
    };

    Assignments(const SortedNames* names, const TermSet* vars) : names_(names), vars_(vars) {}

    assignment_iterator begin() const { return assignment_iterator(names_, *vars_); }
    assignment_iterator end() const { return assignment_iterator(); }

   private:
    const SortedNames* names_;
    const TermSet* vars_;
  };

  void AddMentionedNames(const SortedNames& names) {
    names_.insert(names.begin(), names.end());
  }

  void AddPlusNames(const PlusMap& plus) {
    for (auto p : plus) {
      const Symbol::Sort sort = p.first;
      size_t n = p.second;
      if (plus_[sort] < n) {
        plus_[sort] = n;
        while (n-- > 0) {
          names_.insert(std::make_pair(sort, tf_->CreateTerm(sf_->CreateName(sort))));
        }
      }
    }
  }

  void AddSplitTerms(const TermSet& terms) {
    splits_.insert(splits_.end(), terms.begin(), terms.end());
    MakeSet(&splits_);
  }

  std::list<Clause> kb_;
  PlusMap plus_;
  TermSet splits_;
  SortedNames names_;
  Symbol::Factory* sf_;
  Term::Factory* tf_;
};

}  // namespace lela

#endif  // SRC_GROUNDER_H_

