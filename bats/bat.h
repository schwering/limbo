// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef BATS_BAT_H_
#define BATS_BAT_H_

#include <cassert>
#include <map>
#include <string>
#include "atom.h"
#include "setup.h"
#include "term.h"

namespace bats {

using namespace esbl;

class BAT {
 public:
  void Init() {
    InitNameToStringMap();
    InitPredToStringMap();
    InitStringToNameMap();
    InitStringToPredMap();
    InitSetup();
    //setup_.UpdateHPlus(tf_);
  }

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  virtual void InitSetup() = 0;

  Setup& setup() { return setup_; }
  const Setup& setup() const { return setup_; }

  Term::Factory& tf() { return tf_; }
  const Term::Factory& tf() const { return tf_; }

 protected:
  virtual void InitNameToStringMap() = 0;
  virtual void InitPredToStringMap() = 0;
  virtual void InitStringToNameMap() = 0;
  virtual void InitStringToPredMap() = 0;

  std::map<StdName, const char*> name_to_string_;
  std::map<Atom::PredId, const char*> pred_to_string_;
  std::map<std::string, StdName> string_to_name_;
  std::map<std::string, Atom::PredId> string_to_pred_;

 private:
  Term::Factory tf_;
  Setup setup_;
};

}

#endif  // BATS_BAT_H_

