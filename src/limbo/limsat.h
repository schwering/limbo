// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_LIMSAT_H_
#define LIMBO_LIMSAT_H_

#include <algorithm>
#include <vector>

#include <limbo/sat.h>
#include <limbo/internal/dense.h>

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

  void GetNewlyAssigned(const internal::DenseMap<Fun, Name> model,
                        internal::DenseMap<Fun, bool>* wanted,
                        std::vector<Fun>* funs,
                        bool* all_assigned) {
    funs->clear();
    *all_assigned = true;
    for (const Fun f : wanted->keys()) {
      if ((*wanted)[f]) {
        if (!model[f].null()) {
          if ((*wanted)[f]) { printf("newly set: %d [%d, %d]\n", int(f), bool((*wanted)[f]), bool(funs_[f])); }
          (*wanted)[f] = false;
          funs->push_back(f);
        } else {
          //printf("not set: %d [%d, %d]\n", int(f), bool((*wanted)[f]), bool(funs_[f]));
          *all_assigned = false;
        }
      }
    }
  }

  bool FindModels(int min_model_size) {
    // newly_assigned_in[i] contains the functions first assigned in model i
    std::vector<std::vector<Fun>> newly_assigned_in;
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
      GetNewlyAssigned(model, &wanted, &newly_assigned, &all_assigned);
      if (newly_assigned.empty() && !wanted_is_must) {
        wanted_is_must = true;
        continue;
      }
      if (newly_assigned.empty() && min_model_size == 0) {
        return true;
      }
      for (auto it = newly_assigned_in.begin(); it != newly_assigned_in.end(); ) {
        if (it->size() <= newly_assigned.size() &&
            std::includes(newly_assigned.begin(), newly_assigned.end(), it->begin(), it->end())) {
          it = newly_assigned_in.erase(it);
        } else {
          ++it;
        }
      }
      newly_assigned_in.push_back(newly_assigned);
    }

    std::vector<int> not_yet_assigned_in(newly_assigned_in.size());
    for (int i = int(not_yet_assigned_in.size()) - 2; i >= 0; --i) {
      not_yet_assigned_in[i] = int(newly_assigned_in[i+1].size()) + not_yet_assigned_in[i+1];
    }

    return AllCombinedSubsetsOfSize(newly_assigned_in, not_yet_assigned_in, min_model_size, [&](const std::vector<Fun>& funs) {
      internal::DenseMap<Fun, Name> model;
      internal::DenseMap<Fun, bool> wanted(max_fun(), false);
      for (const Fun f : funs) {
        wanted[f] = true;
      }
      constexpr bool propagate_with_learnt = false;
      constexpr bool wanted_is_must = true;
      const bool succ = FindModel(min_model_size, propagate_with_learnt, wanted_is_must, wanted, &model);
      return succ;
    });
  }

  template<typename UnaryPredicate>
  bool AllCombinedSubsetsOfSize(const std::vector<std::vector<Fun>>& newly_assigned_in,
                                const std::vector<int>& not_yet_assigned_in,
                                const int need,
                                UnaryPredicate pred = UnaryPredicate()) {
    std::vector<Fun> funs;
    return AllCombinedSubsetsOfSize(newly_assigned_in, not_yet_assigned_in, need, 0, &funs, pred);
  }

  template<typename UnaryPredicate>
  bool AllCombinedSubsetsOfSize(const std::vector<std::vector<Fun>>& newly_assigned_in,
                                const std::vector<int>& not_yet_assigned_in,
                                const int need,
                                const int index,
                                std::vector<Fun>* funs,
                                UnaryPredicate pred = UnaryPredicate()) {
    assert(newly_assigned_in.size() == not_yet_assigned_in.size());
    assert(index < int(newly_assigned_in.size()));
    assert(!newly_assigned_in[index].empty());
    if (index == int(newly_assigned_in.size())) {
      return need == 0 && pred(*funs);
    }
    const std::vector<Fun> fafs = newly_assigned_in[index];
    // What is the minimum number of functions we need to assign from this bucket?
    //     (need - min_assign < not_yet_assigned_in[index] - (not_yet_assigned_in.size() - index - 1))
    // <=> (min_assign > need - not_yet_assigned_in[index] + not_yet_assigned_in.size() - index - 1)
    // <=> (min_assign >= need - not_yet_assigned_in[index] + not_yet_assigned_in.size() - index - 1)
    const int min_assign = std::max(0, need - not_yet_assigned_in[index] + int(not_yet_assigned_in.size()) - index - 1);
    // What is the maximum number of functions we can assign from this buckt?
    const int max_assign = std::min(need, int(fafs.size()) - 1);
    for (int i = min_assign; i < max_assign; ++i) {
      const bool succ = AllSubsetsOfSize(fafs.begin(), fafs.end(), i, funs, [&](std::vector<Fun>* funs) {
        return AllCombinedSubsetsOfSize(newly_assigned_in, not_yet_assigned_in, need - i, index + 1, funs, pred);
      });
      if (!succ) {
        return false;
      }
    }
    return true;
  }

  template<typename RandomAccessIt, typename UnaryPredicate>
  bool AllSubsetsOfSize(RandomAccessIt first,
                        RandomAccessIt last,
                        const int subset_size,
                        std::vector<Fun>* funs,
                        UnaryPredicate pred = UnaryPredicate()) {
    auto dist = std::distance(first, last);
    if (dist == 0) {
      return subset_size == 0 && pred(funs);
    } else if (dist < subset_size) {
      return false;
    } else {
      if (dist > subset_size) {
        const bool succ = AllSubsetsOfSize(std::next(first), last, subset_size, funs, pred);
        if (!succ) {
          return false;
        }
      }
      funs->push_back(*first);
      const bool succ = AllSubsetsOfSize(std::next(first), last, subset_size - 1, funs, pred);
      funs->pop_back();
      return succ;
    }
  }

  bool FindModel(const int min_model_size,
                 const bool propagate_with_learnt,
                 const bool wanted_is_must,
                 const internal::DenseMap<Fun, bool>& wanted,
                 internal::DenseMap<Fun, Name>* model) {
    printf("min_model_size = %d, propagate_with_learnt = %s, wanted_is_must = %s, wanted =", min_model_size, propagate_with_learnt ? "true" : "false", wanted_is_must ? "true" : "false");
    for (Fun f : wanted.keys()) {
      if (wanted[f]) {
        printf(" %d", int(f));
      }
    }
    printf("\n");
    auto wanted_covered = [&](const internal::DenseMap<Fun, Name>& m) -> bool {
      for (Fun f : wanted.keys()) {
        if (wanted[f] && m[f].null()) {
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
        [&](int, Lit) -> bool {
          if (min_model_size <= model_size && model_size < sat.model_size() &&
              (!wanted_is_must || wanted_covered(sat.model()))) {
            model_size = sat.model_size();
            *model = sat.model();
          }
          return true;
        });
    printf("truth = %s\n", truth == Sat::Truth::kSat ? "SAT" : truth == Sat::Truth::kUnsat ? "UNSAT" : "UNKNOWN");
    printf("model_size = %d\n", model_size);
    printf("return value = %d\n", truth == Sat::Truth::kSat ? true : model_size >= min_model_size);
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

