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

class Bat {
 public:
  Bat() : tf_() {}

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  Term::Factory& tf() { return tf_; }
  const Term::Factory& tf() const { return tf_; }

 protected:
  Term::Factory tf_;
};

class KBat : public Bat {
 public:
  KBat() : s_() {}

  Setup& setup() { return s_; }
  const Setup& setup() const { return s_; }

 protected:
  Setup s_;
};

class BBat : public Bat {
 public:
  explicit BBat() : s_() {}

  virtual Setups& setups() { return s_; }
  virtual const Setups& setups() const { return s_; }

 protected:
  Setups s_;
};

}

#endif  // BATS_BAT_H_

