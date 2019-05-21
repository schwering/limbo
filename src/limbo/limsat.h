// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_LIMSAT_H_
#define LIMBO_LIMSAT_H_

#include <algorithm>
#include <vector>

#include <limbo/sat.h>
#include <limbo/internal/dense.h>
#include <limbo/internal/subsets.h>

namespace limbo {

class LimSat {
 public:
  explicit LimSat() = default;

  LimSat(const LimSat&)            = delete;
  LimSat& operator=(const LimSat&) = delete;
  LimSat(LimSat&&)                 = default;
  LimSat& operator=(LimSat&&)      = default;

  void AddClause(std::vector<Lit>&& as) {
    clauses_.emplace_back(as);
    AddTerms(clauses_.back());
  }

  void AddClause(const std::vector<Lit>& as) {
    clauses_.push_back(as);
    AddTerms(clauses_.back());
  }

  bool Solve(int k) {
    return FindModels(k);
  }

 private:
  enum class SolverType { kWithLearntClauses, kWithoutLearntClauses };

  bool FindModels(int min_model_size) {
    std::vector<internal::DenseMap<Fun, Name>> models;
    std::vector<std::vector<Fun>> newly_assigned_in;
    const bool all_covered = FindAllAssigningModels(min_model_size, &models, &newly_assigned_in);
    if (!all_covered) {
      return false;
    }
    return internal::AllCombinedSubsetsOfSize(newly_assigned_in, min_model_size, [&](const std::vector<Fun>& must) {
      for (const internal::DenseMap<Fun, Name>& model : models) {
        if (AssignsAll(model, must)) {
          return true;
        }
      }
      internal::DenseMap<Fun, Name> model;
      internal::DenseMap<Fun, bool> wanted(max_fun(), false);
      for (const Fun f : must) {
        wanted[f] = true;
      }
      constexpr bool propagate_with_learnt = false;
      constexpr bool wanted_is_must = true;
      const bool succ = FindModel(min_model_size, propagate_with_learnt, wanted_is_must, wanted, &model);
      return succ;
    });
  }

  bool FindAllAssigningModels(int min_model_size,
                              std::vector<internal::DenseMap<Fun, Name>>* models,
                              std::vector<std::vector<Fun>>* newly_assigned_in) {
    models->clear();
    newly_assigned_in->clear();
    internal::DenseMap<Fun, bool> wanted = funs_;
    bool propagate_with_learnt = true;
    bool wanted_is_must = false;
    bool all_assigned = false;
    while (!all_assigned) {
      internal::DenseMap<Fun, Name> model;
      bool succ = FindModel(min_model_size, propagate_with_learnt, wanted_is_must, wanted, &model);
      if (!succ && propagate_with_learnt) {
        propagate_with_learnt = false;
        continue;
      }
      if (!succ) {
        printf("line %d: !succ\n", __LINE__);
        return false;
      }
      std::vector<Fun> newly_assigned;
      ExtractAssignedFunctions(model, &wanted, &newly_assigned, &all_assigned);
      if (newly_assigned.empty() && !wanted_is_must) {
        wanted_is_must = true;
        continue;
      }
      if (newly_assigned.empty() && min_model_size == 0) {
        return true;
      }
      for (int i = 0; i < int(models->size()); ) {
        if (AssignsAll(model, (*newly_assigned_in)[i])) {
          newly_assigned = MergeFunLists(newly_assigned, (*newly_assigned_in)[i]);
          models->erase(models->begin() + i);
          newly_assigned_in->erase(newly_assigned_in->begin() + i);
        } else {
          ++i;
        }
      }
      models->push_back(model);
      newly_assigned_in->push_back(newly_assigned);
    }
    printf("all assigned (%d) with %lu models\n", all_assigned, newly_assigned_in->size());
    return true;
  }

  void ExtractAssignedFunctions(const internal::DenseMap<Fun, Name> model,
                                internal::DenseMap<Fun, bool>* wanted,
                                std::vector<Fun>* newly_assigned,
                                bool* all_assigned) {
    newly_assigned->clear();
    *all_assigned = true;
    for (const Fun f : wanted->keys()) {
      if ((*wanted)[f]) {
        if (assigns(model, f)) {
          if ((*wanted)[f]) { printf("newly set: %d [%d, %d]\n", int(f), bool((*wanted)[f]), bool(funs_[f])); }
          (*wanted)[f] = false;
          newly_assigned->push_back(f);
        } else {
          *all_assigned = false;
        }
      }
    }
  }

