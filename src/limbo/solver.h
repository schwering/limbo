// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_SOLVER_H_
#define LIMBO_SOLVER_H_

#include <cstdlib>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/lit.h>
#include <limbo/sat.h>

#include <limbo/internal/dense.h>

namespace limbo {

class Solver {
 public:
  explicit Solver() = default;

  Solver(const Solver&)            = delete;
  Solver& operator=(const Solver&) = delete;
  Solver(Solver&&)                 = default;
  Solver& operator=(Solver&&)      = default;

  template<typename ExtraNameFactory>
  void AddLiteral(const Lit a, ExtraNameFactory extra_name = ExtraNameFactory()) {
    sat_.AddLiteral(a, extra_name);
  }

  template<typename ExtraNameFactory>
  void AddClause(const std::vector<Lit>& as, ExtraNameFactory extra_name = ExtraNameFactory()) {
    sat_.AddClause(as, extra_name);
  }

 private:
  Sat sat_;
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

