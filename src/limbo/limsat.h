// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Limited satisfiability solver that checks if for all functions f_1, ..., f_k,
// there are names n_1, ..., n_k such that the partial model obtained by closing
// f_1 = n_1, ..., f_k = n_k under unit propagation with the clauses satisfies
// all those clauses and does not satisfy the query.
//
// Provided the NNF of the query does not contain valid subclauses, the above
// statement is equivalent to: for all f_1, ..., f_k, for some n_1, ..., n_k,
// there is a multi-valued world that satisfies all clauses closed under unit
// propagation with f_1 = n_1, ..., f_k = n_k and is consistent for all units,
// and falsifies the clause.
//
// This formulation in turn is the negation of the (new) semantics of limited
// belief.

#ifndef LIMBO_LIMSAT_H_
#define LIMBO_LIMSAT_H_

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <vector>

#include <limbo/formula.h>
#include <limbo/sat.h>
#include <limbo/internal/dense.h>
#include <limbo/internal/subsets.h>

namespace limbo {

class LimSat {
 public:
  using LitVec = std::vector<Lit>;

  explicit LimSat() = default;

  LimSat(const LimSat&)            = delete;
  LimSat& operator=(const LimSat&) = delete;
  LimSat(LimSat&&)                 = default;
  LimSat& operator=(LimSat&&)      = default;

  bool AddClause(const LitVec& as) { return AddClause(LitVec(as)); }

  bool AddClause(LitVec&& as) {
    std::sort(as.begin(), as.end());
    auto p = clauses_.insert(std::move(as));
    if (p.second) {
      clauses_vec_.push_back(*p.first);
      //clauses_not_yet_added_.push_back(p.first);
      for (const Lit a : *p.first) {
        const Fun f = a.fun();
        const Name n = a.name();
        domains_.FitForKey(f);
        domains_[f].FitForKey(n);
        domains_[f][n] = true;
        extra_name_id_ = std::max(n.id() + 1, extra_name_id_);
        if (!sat_.registered(f, n)) {
          sat_.Register(f, n);
        }
      }
    }
    return p.second;
  }

  void set_extra_name_contained(const bool b) { extra_name_contained_ = b; }
  bool extra_name_contained() const { return extra_name_contained_; }

  const std::set<LitVec>& clauses() const { return clauses_; }

  bool Solve(const int belief_level, const RFormula& query) {
    UpdateDomainsForQuery(query);
    auto query_pred = [&query](const TermMap<Fun, Name>& model, std::vector<Lit>* nogood) {
      return query.SatisfiedBy(model, nogood);
    };
    auto model_found = [](const TermMap<Fun, Name>&) {};
    bool sat = FindModels(belief_level, query_pred, model_found);
    return sat;
  }

  internal::Maybe<Name> Solve(const int belief_level, const Fun f) {
    UpdateDomainsForQuery(f);
    Name n;
    Formula query = Formula();
    // Find a first model without a set query (i.e., query is unsatisfiable) and
    // remember f's value. In the subsequent models, require that 
    // 
    auto query_pred = [query = &query](const TermMap<Fun, Name>& model, std::vector<Lit>* nogood) {
      if (query) {
        return false;
      } else {
        return query->readable().SatisfiedBy(model, nogood);
      }
    };
    auto model_found = [f, n = &n, query = &query](const TermMap<Fun, Name>& model) {
      *n = model[f];
      if (!n->null()) {
        *query = Formula::Lit(Lit::Eq(f, *n));
      }
    };
    return FindModels(belief_level, query_pred, model_found) ? internal::Just(n) : internal::Nothing;
  }

 private:
  enum class SolverType { kWithLearntClauses, kWithoutLearntClauses };

  enum class Intensity { kCould, kShould, kMust };

  struct FoundModel {
    FoundModel() = default;
    FoundModel(TermMap<Fun, Name>&& model) : model(std::move(model)), succ(true) {}
    FoundModel(const TermMap<Fun, Name>& model) : model(model), succ(true) {}
    TermMap<Fun, Name> model{};
    bool succ = false;
  };

  struct FoundCoveringModels {
    FoundCoveringModels() = default;
    FoundCoveringModels(std::vector<TermMap<Fun, Name>>&& models, std::vector<std::vector<Fun>>&& newly_assigned_in)
        : models(std::move(models)), newly_assigned_in(std::move(newly_assigned_in)), all_covered(true) {}
    std::vector<TermMap<Fun, Name>> models{};
    std::vector<std::vector<Fun>> newly_assigned_in{};
    bool all_covered = false;
  };

  struct AssignedFunctions {
    AssignedFunctions(std::vector<Fun>&& newly_assigned, bool all_assigned)
        : newly_assigned(std::move(newly_assigned)), all_assigned(std::move(all_assigned)) {}
    std::vector<Fun> newly_assigned{};
    bool all_assigned = false;
  };

#if 1
  class Activity {
   public:
    explicit Activity() = default;
    explicit Activity(const double d) : val_(d) {}

    Activity& operator+=(const double d) {
      if (neg(val_)) {
        val_ -= d;
      } else {
        val_ += d;
      }
      return *this;
    }

