// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef BATS_BAT_H_
#define BATS_BAT_H_

#include <cassert>
#include <map>
#include <string>
#include <utility>
#include "atom.h"
#include "formula.h"
#include "maybe.h"
#include "setup.h"
#include "term.h"

namespace bats {

using namespace esbl;

class Bat {
 public:
  Bat() : tf_() {}
  Bat(const Bat&) = delete;
  Bat& operator=(const Bat&) = default;
  virtual ~Bat() {}

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  Term::Factory& tf() { return tf_; }
  const Term::Factory& tf() const { return tf_; }

  // Forward to setup[s]. That's not exactly beautiful, but simplifies the
  // ECLiPSe-CLP interface, for instance.
  virtual const StdName::SortedSet& hplus() const = 0;
  virtual void GuaranteeConsistency(Setup::split_level k) = 0;
  virtual void AddSensingResult(const TermSeq& z, const StdName& a, bool r) = 0;
  virtual bool Inconsistent(Setup::split_level k) = 0;
  virtual bool Entails(const Formula& phi, Setup::split_level k) = 0;

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
  Term::Factory tf_;
  std::map<StdName, std::string> name_to_string_;
  std::map<Atom::PredId, std::string> pred_to_string_;
  std::map<Term::Sort, std::string> sort_to_string_;
  std::map<std::string, StdName> string_to_name_;
  std::map<std::string, Atom::PredId> string_to_pred_;
  std::map<std::string, Term::Sort> string_to_sort_;
};

class KBat : public Bat {
 public:
  KBat() : s_() {}

  Setup& setup() { return s_; }
  const Setup& setup() const { return s_; }

  const StdName::SortedSet& hplus() const override { return s_.hplus(); }

  void GuaranteeConsistency(Setup::split_level k) override {
    s_.GuaranteeConsistency(k);
  }

  void AddSensingResult(const TermSeq& z, const StdName& a, bool r) override {
    s_.AddSensingResult(z, a, r);
  }

  bool Inconsistent(Setup::split_level k) override {
    return s_.Inconsistent(k);
  }

  bool Entails(const Formula& phi, Setup::split_level k) override {
    return phi.EntailedBy(&s_, k);
  }

 protected:
  Setup s_;
};

class BBat : public Bat {
 public:
  explicit BBat() : s_() {}

  virtual Setups& setups() { return s_; }
  virtual const Setups& setups() const { return s_; }

  const StdName::SortedSet& hplus() const override { return s_.hplus(); }

  void GuaranteeConsistency(Setup::split_level k) override {
    s_.GuaranteeConsistency(k);
  }

  void AddSensingResult(const TermSeq& z, const StdName& a, bool r) override {
    s_.AddSensingResult(z, a, r);
  }

  bool Inconsistent(Setup::split_level k) override {
    return s_.Inconsistent(k);
  }

  bool Entails(const Formula& phi, Setup::split_level k) override {
    return phi.EntailedBy(&s_, k);
  }

 protected:
  Setups s_;
};

}

#endif  // BATS_BAT_H_

