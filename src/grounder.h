// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Substitutes standard names for variables. Corresponds to the gnd() operator.

#ifndef SRC_GROUNDER_H_
#define SRC_GROUNDER_H_

#include <cassert>
#include <algorithm>
#include <map>
#include <vector>
#include <utility>
#include "./clause.h"
#include "./formula.h"
#include "./iter.h"
#include "./maybe.h"
#include "./setup.h"

namespace lela {

class Grounder {
 public:
  typedef std::multimap<Symbol::Sort, Term> SortedNames;

  explicit Grounder(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf) {}

  const std::list<Clause>& kb() const { return kb_; }

  void AddClause(const Clause& c) {
    assert(c.quasiprimitive());
    AddMentionedNames(MentionedNames(c));
    AddPlusNames(PlusNames(c));
    kb_.push_front(c);
  }

  template<typename T>
  void PrepareFor(const Formula::Reader<T>& phi) {
    AddMentionedNames(MentionedNames(phi));
    AddPlusNames(PlusNames(phi));
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
        const VariableSet vars = MentionedVariables(c);
        for (VariableMapping mapping(&names_, vars); mapping.has_next(); ++mapping) {
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

 private:
  typedef std::vector<Term> VariableSet;
  typedef IntMap<Symbol::Sort, size_t, 0> PlusMap;

  static SortedNames MentionedNames(const Clause& c) {
    assert(c.quasiprimitive());
    SortedNames names;
    c.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    return names;
  }

  template<typename T>
  static SortedNames MentionedNames(const Formula::Reader<T>& phi) {
    SortedNames names;
    phi.Traverse([&names](Term t) { if (t.name()) { names.insert(std::make_pair(t.symbol().sort(), t)); } return true; });
    return names;
  }

  static VariableSet MentionedVariables(const Clause& c) {
    VariableSet vars;
    c.Traverse([&vars](Term t) { if (t.variable()) { vars.push_back(t); } return true; });
    std::sort(vars.begin(), vars.end(), Term::Comparator());
    vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    return vars;
  }

  static PlusMap PlusNames(const VariableSet& vars) {
    PlusMap plus;
    for (const Term var : vars) {
      ++plus[var.symbol().sort()];
    }
    return plus;
  }

  static PlusMap PlusNames(const Clause& c) {
    assert(c.quasiprimitive());
    PlusMap plus = PlusNames(MentionedVariables(c));
    for (const Literal l : c) {
      if (l.lhs().symbol().function()) { ++plus[l.lhs().symbol().sort()]; }
      if (l.rhs().symbol().function()) { ++plus[l.rhs().symbol().sort()]; }
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
        *cur = PlusNames(MentionedVariables(phi.head().clause().val));
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

  class VariableMapping {
   public:
    typedef SortedNames::const_iterator name_iterator;
    typedef std::pair<name_iterator, name_iterator> name_range;
#if 0
    struct Get {
      std::pair<Term, Term> operator()(const std::pair<Term, name_range> p) const {
        const Term& var = p.first;
        const name_range& r = p.second;
        assert(r.first != r.second);
        assert(var.symbol().sort() == r.first->first);
        assert(var.symbol().sort() == r.first->second.symbol().sort());
        const Term& name = r.first->second;
        assert(name.name());
        return std::make_pair(var, name);
      }
    };
    typedef transform_iterator<Get, std::map<Term, name_range>::const_iterator> iterator;
#endif

    VariableMapping(const SortedNames* names, const VariableSet& vars) : names_(names) {
      for (const Term var : vars) {
        assert(var.symbol().variable());
        const name_range r = names_->equal_range(var.symbol().sort());
        assert(r.first != r.second);
        assert(var.symbol().sort() == r.first->first);
        assert(var.symbol().sort() == r.first->second.symbol().sort());
        assignment_[var] = r;
      }
      meta_iter_ = assignment_.begin();
    }

    Maybe<Term> operator()(Term v) const {
      auto it = assignment_.find(v);
      if (it != assignment_.end()) {
        auto r = it->second;
        assert(r.first != r.second);
        const Term& name = r.first->second;
        assert(name.name());
        return Just(name);
      } else {
        return Nothing;
      }
    }

    VariableMapping& operator++() {
      assert(meta_iter_ != assignment_.end());
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
      return *this;
    }

    bool has_next() const { return meta_iter_ != assignment_.end(); }

#if 0
    iterator begin() const { return iterator(Get(), assignment_.begin()); }
    iterator end()   const { return iterator(Get(), assignment_.end()); }
#endif

   private:
    const SortedNames* names_;
    std::map<Term, name_range> assignment_;
    std::map<Term, name_range>::iterator meta_iter_;
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

  std::list<Clause> kb_;
  PlusMap plus_;
  SortedNames names_;
  Symbol::Factory* sf_;
  Term::Factory* tf_;
};

}  // namespace lela

#endif  // SRC_GROUNDER_H_

