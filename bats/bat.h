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
  Bat() = default;

  virtual Term::Id max_std_name() const = 0;
  virtual Atom::PredId max_pred() const = 0;

  virtual Setup& setup() = 0;
  virtual const Setup& setup() const = 0;

  virtual Term::Factory& tf() = 0;
  virtual const Term::Factory& tf() const = 0;
};

}

#endif  // BATS_BAT_H_