    Activity& operator/=(const double d) {
      val_ /= d;
      return *this;
    }

    bool operator>(const Activity a) const {
      return (!neg(val_) &&  neg(a.val_)) ||
             (!neg(val_) && !neg(a.val_) && val_ > a.val_) ||
             ( neg(val_) &&  neg(a.val_) && val_ < a.val_);
    }

    bool operator>(const double d) const {
      return !neg(val_) ? (val_ > d) : (val_ < d);
    }

   private:
    static bool neg(const double d) { return std::signbit(d); }

    double val_ = +0.0;
  };
#else
  using Activity = double;
#endif

  static bool assigns(const TermMap<Fun, Name>& model, const Fun f) {
    return model.key_in_range(f) && !model[f].null();
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

  static int AssignsNumber(const TermMap<Fun, Name>& model, const TermMap<Fun, bool>& wanted) {
    int n = 0;
    for (const Fun f : wanted.keys()) {
      n += wanted[f] && assigns(model, f);
    }
    return n;
  }

  template<typename T>
  static std::vector<Fun> Merge(const std::vector<T>& xs, const std::vector<Fun>& ys) {
    std::vector<Fun> zs;
    std::set_union(xs.begin(), xs.end(), ys.begin(), ys.end(), std::back_inserter(zs));
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

  template<typename QueryPredicate, typename ModelFoundFunction>
  bool FindModels(const int min_model_size, QueryPredicate query_satisfied, ModelFoundFunction model_found) {
    //printf("FindModels:%d\n", __LINE__);
    // Find models such that every function is assigned a value in some model.
    // For example, consider a problem with functions 1,2,3,4,5 and minimum
    // model size 2.
    // We might find two models M1 and M2 that assign 1,2,3 and 3,4,5, which
    // covers all functions. M1 and M2 imply models that assign the subsets of
    // cardinality of size 2 of {1,2,3} and {3,4,5}, that is,
    // {1,2}, {2,3}, {1,3}, and {3,4}, {4,5}, {3,5}.
    const FoundCoveringModels fcm = FindCoveringModels(min_model_size, query_satisfied, model_found);
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
      TermMap<Fun, bool> wanted;
      wanted.FitForKey(domains_.upper_bound_key(), false);
      for (const Fun f : must) {
        wanted[f] = true;
      }
      constexpr bool propagate_with_learnt = false;
      constexpr Intensity want_intensity = Intensity::kMust;
      const FoundModel fm = FindModel(min_model_size, propagate_with_learnt, want_intensity, wanted, query_satisfied);
      return fm.succ;
    });
  }

