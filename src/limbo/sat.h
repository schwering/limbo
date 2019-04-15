// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_SAT_H_
#define LIMBO_SAT_H_

#include <cstdlib>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/lit.h>

#include <limbo/internal/dense.h>
#include <limbo/internal/ringbuffer.h>

namespace limbo {

class Sat {
 public:
  using CRef = Clause::Factory::CRef;

  explicit Sat() = default;

  Sat(const Sat&)            = delete;
  Sat& operator=(const Sat&) = delete;
  Sat(Sat&&)                 = default;
  Sat& operator=(Sat&&)      = default;

  template<typename ExtraNameFactory>
  void AddLiteral(const Lit a, ExtraNameFactory extra_name = ExtraNameFactory()) {
    trail_.push_back(a);
    Register(a.fun(), a.name(), extra_name(a.fun()));
  }

  template<typename ExtraNameFactory>
  void AddClause(const std::vector<Lit>& as, ExtraNameFactory extra_name = ExtraNameFactory()) {
    if (as.size() == 0) {
      empty_clause_ = true;
    } else if (as.size() == 1) {
      AddLiteral(as[0], extra_name);
    } else {
      const CRef cr = CRef(clausef_.New(as));
      Clause& c = clausef_[cr];
      if (c.valid()) {
        clausef_.Delete(cr, as.size());
        return;
      }
      if (c.unsat()) {
        empty_clause_ = true;
        clausef_.Delete(cr, as.size());
        return;
      }
      assert(c.size() >= 1);
      if (c.size() == 1) {
        AddLiteral(c[0], extra_name);
        clausef_.Delete(cr, as.size());
      } else {
        clauses_.push_back(cr);
        for (const Lit a : c) {
          Register(a.fun(), a.name(), extra_name(a.fun()));
        }
        UpdateWatchers(cr, c);
        trail_head_ = 0;
      }
    }
  }

  void Init() {
    assert(trail_head_ == 0);
    assert(level_size_.size() == 1);
    std::vector<Lit> lits = std::move(trail_);
    trail_.resize(0);
    trail_.reserve(lits.size());
    for (const Lit a : lits) {
      if (falsifies(a)) {
        empty_clause_ = true;
        return;
      }
      Enqueue(a, CRef::kNull);
    }
  }

  void Reset() {
    if (current_level() != Level::kRoot) {
      Backtrack(Level::kRoot);
    }
  }

  void Simplify() {
    Reset();
    assert(level_size_.size() == 1);
    assert(level_size_[0] == 0);
    const CRef conflict = Propagate();
    if (conflict != CRef::kNull) {
      empty_clause_ = true;
      return;
    }
    for (std::vector<CRef>& ws : watchers_) {
      ws.clear();
    }
    int n_clauses = clauses_.size();
    for (int i = 1; i < n_clauses; ++i) {
      const CRef cr = clauses_[i];
      Clause& c = clausef_[cr];
      assert(c.size() >= 2);
      const int removed = c.RemoveIf([this](Lit a) -> bool { return falsifies(a); });
      assert(!c.valid());
      if (c.unsat()) {
        empty_clause_ = true;
        clausef_.Delete(cr, c.size() + removed);
        return;
      } else if (satisfies(c)) {
        clausef_.Delete(cr, c.size() + removed);
        clauses_[i--] = clauses_[--n_clauses];
      } else if (c.size() == 1) {
        Enqueue(c[0], CRef::kNull);
        clausef_.Delete(cr, c.size() + removed);
        clauses_[i--] = clauses_[--n_clauses];
      } else {
        UpdateWatchers(cr, c);
      }
    }
    clauses_.resize(n_clauses);
    int n_units = trail_.size();
    for (int i = 0; i < n_units; ++i) {
      const Lit a = trail_[i];
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      if (!p && !m.null()) {
        assert(m != n);
        trail_[i--] = trail_[--n_units];
        model_data_[f][n].Reset();
      }
      model_data_[f][n].reason = CRef::kNull;
      assert(satisfies(a));
    }
    trail_.resize(n_units);
    trail_head_ = trail_.size();
  }

