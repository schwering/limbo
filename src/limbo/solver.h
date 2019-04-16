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
  void AddLiteral(const Lit a) {
    sat_.AddLiteral(a, extra_name_);
  }

  template<typename ExtraNameFactory>
  void AddClause(const std::vector<Lit>& as) {
    sat_.AddClause(as, extra_name_);
  }

  void Solve() {
    constexpr int K = 10;
    std::vector<Model> models(K);
    sat_.Simplify();
    for (int i = 0; i < K; ++i) {
      Solve(&models[i]);
    }
  }

 private:
  using Model = internal::DenseMap<Fun, bool>;

  struct ExtraNameFactory {
    Name operator()(Fun f) const {
      return Name();  // TODO
    }
  };

  struct ConflictPredicate {
    bool operator()(Sat::Level level, Sat::CRef conflict, Sat::Level btlevel) const {
      return true;  // TODO
    }
  };

  struct DecisionPredicate {
    bool operator()(Sat::Level level, Lit a) const {
      return true;  // TODO
    }
  };

  void Solve(Model* m) {
    Sat::Truth truth = sat_.Solve(conflict_predicate_, decision_predicate_);
  }

  Sat sat_;
  ExtraNameFactory extra_name_;
  ConflictPredicate conflict_predicate_;
  DecisionPredicate decision_predicate_;
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