  template<typename QueryPredicate, typename ModelFoundFunction>
  FoundCoveringModels FindCoveringModels(const int min_model_size,
                                         QueryPredicate query_satisfied,
                                         ModelFoundFunction model_found) {
    std::vector<TermMap<Fun, Name>> models;
    std::vector<std::vector<Fun>> newly_assigned_in;
    TermMap<Fun, bool> wanted;
    wanted.FitForKey(domains_.upper_bound_key());
    for (const Fun f : domains_.keys()) {
      wanted[f] = !domains_[f].empty();
    }
    bool propagate_with_learnt = true;
    Intensity want_intensity = Intensity::kShould;
    for (;;) {
      const FoundModel fm = FindModel(min_model_size, propagate_with_learnt, want_intensity, wanted, query_satisfied);
      if (propagate_with_learnt && !fm.succ) {
        propagate_with_learnt = false;
        continue;
      }
      AssignedFunctions gaf = GetAndUnwantNewlyAssignedFunctions(fm.model, &wanted);
      if (want_intensity == Intensity::kCould && gaf.newly_assigned.empty()) {
        want_intensity = Intensity::kCould;
        continue;
      }
      if (!fm.succ || (min_model_size > 0 && gaf.newly_assigned.empty())) {
        return FoundCoveringModels();
      }
      model_found(fm.model);
      if (min_model_size == 0) {
        return FoundCoveringModels(std::move(models), std::move(newly_assigned_in));
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

  template<typename QueryPredicate>
  FoundModel FindModel(const int min_model_size,
                       const bool propagate_with_learnt,
                       const Intensity want_intensity,
                       const TermMap<Fun, bool>& wanted,
                       QueryPredicate query_satisfied) {
    //printf("FindModel: min_model_size = %d, propagate_with_learnt = %s, want_intensity = %s, wanted =", min_model_size, propagate_with_learnt ? "true" : "false", want_intensity ? "true" : "false"); for (Fun f : wanted.keys()) { if (wanted[f]) { printf(" %d", f.id()); } } printf("\n");
    auto activity = [&wanted, want_intensity](Fun f) {
      return Activity(wanted.key_in_range(f) && wanted[f] ? kActivityOffset : true||want_intensity > Intensity::kCould ? -1.0 : 1.0);
    };
#if 1
    InitSat(false && propagate_with_learnt, activity);
#else
    sat_ = Sat<Activity>();
    for (const auto& c : clauses_vec_) {
      for (const Lit a : c) {
        sat_.Register(a.fun(), a.name(), activity);
      }
    }
    if (!extra_name_contained_) {
      sat_.RegisterExtraName(Name::FromId(extra_name_id_));
    }
    for (const auto& c : clauses_vec_) {
      sat_.AddClause(c);
    }
    sat_.Reset(Sat<Activity>::KeepLearnt(propagate_with_learnt), activity);
#endif
    sat_.set_propagate_with_learnt(propagate_with_learnt);
    TermMap<Fun, Name> partial_model;
    int partial_model_size = -1;
    int n_conflicts = 0;
    int partial_assigns_number = -1;
    int partial_last_conflict = -1;
    const Sat<Activity>::Truth truth = sat_.Solve(
        [&](int, Sat<Activity>::CRef, const LitVec&, int) -> bool {
          ++n_conflicts;
          return partial_last_conflict < 0 || n_conflicts - partial_last_conflict < kMaxConflicts;
        },
        [&](int, Lit) -> bool {
          int assigns_number = -1;
          if (min_model_size <= sat_.model_size() && partial_model_size < sat_.model_size() &&
              (want_intensity < Intensity::kMust || AssignsAll(sat_.model(), wanted)) && !query_satisfied(partial_model, nullptr) &&
              (want_intensity == Intensity::kMust || (assigns_number = AssignsNumber(sat_.model(), wanted)) > partial_assigns_number)) {
            partial_model_size     = sat_.model_size();
            partial_model          = sat_.model();
            partial_last_conflict  = n_conflicts;
            partial_assigns_number = assigns_number;
          }
          return true;
        },
        [&](const TermMap<Fun, Name>& model, LitVec* nogood) -> bool {
          const bool sat = query_satisfied(model, nogood);
          int assigns_number = -1;
          if (!sat && min_model_size <= sat_.model_size() && partial_model_size < sat_.model_size() &&
              (want_intensity < Intensity::kMust || AssignsAll(sat_.model(), wanted)) &&
              (want_intensity == Intensity::kMust || (assigns_number = AssignsNumber(sat_.model(), wanted)) > partial_assigns_number)) {
            partial_model_size     = sat_.model_size();
            partial_model          = sat_.model();
            partial_last_conflict  = n_conflicts;
            partial_assigns_number = assigns_number;
          }
          return sat;
        });
    if (truth == Sat<Activity>::Truth::kSat) {
      //printf("FindModel %d: true, partial_model_size = %d, assignment =", __LINE__, sat_.model_size()); for (const Fun f : sat_.model().keys()) { if (assigns(sat_.model(), f)) { printf(" (%d = %d)", f.id(), sat_.model()[f].id()); } } printf("\n");
      assert(AssignsAll(sat_.model(), wanted));
      return FoundModel(sat_.model());
    } else if (partial_model_size >= min_model_size) {
      //printf("FindModel %d: true, partial_model_size = %d, assignment =", __LINE__, partial_model_size); for (const Fun f : partial_model.keys()) { if (assigns(partial_model, f)) { printf(" (%d = %d)", f.id(), partial_model[f].id()); } } printf("\n");
      return FoundModel(std::move(partial_model));
    } else {
      //printf("FindModel %d: false\n", __LINE__);
      return FoundModel();
    }
  }

  void UpdateDomainsForQuery(const RFormula& query) {
    for (const Alphabet::Symbol& s : query) {
      if (s.tag == Alphabet::Symbol::kStrippedLit) {
        const Fun f = s.u.a.fun();
        const Name n = s.u.a.name();
        domains_.FitForKey(f);
        domains_[f].FitForKey(n, false);
        if (!domains_[f][n]) {
          domains_[f][n] = true;
          extra_name_id_ = std::max(n.id() + 1, extra_name_id_);
          sat_.Register(f, n);
        }
      } else {
        assert(!s.stripped());
      }
    }
  }

  void UpdateDomainsForQuery(const Fun f) {
    domains_.FitForKey(f);
  }

  template<typename ActivityFunction>
  void InitSat(const bool keep_learnt, ActivityFunction activity = ActivityFunction()) {
    if (!extra_name_contained_) {
      sat_.RegisterExtraName(Name::FromId(extra_name_id_));
      extra_name_contained_ = true;
    }
    sat_.Reset(Sat<Activity>::KeepLearnt{keep_learnt}, activity);
    for (; sat_init_index_ < int(clauses_vec_.size()); ++sat_init_index_) {
      sat_.AddClause(clauses_vec_[sat_init_index_]);
    }
  }

  static constexpr double kActivityOffset = 1000.0;
  static constexpr int    kMaxConflicts   = 50;

  std::set<LitVec>    clauses_{};
  std::vector<LitVec> clauses_vec_{};

  TermMap<Fun, TermMap<Name, bool>> domains_{};
  Name::id_t                        extra_name_id_ = 1;
  bool                              extra_name_contained_ = false;

  Sat<Activity> sat_{};
  int           sat_init_index_ = 0;
};

}  // namespace limbo

#endif  // LIMBO_LIMSAT_H_