  const std::vector<CRef>&             clauses()       const { return clauses_; }
  const Clause&                        clause(CRef cr) const { return clausef_[cr]; }
  const internal::DenseMap<Fun, Name>& model()         const { return model_; }

  bool propagate_with_learnt() const { return propagate_with_learned_; }
  void set_propagate_with_learnt(bool b) { propagate_with_learned_ = b; }

  template<typename ConflictPredicate, typename DecisionPredicate>
  int Solve(ConflictPredicate conflict_predicate = ConflictPredicate(),
            DecisionPredicate decision_predicate = DecisionPredicate()) {
    if (empty_clause_) {
      return -1;
    }
    std::vector<Lit> learnt;
    bool go = true;
    while (go) {
      const CRef conflict = Propagate();
      if (conflict != CRef::kNull) {
        if (current_level() == Level::kRoot) {
          return -1;
        }
        Level btlevel;
        Analyze(conflict, &learnt, &btlevel);
        go &= conflict_predicate(current_level(), conflict, learnt, btlevel);
        Backtrack(btlevel);
        assert(learnt.size() >= 1);
        if (learnt.size() > 1) {
          const CRef cr = clausef_.New(learnt, Clause::NormalizationPromise(true));
          Clause& c = clausef_[cr];
          c.set_learnt(true);
          assert(c.size() >= 1);
          assert(!satisfies(c[0]) && !falsifies(c[0]));
          assert(std::all_of(c.begin() + 1, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
          clauses_.push_back(cr);
          UpdateWatchers(cr, c);
          Enqueue(learnt[0], cr);
        } else {
          Enqueue(learnt[0], CRef::kNull);
        }
        learnt.clear();
        funs_.Decay();
      } else {
        Fun f;
        do {
          f = funs_.top();
          if (f.null()) {
            return 1;
          }
          funs_.Remove(f);
        } while (!model_[f].null());
        Name n;
        do {
          assert(fun_n_names_[f] - model_neqs_[f] <= fun_names_[f].size());
          if (fun_names_[f].empty()) {
            return -1;
          }
          n = fun_names_[f].PopFront();
          model_data_[f][n].popped = true;
        } while (model_data_[f][n].model_neq);
        AddNewLevel();
        const Lit a = Lit::Eq(f, n);
        EnqueueEq(a, CRef::kNull);
        go &= decision_predicate(current_level(), a);
      }
      assert(level_size_.back() < trail_.size());
    }
    Backtrack(Level::kRoot);
    return 0;
  }

 private:
  enum class Level : int { kAll = -1, kNull = 0, kRoot = 1 };

  template<typename T>
  class ActivityOrder {
   public:
    explicit ActivityOrder(double bump_step = kBumpStepInit) : bump_step_(bump_step) {}

    ActivityOrder(const ActivityOrder&) = delete;
    ActivityOrder& operator=(const ActivityOrder&) = delete;

    ActivityOrder(ActivityOrder&& ao) :
        bump_step_(ao.bump_step_),
        acti_(std::move(ao.acti_)),
        heap_(std::move(ao.heap_)) {
      heap_.set_less(ActivityCompare(&acti_));
    }
    ActivityOrder& operator=(ActivityOrder&& ao) {
      bump_step_ = ao.bump_step_;
      acti_ = std::move(ao.acti_);
      heap_ = std::move(ao.heap_);
      heap_.set_less(ActivityCompare(&acti_));
      return *this;
    }

    void Capacitate(const int i) { heap_.Capacitate(i); acti_.Capacitate(i); }

    T top()              const { return heap_.top(); }
    bool Contains(T t)   const { return heap_.Contains(t); }
    int size()           const { return heap_.size(); }
    T operator[](int i)  const { return heap_[i]; }
    double activity(T x) const { return acti_[x]; }

    void Insert(T t) { heap_.Insert(t); }
    void Remove(T t) { heap_.Remove(t); }

    void BumpToFront(T t) { Bump(t, acti_[top()] - acti_[t] + bump_step_); }
    void BumpToRear(T t)  { Bump(t, -1 * bump_step_); }
    void BumpUp(T t)      { Bump(t, bump_step_); }
    void Decay()          { bump_step_ /= kDecayFactor; }

   private:
    struct ActivityCompare {
      explicit ActivityCompare(const internal::DenseMap<T, double>* a) : acti_(a) {}
      bool operator()(const T t1, const T t2) const { return (*acti_)[t1] > (*acti_)[t2]; }
     private:
      const internal::DenseMap<T, double>* acti_;
    };

    void Bump(const T t, const double bump) {
      acti_[t] += bump;
      if (acti_[t] > kActivityThreshold) {
        for (double& activity : acti_) {
          activity /= kActivityThreshold;
        }
        bump_step_ /= kActivityThreshold;
      }
      if (heap_.Contains(t)) {
        if (bump >= 0) {
          heap_.Increase(t);
        } else {
          heap_.Decrease(t);
        }
      }
    }

    double                                bump_step_;
    internal::DenseMap<T, double>         acti_;
    internal::MinHeap<T, ActivityCompare> heap_{ActivityCompare(&acti_)};
  };

  struct ModelData {  // meta data for a pair (f,n)
    static constexpr bool kModelNeq = true;
    static constexpr bool kModelEq = false;

    explicit ModelData()
        : seen_subsumed(0), wanted(0), occurs(0), popped(0), model_neq(0), level(0), reason(CRef::kNull) {}

    ModelData(const ModelData&) = default;
    ModelData& operator=(const ModelData&) = default;
    ModelData(ModelData&&) = default;
    ModelData& operator=(ModelData&&) = default;

    void Update(const bool model_neq, const Level level, const CRef reason) {
      this->model_neq = model_neq;
      this->level = unsigned(level);
      this->reason = reason;
    }

    void Reset() {
      assert(!seen_subsumed);
      assert(!wanted);
      assert(occurs);
      model_neq = false;
      level = unsigned(Level::kNull);
      reason = CRef::kNull;
    }

    unsigned seen_subsumed :  1;  // true iff a literal subsumed by f = n / f != n on the trail (helper for Analyze())
    unsigned wanted        :  1;  // true iff a literal complementary to f = n / f != n is wanted (helper for Analyze())
    unsigned occurs        :  1;  // true iff f occurs with n in added clauses or literals
    unsigned popped        :  1;  // true iff n has been popped from fun_names_[f]
    unsigned model_neq     :  1;  // true iff f != n was set or derived
    unsigned level         : 27;  // level at which f = n or f != n was set or derived
    CRef reason;                  // clause which derived f = n or f != n
  };


  static_assert(sizeof(ModelData) == 4 + sizeof(CRef), "ModelData should be 4 + 4 bytes");


  static constexpr double kBumpStepInit = 1.0;
  static constexpr double kActivityThreshold = 1e100;
  static constexpr double kDecayFactor = 0.95;


  void ClauseBump(Clause& c) {
    c.set_activity(c.activity() + clause_bump_step_);
    if (c.activity() > kActivityThreshold) {
      for (const CRef cr : clauses_) {
        Clause& c = clausef_[cr];
        c.set_activity(c.activity() / kActivityThreshold);
      }
      clause_bump_step_ /= kActivityThreshold;
    }
  }

  void ClauseDecay() { clause_bump_step_ /= kDecayFactor; }

  void Register(const Fun f, const Name n, const Name extra_n) {
    CapacitateMaps(f, n, extra_n);
    if (!funs_.Contains(f)) {
      funs_.Insert(f);
      if (!model_data_[f][extra_n].occurs) {
        model_data_[f][extra_n].occurs = true;
        fun_names_[f].PushBack(extra_n);
        ++fun_n_names_[f];
      }
    }
    if (!model_data_[f][n].occurs) {
      model_data_[f][n].occurs = true;
      fun_names_[f].PushBack(n);
      ++fun_n_names_[f];
    }
  }

  void UpdateWatchers(const CRef cr, const Clause& c) {
    assert(&clausef_[cr] == &c);
    assert(!c.unsat());
    assert(!c.valid());
    assert(c.size() >= 2);
    assert(!falsifies(c[0]) || std::all_of(c.begin()+2, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
    assert(!falsifies(c[1]) || std::all_of(c.begin()+2, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
    const Fun f0 = c[0].fun();
    const Fun f1 = c[1].fun();
    assert(std::count(watchers_[f0].begin(), watchers_[f0].end(), cr) == 0);
    assert(std::count(watchers_[f1].begin(), watchers_[f1].end(), cr) == 0);
    watchers_[f0].push_back(cr);
    if (f0 != f1) {
      watchers_[f1].push_back(cr);
    }
    assert(std::count(watchers_[f0].begin(), watchers_[f0].end(), cr) >= 1);
    assert(std::count(watchers_[f1].begin(), watchers_[f1].end(), cr) >= 1);
  }

  CRef Propagate() {
    CRef conflict = CRef::kNull;
    while (trail_head_ < trail_.size() && conflict == CRef::kNull) {
      Lit a = trail_[trail_head_++];
      conflict = Propagate(a);
    }
    assert(std::all_of(clauses_.begin()+1, clauses_.end(), [this](CRef cr) -> bool { return clausef_[cr].learnt() || std::all_of(clausef_[cr].begin(), clausef_[cr].begin()+2, [this, cr](Lit a) -> bool { auto& ws = watchers_[a.fun()]; return std::count(ws.begin(), ws.end(), cr) >= 1; }); }));
    assert(conflict != CRef::kNull || std::all_of(clauses_.begin()+1, clauses_.end(), [this](CRef cr) -> bool { const Clause& c = clausef_[cr]; return c.learnt() || satisfies(c) || (!falsifies(c[0]) && !falsifies(c[1])) || std::all_of(c.begin()+2, c.end(), [this](Lit a) -> bool { return falsifies(a); }); }));
    return conflict;
  }

  // XXX new watchers:
  //
  // Two maps:
  // M1: Lit -> Clauses for (1) and (2)
  // M2: Fun -> Clauses for (2)
  //
  // (1) If f!=n in {c[0],c[1]} => watch f=n in M1
  // (2) If f=n  in {c[0],c[1]} => watch f!=n in M1, f=n' in M2
  //
  // If f=n in trail:  iterate M1[f=n] and M2[f]
  // If f!=n in trail: iterate M1[f!=n]
  //
  // A problem with this design is how to delete clauses that
  // watch f!=n and f=n' when either of them is set. One way
  // could be to do it lazily.
  //
  // One question is what to do if {f=n,f=n'} = {c[0],c[1]}:
  // have one or two entries for c in M2?
  //
  // At the moment it's only one.

  CRef Propagate(const Lit a) {
    CRef conflict = CRef::kNull;
    const Fun f = a.fun();
    std::vector<CRef>& ws = watchers_[f];
    CRef* const begin = ws.data();
    CRef* const end = begin + ws.size();
    CRef* cr_ptr1 = begin;
    CRef* cr_ptr2 = begin;
    while (cr_ptr1 != end) {
      const CRef cr = *cr_ptr1;
      Clause& c = clausef_[cr];

      assert(conflict == CRef::kNull);
      assert(std::count(cr_ptr1, end, cr) == 1);

      if (c.learnt() && !propagate_with_learned_) {
        *cr_ptr2++ = *cr_ptr1++;
        continue;
      }

      const Fun f0 = c[0].fun();
      const Fun f1 = c[1].fun();

      // w is a two-bit number where the i-th bit indicates that c[i] is falsified
      char w = (char(f == f1 && falsifies(c[1])) << 1)
              | char(f == f0 && falsifies(c[0]));
      if (w == 0 || satisfies(c[0]) || satisfies(c[1])) {
        *cr_ptr2++ = *cr_ptr1++;
        continue;
      }

      assert(w == 1 || w == 2 || w == 3);
      assert(bool(w & 1) == (c[0].fun() == f && falsifies(c[0])));
      assert(bool(w & 2) == (c[1].fun() == f && falsifies(c[1])));

      // find new watched literals if necessary
      for (int k = 2, s = c.size(); w != 0 && k < s; ++k) {
        if (!falsifies(c[k])) {
          const int i = w >> 1;
          assert(falsifies(c[i]));
          const Fun fk = c[k].fun();
          if (fk != f0 && fk != f1 && fk != c[1-i].fun()) {
            assert(std::count(watchers_[fk].begin(), watchers_[fk].end(), cr) == 0);
            watchers_[fk].push_back(cr);
          }
          std::swap(c[i], c[k]);
          w = (w - 1) >> 1;  // 11 becomes 01, 10 becomes 00, 01 becomes 00
        }
      }

      assert(w == 0 || std::all_of(c.begin() + 2, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
      assert(bool(w & 1) == (c[0].fun() == f && falsifies(c[0])));
      assert(bool(w & 2) == (c[1].fun() == f && falsifies(c[1])));
      assert(!(c[0].fun() != f && c[1].fun() != f) || w == 0);

      // remove c if f is not watched anymore
      if (w == 0 && c[0].fun() != f && c[1].fun() != f) {
        ++cr_ptr1;
      }

      // conflict or propagate or promote
      if (w != 0) {
        const int i = 1 - (w >> 1);  // 11 becomes 0, 10 becomes 0, 01 becomes 1
        if (w == 3 || falsifies(c[i])) {
          std::memmove(cr_ptr2, cr_ptr1, (end - cr_ptr1) * sizeof(CRef));
          cr_ptr2 += end - cr_ptr1;
          cr_ptr1 = end;
          trail_head_ = trail_.size();
          conflict = cr;
        } else {
          Enqueue(c[i], cr);
          *cr_ptr2++ = *cr_ptr1++;
        }
        ClauseBump(c);
      }
    }
    ws.resize(cr_ptr2 - begin);
    ClauseDecay();
    return conflict;
  }

  void Analyze(CRef conflict, std::vector<Lit>* const learnt, Level* const btlevel) {
    assert(std::all_of(model_data_.begin(), model_data_.end(),
                       [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(),
                       [](const ModelData& d) -> bool { return !d.seen_subsumed && !d.wanted; }); }));
    int depth = 0;
    Lit trail_a = Lit();
    int trail_i = trail_.size() - 1;
    learnt->push_back(trail_a);

    // see_subsuming(a) marks all literals that subsume a as seen,
    // seen_subsumed(a) holds iff some literal subsumed by a has been seen.
    //
    // Proof:
    // (#) The stack contains a literal complementary to a.
    // (*) The stack does not contain a literal that subsumes a.
    // (1) f == n is subsumed (only) by f == n;
    // (2) f == n is complementary (only) to f != n and f == n';
    // (3) f != n is subsumed (only) by f != n and f == n';
    // (4) f != n is complementary (only) to f == n;
    // Here, (*) follows from (#) and the fact that the trail does not contain
    // complementary literals.
    //
    // We show seen_subsumed(a) iff see_subsuming(a'), where a subsumes a'.
    //
    // Case 1:
    //     seen_subsumed(f == n)
    // iff (f,n) or else m and (f,m)
    // iff see_subsuming(f == n) or
    //     see_subsuming(f != n) or
    //     m and see_subsuming(f == m) or
    //     m and see_subsuming(f != m)
    // iff (by ($) and (1))
    //     see_subsuming(f == n) or
    //     see_subsuming(f != n) or
    //     m and see_subsuming(f != m)
    // iff (by (#), (4), (1), ($))
    //     see_subsuming(f == n) or
    //     m and see_subsuming(f != m)
    // iff (by (#) and (4))
    //     see_subsuming(f == n) or
    //     see_subsuming(f != n') for n' distinct from n
    //
    // Case 2:
    //     seen_subsumed(f != n)
    // iff (f,n) or else m and (f,m)
    // iff see_subsuming(f == n) or
    //     see_subsuming(f != n)
    // iff (by (#), (2), (3), ($))
    //     see_subsuming(f != n)
    auto see_subsuming = [this](const Lit a) -> void {
      assert(falsifies(a));
      assert(a.pos() || !model_[a.fun()].null());
      assert(model_[a.fun()].null() || (a.pos() != (model_[a.fun()] == a.name())));
      const Fun f = a.fun();
      const Name n = a.name();
      model_data_[f][n].seen_subsumed = 1;
    };
    auto seen_subsumed = [this](const Lit a) -> bool {
      assert(falsifies(a));
      assert(model_[a.fun()].null() || (a.pos() != (model_[a.fun()] == a.name())));
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      return model_data_[f][n].seen_subsumed || (p && !m.null() && model_data_[f][m].seen_subsumed);
    };

    // want_complementary_on_level(a,l) marks all literals on level l that
    // are complementary to a as wanted.
    // By the following reasoning, it suffices to mark only a single literal
    // that implicitly also determines the others as wanted.
    // When we want a complementary literal to f == n, we prefer f != n over
    // f == model_[f] because this will become f == n in the conflict clause.
    // This also means that we want exactly one literal, which eliminates the
    // need for traversing the whole level again to reset the wanted flag.
    auto want_complementary_on_level = [this](const Lit a, Level l) -> void {
      assert(falsifies(a));
      assert(Level(model_data_[a.fun()][a.name()].level) <= l);
      assert(a.pos() || Level(model_data_[a.fun()][a.name()].level) == l);
      assert(a.pos() || model_[a.fun()] == a.name());
      assert(!a.pos() || Level(model_data_[a.fun()][a.name()].level) != l || model_data_[a.fun()][a.name()].model_neq);
      assert(!a.pos() || Level(model_data_[a.fun()][a.name()].level) == l || !model_[a.fun()].null());
      assert(!a.pos() || Level(model_data_[a.fun()][a.name()].level) == l || Level(model_data_[a.fun()][model_[a.fun()]].level) == l);
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      model_data_[f][Level(model_data_[f][n].level) == l ? n : m].wanted = true;
    };
    // wanted_complementary_on_level(a,l) iff a on level l is wanted.
    auto wanted_complementary_on_level = [this](const Lit a, Level l) -> bool {
      assert(falsifies(a));
      assert(Level(model_data_[a.fun()][a.name()].level) <= l);
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      return (!p && model_data_[f][n].wanted) ||
             (p && ((Level(model_data_[f][n].level) == l && model_data_[f][n].wanted) ||
                    (!m.null() && model_data_[f][m].wanted)));
    };
    // We un-want every trail literal after it has been traversed.
    auto wanted = [this](const Lit a) -> bool {
      assert(satisfies(a));
      const Fun f = a.fun();
      const Name n = a.name();
      return model_data_[f][n].wanted;
    };

    auto handle_conflict = [this, &trail_a, &learnt, &depth, &see_subsuming, &seen_subsumed,
                            &want_complementary_on_level, &wanted_complementary_on_level](const Lit a) -> void {
      if (trail_a == a) {
        return;
      }
      assert(falsifies(a));
      assert(!satisfies(a));
      const Level l = level_of_complementary(a);
      if (l == Level::kRoot || seen_subsumed(a) || wanted_complementary_on_level(a, l)) {
        return;
      }
      if (l < current_level()) {
        learnt->push_back(a);
        see_subsuming(a);
      } else {
        assert(l == current_level());
        ++depth;
        want_complementary_on_level(a, l);
      }
      funs_.BumpUp(a.fun());
      //fun_names_[a.fun()].BumpUp(a.name());
    };

    do {
      assert(conflict != CRef::kNull);
      // XXX TODO test that kDomainRef can cause a conflict (probably true because f=n is added when domain reaches size 1)
      if (conflict == CRef::kDomain) {
        assert(!trail_a.null());
        assert(trail_a.pos());
        const Fun f = trail_a.fun();
        for (int i = 1; i < model_data_[f].upper_bound(); ++i) {
          const Name n = Name::FromId(i);
          if (model_data_[f][n].occurs) {
            const Lit a = Lit::Eq(f, n);
            handle_conflict(a);
          }
        }
      } else {
        for (const Lit a : clausef_[conflict]) {
          handle_conflict(a);
        }
      }
      assert(depth > 0);

      while (!wanted(trail_[trail_i--])) {
        assert(trail_i >= 0);
      }
      trail_a = trail_[trail_i + 1];
      model_data_[trail_a.fun()][trail_a.name()].wanted = false;
      --depth;
      conflict = reason_of(trail_a);
    } while (depth > 0);
    (*learnt)[0] = trail_a.flip();

    for (const Lit a : *learnt) {
      model_data_[a.fun()][a.name()].seen_subsumed = false;
    }

    learnt->resize(Clause::Normalize(learnt->size(), learnt->data(), Clause::InvalidityPromise(true)));

    if (learnt->size() == 1) {
      *btlevel = Level::kRoot;
    } else {
      assert(learnt->size() >= 2);
      int max = 1;
      *btlevel = level_of_complementary((*learnt)[max]);
      for (int i = 2, s = learnt->size(); i != s; ++i) {
        Level l = level_of_complementary((*learnt)[i]);
        if (*btlevel < l) {
          max = i;
          *btlevel = l;
        }
      }
      std::swap((*learnt)[1], (*learnt)[max]);
    }
    assert(level_of(trail_a) > *btlevel && *btlevel >= Level::kRoot);
    assert(std::all_of(learnt->begin(), learnt->end(), [this](Lit a) -> bool { return falsifies(a); }));
    assert(std::all_of(learnt->begin(), learnt->end(), [this](Lit a) -> bool { return !satisfies(a); }));
    assert(std::all_of(model_data_.begin(), model_data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const ModelData& d) -> bool { return !d.seen_subsumed; }); }));
    assert(std::all_of(model_data_.begin(), model_data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const ModelData& d) -> bool { return !d.wanted; }); }));
  }

  void AddNewLevel() { level_size_.push_back(trail_.size()); }

  void Enqueue(const Lit a, const CRef reason) {
    assert(model_data_[a.fun()][a.name()].occurs);
    return a.pos() ? EnqueueEq(a, reason) : EnqueueNeq(a, reason);
  }

  void EnqueueEq(const Lit a, const CRef reason) {
    assert(a.pos());
    assert(!falsifies(a));
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    if (m.null()) {
      assert(!satisfies(a));
      assert(fun_names_[a.fun()].size() >= !a.pos());
      trail_.push_back(a);
      model_[f] = n;
      model_data_[f][n].Update(ModelData::kModelEq, current_level(), reason);
      ++model_eqs_;
    }
    assert(satisfies(a));
  }

  void EnqueueNeq(const Lit a, const CRef reason) {
    assert(!a.pos());
    assert(!falsifies(a));
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    if (m.null() && !model_data_[f][n].model_neq) {
      assert(!model_data_[a.fun()][a.name()].popped);
      assert(fun_names_[a.fun()].size() >= !a.pos());
      assert(!satisfies(a));
      trail_.push_back(a);
      model_data_[f][n].Update(ModelData::kModelNeq, current_level(), reason);
      ++model_neqs_[f];
      if (fun_n_names_[f] - model_neqs_[f] == 1) {
        Name m;
        do {
          assert(fun_n_names_[f] - model_neqs_[f] <= fun_names_[f].size());
          assert(!fun_names_[f].empty());
          m = fun_names_[f].PopFront();
          model_data_[f][m].popped = true;
        } while (model_data_[f][m].model_neq);
        assert(!satisfies(Lit::Eq(f, m)) && !falsifies(Lit::Eq(f, m)));
        EnqueueEq(Lit::Eq(f, m), CRef::kDomain);
      } else {
        funs_.BumpToFront(f);
      }
    }
    assert(satisfies(a));
  }

  void Backtrack(const Level l) {
    for (auto a = trail_.rbegin(), e = trail_.rend() - level_size_[int(l)]; a != e; ++a) {
      const bool p = a->pos();
      const Fun f = a->fun();
      const Name n = a->name();
      if (p) {
        model_[f] = Name();
        --model_eqs_;
        if (!funs_.Contains(f)) {
          funs_.Insert(f);
        }
      } else {
        --model_neqs_[f];
      }
      if (model_data_[f][n].popped) {
        fun_names_[f].PushBack(n);
        model_data_[f][n].popped = false;
      }
      model_data_[f][n].Reset();
      assert(!satisfies(*a) && !falsifies(*a));
      assert(fun_n_names_[f] - model_neqs_[f] <= fun_names_[f].size());
    }
    trail_.resize(level_size_[int(l)]);
    trail_head_ = trail_.size();
    level_size_.resize(int(l));
  }

  bool satisfies(const Lit a) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((p && m == n) ||
            (!p && ((!m.null() && m != n) || model_data_[f][n].model_neq)));
  }

