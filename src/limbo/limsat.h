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

  struct FoundModel {
    FoundModel() : succ(false) {}
    FoundModel(TermMap<Fun, Name>&& model) : model(model), succ(true) {}
    FoundModel(const TermMap<Fun, Name>& model) : model(model), succ(true) {}
    TermMap<Fun, Name> model;
    bool succ;
  };

  struct FoundCoveringModels {
    FoundCoveringModels() : all_covered(false) {}
    FoundCoveringModels(std::vector<TermMap<Fun, Name>>&& models,
                        std::vector<std::vector<Fun>>&& newly_assigned_in)
        : models(models), newly_assigned_in(newly_assigned_in), all_covered(true) {}
    std::vector<TermMap<Fun, Name>> models;
    std::vector<std::vector<Fun>> newly_assigned_in;
    bool all_covered;
  };

  struct AssignedFunctions {
    AssignedFunctions(std::vector<Fun>&& newly_assigned, bool all_assigned)
        : newly_assigned(newly_assigned), all_assigned(all_assigned) {}
    std::vector<Fun> newly_assigned;
    bool all_assigned = false;
  };

  static bool assigns(const TermMap<Fun, Name>& model, const Fun f) {
    return model.in_range(f) && !model[f].null();
  }

  static bool AssignsAll(const TermMap<Fun, Name>& model, const std::vector<Fun>& funs) {
    for (const Fun f : funs) {
      if (!assigns(model, f)) {
        return false;
      }
    }
    return true;
  }

  static bool AssignsAll(const TermMap<Fun, Name>& model, const TermMap<Fun, bool>& wanted) {
    for (const Fun f : wanted.keys()) {
      if (wanted[f] && !assigns(model, f)) {
        return false;
      }
    }
    return true;
  }

  template<typename T>
  static std::vector<Fun> Merge(const std::vector<T>& xs, const std::vector<Fun>& ys) {
    std::vector<Fun> zs;
    zs.resize(std::set_union(xs.begin(), xs.end(), ys.begin(), ys.end(), zs.begin()) - zs.begin());
    return zs;
  }

  static AssignedFunctions GetAndUnwantNewlyAssignedFunctions(const TermMap<Fun, Name> model,
                                                              TermMap<Fun, bool>* wanted) {
    std::vector<Fun> newly_assigned;
    bool all_assigned = true;
    for (const Fun f : wanted->keys()) {
      if ((*wanted)[f]) {
        if (assigns(model, f)) {
          (*wanted)[f] = false;
          newly_assigned.push_back(f);
        } else {
          all_assigned = false;
        }
      }
    }
    return AssignedFunctions(std::move(newly_assigned), all_assigned);
  }

  bool FindModels(const int min_model_size) {
    // Find models such that every function is assigned a value in some model.
    // For example, consider a problem with functions 1,2,3,4,5 and minimum
    // model size 2.
    // We might find two models M1 and M2 that assign 1,2,3 and 3,4,5, which
    // covers all functions. M1 and M2 imply models that assign the subsets of
    // cardinality of size 2 of {1,2,3} and {3,4,5}, that is,
    // {1,2}, {2,3}, {1,3}, and {3,4}, {4,5}, {3,5}.
    const FoundCoveringModels fcm = FindCoveringModels(min_model_size);
    if (!fcm.all_covered) {
      return false;
    }
    // Now find models for sets for which models aren't implied yet.
    // In the example, the sets {{x,y} | x in {1,2,3}, y in {4,5}} that are not
    // subsets of {1,2,3} or {3,4,5}.
    return internal::AllCombinedSubsetsOfSize(fcm.newly_assigned_in, min_model_size,
                                              [&](const std::vector<Fun>& must) -> bool {
      // Skip sets of functions that have been covered already.
      // In the example, {3,4} and {3,5} are implied by M2.
      for (const TermMap<Fun, Name>& model : fcm.models) {
        if (AssignsAll(model, must)) {
          return true;
        }
      }
      TermMap<Fun, bool> wanted(max_fun(), false);
      for (const Fun f : must) {
        wanted[f] = true;
      }
      constexpr bool propagate_with_learnt = false;
      constexpr bool wanted_is_must = true;
      const FoundModel fm = FindModel(min_model_size, propagate_with_learnt, wanted_is_must, wanted);
      return fm.succ;
    });
  }

  FoundCoveringModels FindCoveringModels(const int min_model_size) {
    std::vector<TermMap<Fun, Name>> models;
    std::vector<std::vector<Fun>> newly_assigned_in;
    TermMap<Fun, bool> wanted = funs_;
    bool propagate_with_learnt = true;
    bool wanted_is_must = false;
    for (;;) {
      const FoundModel fm = FindModel(min_model_size, propagate_with_learnt, wanted_is_must, wanted);
      if (!fm.succ && propagate_with_learnt) {
        propagate_with_learnt = false;
        continue;
      }
      if (!fm.succ) {
        return FoundCoveringModels();
      }
      AssignedFunctions gaf = GetAndUnwantNewlyAssignedFunctions(fm.model, &wanted);
      if (gaf.newly_assigned.empty() && !wanted_is_must) {
        wanted_is_must = true;
        continue;
      }
      if (gaf.newly_assigned.empty() && min_model_size == 0) {
        return FoundCoveringModels();
      }
      // Remove previous models that assign a subset of the newly found model.
      for (int i = 0; i < int(models.size()); ) {
        if (AssignsAll(fm.model, newly_assigned_in[i])) {
          gaf.newly_assigned = Merge(gaf.newly_assigned, newly_assigned_in[i]);
          models.erase(models.begin() + i);
          newly_assigned_in.erase(newly_assigned_in.begin() + i);
        } else {
          ++i;
        }
      }
      models.push_back(fm.model);
      newly_assigned_in.push_back(gaf.newly_assigned);
      if (gaf.all_assigned) {
        return FoundCoveringModels(std::move(models), std::move(newly_assigned_in));
      }
    }
  }

  FoundModel FindModel(const int min_model_size,
                       const bool propagate_with_learnt,
                       const bool wanted_is_must,
                       const TermMap<Fun, bool>& wanted) {
    //printf("FindModel: min_model_size = %d, propagate_with_learnt = %s, wanted_is_must = %s, wanted =", min_model_size, propagate_with_learnt ? "true" : "false", wanted_is_must ? "true" : "false"); for (Fun f : wanted.keys()) { if (wanted[f]) { printf(" %d", int(f)); } } printf("\n");
    static constexpr double activity_offset = 1000.0;
    static constexpr int max_conflicts = 50;
    int n_conflicts = 0;
    int model_size = 0;
    Sat sat;
    for (const std::vector<Lit>& as : clauses_) {
      sat.AddClause(as,
                    [&](const Fun)   -> Name   { return extra_name_; },
                    [&](const Fun f) -> double { return wanted[f] * activity_offset; });
    }
    sat.set_propagate_with_learnt(propagate_with_learnt);
    TermMap<Fun, Name> model;
    const Sat::Truth truth = sat.Solve(
        [&](int, Sat::CRef, const std::vector<Lit>&, int) -> bool {
          return ++n_conflicts <= max_conflicts;
        },
        [&](int, Lit) -> bool {
          if (min_model_size <= sat.model_size() && model_size < sat.model_size() &&
              (!wanted_is_must || AssignsAll(sat.model(), wanted))) {
            model_size = sat.model_size();
            model = sat.model();
          }
          return true;
        });
    if (truth == Sat::Truth::kSat) {
      //printf("FindModel: true, model_size = %d, assignment =", sat.model_size()); for (const Fun f : sat.model().keys()) { if (assigns(sat.model(), f)) { printf(" (%d = %d)", int(f), int(sat.model()[f])); } } printf("\n");
      assert(AssignsAll(sat.model(), wanted));
      return FoundModel(sat.model());
    } else if (model_size >= min_model_size) {
      //printf("FindModel: true, model_size = %d, assignment =", model_size); for (const Fun f : model.keys()) { if (assigns(model, f)) { printf(" (%d = %d)", int(f), int(model[f])); } } printf("\n");
      return FoundModel(std::move(model));
    } else {
      //printf("FindModel: false\n");
      return FoundModel();
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
  TermMap<Fun, bool>  funs_;
  TermMap<Name, bool> names_;
  Name                           extra_name_;
  //TermMap<Fun, TermMap<Name, bool>> names_;
  //TermMap<Fun, Name>                extra_names_;
};

}  // namespace limbo

#endif  // LIMBO_LIMSAT_H_