  static bool assigns(const internal::DenseMap<Fun, Name>& model, Fun f) {
    return !model[f].null();
  }

  static bool AssignsAll(const internal::DenseMap<Fun, Name>& model, std::vector<Fun> funs) {
    for (const Fun f : funs) {
      if (!assigns(model, f)) {
        return false;
      }
    }
    return true;
  }

  static std::vector<Fun> MergeFunLists(const std::vector<Fun>& funs1, const std::vector<Fun>& funs2) {
    std::vector<Fun> v;
    v.resize(std::set_union(funs1.begin(), funs1.end(), funs2.begin(), funs2.end(), v.begin()) - v.begin());
    return v;
  }

  bool FindModel(const int min_model_size,
                 const bool propagate_with_learnt,
                 const bool wanted_is_must,
                 const internal::DenseMap<Fun, bool>& wanted,
                 internal::DenseMap<Fun, Name>* model,
                 int index = 0) {
    printf("FindModel: min_model_size = %d, propagate_with_learnt = %s, wanted_is_must = %s, wanted =", min_model_size, propagate_with_learnt ? "true" : "false", wanted_is_must ? "true" : "false");
    for (Fun f : wanted.keys()) {
      if (wanted[f]) {
        printf(" %d", int(f));
      }
    }
    printf("\n");
    auto wanted_covered = [&](const internal::DenseMap<Fun, Name>& m) -> bool {
      for (Fun f : wanted.keys()) {
        if (wanted[f] && !assigns(m, f)) {
          return false;
        }
      }
      return true;
    };
    static constexpr double activity_offset = 1000.0;
    static constexpr int max_conflicts = 50;
    int n_conflicts = 0;
    int model_size = 0;
    Sat sat;
    for (const std::vector<Lit>& as : clauses_) {
      //printf("adding clause:");
      //for (Lit a : as) {
      //  printf("   %d %s %d", int(a.fun()), a.pos() ? "=" : "!=", int(a.name()));
      //}
      //printf("\n");
      sat.AddClause(as,
                    [&](Fun)   -> Name   { return extra_name_; },
                    [&](Fun f) -> double { return wanted[f] * activity_offset; });
    }
    sat.set_propagate_with_learnt(propagate_with_learnt);
    Sat::Truth truth = sat.Solve(
        [&](int, Sat::CRef, const std::vector<Lit>&, int) -> bool {
          return ++n_conflicts <= max_conflicts;
        },
        [&](int, Lit a) -> bool {
          //printf("   %d %s %d\n", int(a.fun()), a.pos() ? "=" : "!=", int(a.name()));
          if (min_model_size <= sat.model_size() && model_size < sat.model_size() &&
              (!wanted_is_must || wanted_covered(sat.model()))) {
            model_size = sat.model_size();
            *model = sat.model();
          }
          return true;
        });
    //printf("truth = %s\n", truth == Sat::Truth::kSat ? "SAT" : truth == Sat::Truth::kUnsat ? "UNSAT" : "UNKNOWN");
    //printf("model_size = %d\n", model_size);
    //printf("return value = %d\n", truth == Sat::Truth::kSat ? true : model_size >= min_model_size);
    if (truth == Sat::Truth::kSat ? true : model_size >= min_model_size) {
      printf("FindModel: true, model_size = %d, assignment =", model_size);
      if (truth == Sat::Truth::kSat) {
        *model = sat.model();
      }
      for (const Fun f : model->keys()) {
        if (assigns(*model, f)) {
          printf(" (%d = %d)", int(f), int((*model)[f]));
        }
      }
      printf("\n");
    } else {
      printf("FindModel: false\n");
    }
    if (truth == Sat::Truth::kSat) {
      assert(wanted_covered(sat.model()));
      *model = sat.model();
      return true;
    } else {
      return model_size >= min_model_size;
    }
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

  Fun max_fun() const { return Fun::FromId(funs_.upper_bound()); }

  std::vector<std::vector<Lit>>  clauses_;
  internal::DenseMap<Fun, bool>  funs_;
  internal::DenseMap<Name, bool> names_;
  Name                           extra_name_;
  //internal::DenseMap<Fun, internal::DenseMap<Name, bool>> names_;
  //internal::DenseMap<Fun, Name>                           extra_names_;
};

}  // namespace limbo

#endif  // LIMBO_LIMSAT_H_