  bool falsifies(const Lit a) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((!p && m == n) ||
            (p && ((!m.null() && m != n) || model_data_[f][n].model_neq)));
  }

  bool satisfies(const Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this](Lit a) -> bool { return satisfies(a); });
  }

  bool falsifies(const Clause& c) const {
    return std::all_of(c.begin(), c.end(), [this](Lit a) -> bool { return falsifies(a); });
  }

  Level level_of(const Lit a) const {
    assert(satisfies(a));
    assert(!a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return Level(!p && model_data_[f][n].model_neq ? model_data_[f][n].level : model_data_[f][m].level);
  }

  Level level_of_complementary(const Lit a) const {
    assert(falsifies(a));
    assert(a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return Level(p && model_data_[f][n].model_neq ? model_data_[f][n].level : model_data_[f][m].level);
  }

  CRef reason_of(const Lit a) const {
    assert(satisfies(a));
    assert(!a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return !p && model_data_[f][n].model_neq ? model_data_[f][n].reason : model_data_[f][m].reason;
  }

  Level current_level() const { return Level(level_size_.size()); }

  void CapacitateMaps(const Fun f, const Name n, const Name extra_n) {
    const int max_ni = int(n) > int(extra_n) ? int(n) : int(extra_n);
    const int fi = int(f) >= model_data_.upper_bound() ? int(f) : -1;
    const int ni = model_data_.upper_bound() == 0 || max_ni >= model_data_[0].upper_bound() ? max_ni : -1;
    const int fig = (fi + 1) * 1.5;
    const int nig = ni >= 0 ? (ni + 1) * 3 / 2 : model_data_[0].upper_bound();
    if (fi >= 0) {
      funs_.Capacitate(fig);
      fun_names_.Capacitate(fig);
      fun_n_names_.Capacitate(fig);
      watchers_.Capacitate(fig);
      model_.Capacitate(fig);
      model_data_.Capacitate(fig);
      model_neqs_.Capacitate(fig);
    }
    if (fi >= 0 || ni >= 0) {
      for (internal::DenseMap<Name, ModelData>& ds : model_data_) {
        ds.Capacitate(nig);
      }
      //for (RingBuffer<Name>& ns : fun_names_) {
      //  ns.Capacitate(nig);
      //}
    }
    if (ni >= 0) {
      for (internal::DenseMap<Name, ModelData>& ds : model_data_) {
        ds.Capacitate(nig);
      }
    }
  }


  // empty_clause_ is true iff the empty clause has been derived.
  bool empty_clause_ = false;

  // clauses_ is the sequence of clauses added initially or learnt where
  // learnt clauses.
  Clause::Factory clausef_;
  std::vector<CRef> clauses_ = std::vector<CRef>(1, CRef::kNull);
  bool propagate_with_learned_ = true;
  double clause_bump_step_ = kBumpStepInit;

  // funs_ ranks unassigned functions by their activity.
  // fun_names_ contains names n for each function f; some of them may
  // be excluded by literals f != n on the trail.
  // fun_n_names_ is the number of names that occured per function.
  ActivityOrder<Fun>                        funs_;
  internal::DenseMap<Fun, int>              fun_n_names_;
  internal::DenseMap<Fun, RingBuffer<Name>> fun_names_;

  // watchers_ maps every function to a sequence of clauses that watch it.
  // Every clause watches two functions, and when a literal with this function
  // is propagated, the watching clauses are inspected.
  internal::DenseMap<Fun, std::vector<CRef>> watchers_;

  // trail_ is a sequence of literals in the order they were derived.
  // level_size_ groups the literals of trail_ into chunks by their level at
  // which they were derived, where level_size_[l] determines the number of
  // literals set or derived up to level l.
  // trail_head_ is the index of the first literal of trail_ that hasn't been
  // propagated yet.
  std::vector<Lit> trail_;
  std::vector<int> level_size_ = std::vector<int>(1, trail_.size());
  int              trail_head_ = 0;

  // model_ is an assignment of functions to names, i.e., positive literals.
  // model_data_ is meta data for every function and name pair (cf. ModelData).
  internal::DenseMap<Fun, Name>                                model_;
  internal::DenseMap<Fun, internal::DenseMap<Name, ModelData>> model_data_;
  int                                                          model_eqs_ = 0;
  internal::DenseMap<Fun, int>                                 model_neqs_;
};

}  // namespace limbo

#endif  // LIMBO_SAT_H_

