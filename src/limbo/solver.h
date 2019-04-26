// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_SOLVER_H_
#define LIMBO_SOLVER_H_

#include <algorithm>
#include <vector>

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

  void AddClause(std::vector<Lit>&& as) {
    clauses_.emplace_back(as);
    AddTerms(clauses_.back());
  }

  void AddClause(const std::vector<Lit>& as) {
    clauses_.push_back(as);
    AddTerms(clauses_.back());
  }

  void Solve() {
    internal::DenseMap<Fun, bool> covered;

    covered.Capacitate(funs_.upper_bound());

    sat_.Simplify();
    for (int i = 0; i < K; ++i) {
      Solve(&models[i]);
    }
  }

 private:
  using Model = internal::DenseMap<Fun, bool>;

  struct ExtraNameFactory {
    explicit ExtraNameFactory(const Name* n) : n_(n) {}
    Name operator()(Fun) const { return *n_; }
   private:
    const Name* n_;
  };

  void Solve(Model* m) {
    Sat sat;
    for (const std::vector<Lit>& as : clauses_) {
      sat.AddClause(as, [this](Fun) { return extra_name_; });
    }
    Sat::Truth truth = sat.Solve(
        [this](int, Sat::CRef, const std::vector<Lit>&, int) -> bool { return HandleConflict(); },
        [this](int, Lit)                                     -> bool { return HandleDecision(); });
  }

  bool HandleConflict() const {
    return true;
  }

  bool HandleDecision() const {
    return true;
  }

  void AddTerms(const std::vector<Lit>& as) {
    for (const Lit a : as) {
      const Fun f = a.fun();
      const Name n = a.name();
      funs_.Capacitate(f);
      if (!funs_[f]) {
        funs_[f] = true;
      }
      names_.Capacitate(n);
      if (!names_[n]) {
        names_[n] = true;
        extra_name_ = Name::FromId(std::max(int(n) + 1, int(extra_name_)));
      }
      //names_.Capacitate(f);
      //extra_names_.Capacitate(f);
      //if (!names_[f][n]) {
      //  names_[f][n] = true;
      //  extra_names_[f] = Name::FromId(std::max(int(n) + 1, int(extra_names_[f])));
      //}
    }
  }

  std::vector<std::vector<Lit>>  clauses_;
  internal::DenseMap<Fun, bool>  funs_;
  internal::DenseMap<Name, bool> names_;
  Name                           extra_name_;
  //internal::DenseMap<Fun, internal::DenseMap<Name, bool>> names_;
  //internal::DenseMap<Fun, Name>                           extra_names_;
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

