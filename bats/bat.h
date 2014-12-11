// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef BATS_BAT_H_
#define BATS_BAT_H_

#include <cassert>
#include <map>
#include <iostream>
#include <string>
#include <utility>
#include "./atom.h"
#include "./formula.h"
#include "./maybe.h"
#include "./setup.h"
#include "./term.h"

static int queries = 0;

namespace esbl {

namespace bats {

class Bat : public BasicActionTheory {
 public:
  Bat() : tf_() {}
  Bat(const Bat&) = delete;
  Bat& operator=(const Bat&) = default;
  virtual ~Bat() {}

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  Term::Factory& tf() { return tf_; }
  const Term::Factory& tf() const { return tf_; }

  Maybe<std::string> NameToString(const StdName& n) const {
    const auto it = name_to_string_.find(n);
    if (it != name_to_string_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

  Maybe<std::string> PredToString(const Atom::PredId& p) const {
    const auto it = pred_to_string_.find(p);
    if (it != pred_to_string_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

  Maybe<std::string> SortToString(const Term::Sort& p) const {
    const auto it = sort_to_string_.find(p);
    if (it != sort_to_string_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

  Maybe<StdName> StringToName(const std::string& s) const {
    const auto it = string_to_name_.find(s);
    if (it != string_to_name_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

  Maybe<Atom::PredId> StringToPred(const std::string& s) const {
    const auto it = string_to_pred_.find(s);
    if (it != string_to_pred_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

  Maybe<Term::Sort> StringToSort(const std::string& s) const {
    const auto it = string_to_sort_.find(s);
    if (it != string_to_sort_.end()) {
      return Just(it->second);
    } else {
      return Nothing;
    }
  }

 protected:
#if 0
  void ClearNegativeCacheEntries() const {
    for (auto it = cache_.begin(); it != cache_.end(); ) {
      assert(it->second);
      if (it->second.val) {
        ++it;
      } else {
        it = cache_.erase(it);
      }
    }
  }

  Maybe<bool>* CacheLookup(belief_level p,
                           const SimpleClause& c,
                           split_level k) const {
    return &cache_[std::make_tuple(p, k, c)];
  }
#endif

  Term::Factory tf_;
  std::map<StdName, std::string> name_to_string_;
  std::map<Atom::PredId, std::string> pred_to_string_;
  std::map<Term::Sort, std::string> sort_to_string_;
  std::map<std::string, StdName> string_to_name_;
  std::map<std::string, Atom::PredId> string_to_pred_;
  std::map<std::string, Term::Sort> string_to_sort_;

 private:
  struct Comparator {
    typedef std::tuple<belief_level, split_level, SimpleClause> value_type;

    LexicographicComparator<LessComparator<belief_level>,
                            LessComparator<split_level>,
                            SimpleClause::Comparator> compar;

    bool operator()(const value_type& t1, const value_type& t2) const {
      return compar(std::get<0>(t1), std::get<1>(t1), std::get<2>(t1),
                    std::get<0>(t2), std::get<1>(t2), std::get<2>(t2));
    }
  };

  mutable std::map<std::tuple<belief_level, split_level, SimpleClause>,
                   Maybe<bool>,
                   Comparator> cache_;
};

class KBat : public Bat {
 public:
  KBat() : names_init_(false) {}

  void GuaranteeConsistency(split_level k) override {
    s_.GuaranteeConsistency(k);
#if 0
    ClearNegativeCacheEntries();
#endif
  }

  const Setup& setup() const { return s_; }
  size_t n_levels() const override { return 1; }

  const StdName::SortedSet& names() const override {
    if (!names_init_) {
      names_ = s_.hplus().WithoutPlaceholders();
      names_init_ = true;
    }
    return names_;
  }

  void AddClause(const Clause& c) override {
    s_.AddClause(c);
    names_init_ = false;
#if 0
    ClearNegativeCacheEntries();
#endif
  }

  bool InconsistentAt(belief_level p, split_level k) const override {
    assert(p == 0);
    return s_.Inconsistent(k);
  }

  bool EntailsAt(belief_level p,
                 const SimpleClause& c,
                 split_level k) const override {
    assert(p == 0);
#if 0
    Maybe<bool>* r = CacheLookup(p, c, k);
    if (*r) {
      return r->val;
    }
    *r = Just(s_.Entails(c, k));
    return r->val;
#else
    ++queries;
    const bool r = s_.Entails(c, k);
    //std::cout << __FILE__ << ":" << __LINE__ << ": split level " << k << ": " << c << " = " << std::boolalpha << r << std::endl;
    return r;
#endif
  }

 private:
  Setup s_;
  mutable StdName::SortedSet names_;
  mutable bool names_init_;
};

class BBat : public Bat {
 public:
  BBat() : names_init_(false) {}

  void GuaranteeConsistency(split_level k) override {
    s_.GuaranteeConsistency(k);
#if 0
    ClearNegativeCacheEntries();
#endif
  }

  const Setups& setups() const { return s_; }
  size_t n_levels() const override { return s_.n_setups(); }

  const StdName::SortedSet& names() const override {
    if (!names_init_) {
      names_ = s_.hplus().WithoutPlaceholders();
      names_init_ = true;
    }
    return names_;
  }

  void AddClause(const Clause& c) override {
    s_.AddClause(c);
    names_init_ = false;
#if 0
    ClearNegativeCacheEntries();
#endif
  }

  void AddBeliefConditional(const Clause& neg_phi,
                            const Clause& psi,
                            split_level k) {
    s_.AddBeliefConditional(neg_phi, psi, k);
    names_init_ = false;
#if 0
    ClearNegativeCacheEntries();
#endif
  }

  bool InconsistentAt(belief_level p, split_level k) const override {
    return s_.setup(p).Inconsistent(k);
  }

  bool EntailsAt(belief_level p,
                 const SimpleClause& c,
                 split_level k) const override {
#if 0
    Maybe<bool>* r = CacheLookup(p, c, k);
    if (*r) {
      return r->val;
    }
    *r = Just(s_.setup(p).Entails(c, k));
    return r->val;
#else
    ++queries;
    const bool r = s_.setup(p).Entails(c, k);
    //std::cout << __FILE__ << ":" << __LINE__ << ": belief level " << p << ": split level " << k << ": " << c << " = " << std::boolalpha << r << std::endl;
    return r;
#endif
  }

 private:
  Setups s_;
  mutable StdName::SortedSet names_;
  mutable bool names_init_;
};

}  // namespace bats

}  // namespace esbl

#endif  // BATS_BAT_H_

