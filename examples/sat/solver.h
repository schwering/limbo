// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
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

#include <limbo/internal/dense.h>

namespace limbo {

template<typename T>
class ActivityOrder {
 public:
  explicit ActivityOrder(double bump_step = 1.0) : bump_step_(bump_step) {}

  ActivityOrder(const ActivityOrder&) = delete;
  ActivityOrder& operator=(const ActivityOrder&) = delete;

  ActivityOrder(ActivityOrder&& ao) :
      bump_step_(std::move(ao.bump_step_)),
      acti_(std::move(ao.acti_)),
      heap_(std::move(ao.heap_)) {
    heap_.set_less(ActivityCompare(&acti_));
  }
  ActivityOrder& operator=(ActivityOrder&& ao) {
    bump_step_ = std::move(ao.bump_step_);
    acti_ = std::move(ao.acti_);
    heap_ = std::move(ao.heap_);
    heap_.set_less(ActivityCompare(&acti_));
    return *this;
  }

  void Capacitate(const int i) { heap_.Capacitate(i); acti_.Capacitate(i); }
  void Capacitate(const T t) { heap_.Capacitate(t); acti_.Capacitate(t); }

  int size() const { return heap_.size(); }

  T Top() const { return heap_.Top(); }
  void Insert(const T t) { heap_.Insert(t); }
  void Remove(const T t) { heap_.Remove(t); }
  bool Contains(const T t) const { return heap_.Contains(t); }

  typename std::vector<T>::const_iterator begin() const { return heap_.begin(); }
  typename std::vector<T>::const_iterator end()   const { return heap_.end(); }

  void BumpMax(const T t) { Bump(t, *std::max_element(acti_.begin(), acti_.end()) - acti_[t] + bump_step_); }
  void Bump(const T t) { Bump(t, bump_step_); }
  void Decay() { bump_step_ /= kDecayFactor; }

 private:
  struct ActivityCompare {
    explicit ActivityCompare(const internal::DenseMap<T, double>* a) : acti_(a) {}
    bool operator()(const T t1, const T t2) const { return (*acti_)[t1] > (*acti_)[t2]; }
   private:
    const internal::DenseMap<T, double>* acti_;
  };

  static constexpr double kActivityThreshold = 1e100;
  static constexpr double kDecayFactor = 0.95;

  void Bump(const T t, const double bump) {
    acti_[t] += bump;
    if (acti_[t] > kActivityThreshold) {
      for (double& activity : acti_) {
        activity /= kActivityThreshold;
      }
      bump_step_ /= kActivityThreshold;
    }
    if (heap_.Contains(t)) {
      heap_.Increase(t);
    }
  }

  double                             bump_step_;
  internal::DenseMap<T, double>      acti_;
  internal::Heap<T, ActivityCompare> heap_{ActivityCompare(&acti_)};
};

class Solver {
 public:
  using uref_t = int;
  using cref_t = Clause::Factory::cref_t;
  using level_t = int;

  Solver() = default;

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
      const cref_t cr = clause_factory_.New(as);
      Clause& c = clause_factory_[cr];
      if (c.valid()) {
        clause_factory_.Delete(cr, as.size());
        return;
      }
      if (c.unsat()) {
        empty_clause_ = true;
        clause_factory_.Delete(cr, as.size());
        return;
      }
      assert(c.size() >= 1);
      if (c.size() == 1) {
        trail_.push_back(c[0]);
        clause_factory_.Delete(cr, as.size());
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
    std::vector<Lit> lits = std::move(trail_);
    trail_.reserve(lits.size());
    for (const Lit a : lits) {
      if (falsifies(a)) {
        empty_clause_ = true;
        return;
      }
      Enqueue(a, kNullRef);
    }
  }

  void Reset() {
    if (current_level() != kRootLevel) {
      Backtrack(kRootLevel);
    }
  }

  void Simplify() {
    Reset();
    assert(level_size_.size() == 1);
    assert(level_size_[0] == 0);
    const cref_t conflict = Propagate();
    if (conflict != kNullRef) {
      empty_clause_ = true;
      return;
    }
    for (std::vector<cref_t>& ws : watchers_) {
      ws.clear();
    }
    int n_clauses = clauses_.size();
    for (int i = 1; i < n_clauses; ++i) {
      const cref_t cr = clauses_[i];
      Clause& c = clause_factory_[cr];
      assert(c.size() >= 2);
      const int removed = c.RemoveIf([this](Lit a) -> bool { return falsifies(a); });
      assert(!c.valid());
      if (c.unsat()) {
        empty_clause_ = true;
        clause_factory_.Delete(cr, c.size() + removed);
        return;
      } else if (satisfies(c)) {
        clause_factory_.Delete(cr, c.size() + removed);
        clauses_[i--] = clauses_[--n_clauses];
      } else if (c.size() == 1) {
        Enqueue(c[0], kNullRef);
        clause_factory_.Delete(cr, c.size() + removed);
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
        data_[f][n].Reset();
      }
      data_[f][n].reason = kNullRef;
      assert(satisfies(a));
    }
    trail_.resize(n_units);
    trail_head_ = trail_.size();
  }

  const std::vector<cref_t>&                        clauses() const { return clauses_; }
  const Clause&                                     clause(cref_t cr) const { return clause_factory_[cr]; }
  const internal::DenseSet<Fun>&                    funs() const { return funs_; }
  const internal::DenseMap<Fun, std::vector<Name>>& names() const { return names_; }
  const std::vector<Name>&                          names(Fun f) const { return names_[f]; }
  const internal::DenseMap<Fun, Name>&              model() const { return model_; }

  template<typename ConflictPredicate, typename DecisionPredicate>
  int Solve(ConflictPredicate conflict_predicate = ConflictPredicate(),
            DecisionPredicate decision_predicate = DecisionPredicate()) {
    if (empty_clause_) {
      return -1;
    }
    std::vector<Lit> learnt;
    bool go = true;
    while (go) {
      const cref_t conflict = Propagate();
      if (conflict != kNullRef) {
        if (current_level() == kRootLevel) {
          return -1;
        }
        level_t btlevel;
        Analyze(conflict, &learnt, &btlevel);
        go &= conflict_predicate(current_level(), conflict, learnt, btlevel);
        Backtrack(btlevel);
        if (learnt.size() == 1) {
          const Lit a = learnt[0];
          assert(!falsifies(a));
          Enqueue(a, kNullRef);
        } else {
          const cref_t cr = clause_factory_.New<Clause::kGuaranteeNormalized>(learnt);
          const Clause& c = clause(cr);
          assert(c.size() >= 2);
          assert(!satisfies(c));
          assert(!falsifies(c[0]));
          assert(std::all_of(c.begin() + 1, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
          clauses_.push_back(cr);
          UpdateWatchers(cr, c);
          Enqueue(c[0], cr);
        }
        learnt.clear();
        fun_order_.Decay();
      } else {
        Fun f;
        do {
          f = fun_order_.Top();
          if (f.null()) {
            return 1;
          }
          fun_order_.Remove(f);
        } while (!model_[f].null());
#ifdef NAME_ORDER
        const Name n = name_order_[f].Top();
#else
        const Name n = CandidateName(f);
#endif
        if (n.null()) {
          return -1;
        }
        NewLevel();
        const Lit a = Lit::Eq(f, n);
        Enqueue(a, kNullRef);
        go &= decision_predicate(current_level(), a);
      }
    }
    Backtrack(kRootLevel);
    return 0;
  }

 private:
  struct Data {  // meta data for a pair (f,n)
    Data() = default;

    void Update(const bool neq, const level_t l, const cref_t r) {
      model_neq = neq;
      level = l;
      reason = r;
    }

    void Reset() {
      assert(!seen_subsumed);
      assert(!wanted);
      assert(occurs);
      model_neq = false;
      level = 0;
      reason = kNullRef;
    }

    unsigned seen_subsumed :  1;  // auxiliary flag to keep track of seen trail literals
    unsigned wanted        :  1;  // auxiliary flag to keep track of seen trail literals
    unsigned occurs        :  1;  // true iff f occurs with n in added clauses or literals
    unsigned model_neq     :  1;  // true iff f != n was set or derived
    unsigned level         : 28;  // level at which f = n or f != n was set or derived
    cref_t reason;                // clause which derived f = n or f != n
  };

  static_assert(sizeof(Data) == 4 + sizeof(cref_t), "Data should be 4 + 4 bytes");

  struct ActivityCompare {
    explicit ActivityCompare(const internal::DenseMap<Fun, double>* a) : activity_(a) {}
    bool operator()(const Fun f1, const Fun f2) const { return (*activity_)[f1] > (*activity_)[f2]; }
   private:
    const internal::DenseMap<Fun, double>* activity_;
  };

  static constexpr cref_t kNullRef = 0;
  static constexpr cref_t kDomainRef = -1;
  static constexpr level_t kRootLevel = 1;

  void Register(const Fun f, const Name n, const Name extra_n) {
    CapacitateMaps(f, n, extra_n);
    if (!funs_.Contains(f)) {
      funs_.Insert(f);
      fun_order_.Insert(f);
      if (!data_[f][extra_n].occurs) {
        data_[f][extra_n].occurs = true;
        ++domain_size_[f];
        names_[f].push_back(extra_n);
#ifdef NAME_ORDER
        name_order_[f].Insert(extra_n);
#endif
      }
    }
    if (!data_[f][n].occurs) {
      data_[f][n].occurs = true;
      ++domain_size_[f];
      names_[f].push_back(n);
#ifdef NAME_ORDER
      name_order_[f].Insert(n);
#endif
    }
#ifdef NAME_ORDER
    assert(domain_size_[f] == name_order_[f].size());
#endif
  }

  void UpdateWatchers(const cref_t cr, const Clause& c) {
    assert(&clause(cr) == &c);
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

  cref_t Propagate() {
    cref_t conflict = kNullRef;
    while (trail_head_ < trail_.size() && conflict == kNullRef) {
      Lit a = trail_[trail_head_++];
      conflict = Propagate(a);
    }
    assert(std::all_of(clauses_.begin()+1, clauses_.end(), [this](cref_t cr) -> bool { return std::all_of(clause(cr).begin(), clause(cr).begin()+2, [this, cr](Lit a) -> bool { auto& ws = watchers_[a.fun()]; return std::count(ws.begin(), ws.end(), cr) >= 1; }); }));
    assert(conflict != kNullRef || std::all_of(clauses_.begin()+1, clauses_.end(), [this](cref_t cr) -> bool { const Clause& c = clause(cr); return satisfies(c) || (!falsifies(c[0]) && !falsifies(c[1])) || std::all_of(c.begin()+2, c.end(), [this](Lit a) -> bool { return falsifies(a); }); }));
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

  cref_t Propagate(const Lit a) {
    cref_t conflict = kNullRef;
    const Fun f = a.fun();
    std::vector<cref_t>& ws = watchers_[f];
    cref_t* const begin = ws.data();
    cref_t* const end = begin + ws.size();
    cref_t* cr_ptr1 = begin;
    cref_t* cr_ptr2 = begin;
    while (cr_ptr1 != end) {
      assert(begin == ws.data() && end == begin + ws.size());
      const cref_t cr = *cr_ptr1;
      Clause& c = clause_factory_[cr];
      const Fun f0 = c[0].fun();
      const Fun f1 = c[1].fun();

      // w is a two-bit number where the i-th bit indicates that c[i] is falsified
      // w >> 1 is falsified (c[1] if it is, else c[0])
      // 1 - (w >> 1) is the other literal
      char w = (static_cast<char>(f == f1 && falsifies(c[1])) << 1)
              | static_cast<char>(f == f0 && falsifies(c[0]));
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
            watchers_[fk].push_back(cr);
          }
          assert(std::count(watchers_[fk].begin(), watchers_[fk].end(), cr) >= 1);
          std::swap(c[i], c[k]);
          w = (w - 1) >> 1;  // 11 becomes 01, 10 becomes 00, 01 becomes 00
        }
      }

      assert(bool(w & 1) == (c[0].fun() == f && falsifies(c[0])));
      assert(bool(w & 2) == (c[1].fun() == f && falsifies(c[1])));

      // remove c if f is not watched anymore
      if (c[0].fun() != f && c[1].fun() != f) {
        ++cr_ptr1;
      }

      // conflict or propagate
      if (w != 0) {
        const int i = 1 - (w >> 1);  // 11 becomes 0, 10 becomes 0, 01 becomes 1
        if (w == 3 || falsifies(c[i])) {
          assert(std::all_of(c.begin(), c.end(), [this](Lit a) -> bool { return falsifies(a); }));
          std::memmove(cr_ptr2, cr_ptr1, (end - cr_ptr1) * sizeof(cref_t));
          cr_ptr2 += end - cr_ptr1;
          cr_ptr1 = end;
          trail_head_ = trail_.size();
          conflict = cr;
        } else {
          assert(std::all_of(c.begin(), c.end(), [this, &c, i](Lit a) -> bool { return (a != c[i]) == falsifies(a); }));
          Enqueue(c[i], cr);
        }
      }
    }
    ws.resize(cr_ptr2 - begin);
    return conflict;
  }

  void Analyze(cref_t conflict, std::vector<Lit>* const learnt, level_t* const btlevel) {
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) -> bool { return !d.seen_subsumed && !d.wanted; }); }));
    int depth = 0;
    Lit trail_a = Lit();
    uref_t trail_i = trail_.size() - 1;
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
      data_[f][n].seen_subsumed = 1;
    };
    auto seen_subsumed = [this](const Lit a) -> bool {
      assert(falsifies(a));
      assert(model_[a.fun()].null() || (a.pos() != (model_[a.fun()] == a.name())));
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      return data_[f][n].seen_subsumed || (p && !m.null() && data_[f][m].seen_subsumed);
    };

    // want_complementary_on_level(a,l) marks all literals on level l that
    // are complementary to a as wanted.
    // By the following reasoning, it suffices to mark only a single literal
    // that implicitly also determines the others as wanted.
    // When we want a complementary literal to f == n, we prefer f != n over
    // f == model_[f] because this will become f == n in the conflict clause.
    // This also means that we want exactly one literal, which eliminates the
    // need for traversing the whole level again to reset the wanted flag.
    auto want_complementary_on_level = [this](const Lit a, level_t l) -> void {
      assert(falsifies(a));
      assert(data_[a.fun()][a.name()].level <= l);
      assert(a.pos() || data_[a.fun()][a.name()].level == l);
      assert(a.pos() || model_[a.fun()] == a.name());
      assert(!a.pos() || data_[a.fun()][a.name()].level != l || data_[a.fun()][a.name()].model_neq);
      assert(!a.pos() || data_[a.fun()][a.name()].level == l || !model_[a.fun()].null());
      assert(!a.pos() || data_[a.fun()][a.name()].level == l || data_[a.fun()][model_[a.fun()]].level == l);
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      data_[f][data_[f][n].level == l ? n : m].wanted = true;
    };
    // wanted_complementary_on_level(a,l) iff a on level l is wanted.
    auto wanted_complementary_on_level = [this](const Lit a, level_t l) -> bool {
      assert(falsifies(a));
      assert(data_[a.fun()][a.name()].level <= l);
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      return (!p && data_[f][n].wanted) ||
             (p && ((data_[f][n].level == l && data_[f][n].wanted) || (!m.null() && data_[f][m].wanted)));
    };
    // We un-want every trail literal after it has been traversed.
    auto wanted = [this](const Lit a) -> bool {
      assert(satisfies(a));
      const Fun f = a.fun();
      const Name n = a.name();
      return data_[f][n].wanted;
    };

    auto handle_conflict = [this, &trail_a, &learnt, &depth, &see_subsuming, &seen_subsumed,
                            &want_complementary_on_level, &wanted_complementary_on_level](const Lit a) -> void {
      if (trail_a == a) {
        return;
      }
      assert(falsifies(a));
      assert(!satisfies(a));
      const level_t l = level_of_complementary(a);
      if (l == kRootLevel || seen_subsumed(a) || wanted_complementary_on_level(a, l)) {
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
      fun_order_.Bump(a.fun());
#ifdef NAME_ORDER
      name_order_[a.fun()].Bump(a.name());
#endif
    };

    do {
      assert(conflict != kNullRef);
      if (conflict == kDomainRef) {
        assert(!trail_a.null());
        assert(trail_a.pos());
        const Fun f = trail_a.fun();
        for (const Name n : names_[f]) {
          assert(!n.null() && data_[f][n].occurs);
          const Lit a = Lit::Eq(f, n);
          handle_conflict(a);
        }
      } else {
        for (const Lit a : clause(conflict)) {
          handle_conflict(a);
        }
      }
      assert(depth > 0);

      while (!wanted(trail_[trail_i--])) {
        assert(trail_i >= 0);
      }
      trail_a = trail_[trail_i + 1];
      data_[trail_a.fun()][trail_a.name()].wanted = false;
      --depth;
      conflict = reason_of(trail_a);
    } while (depth > 0);
    (*learnt)[0] = trail_a.flip();

    for (const Lit a : *learnt) {
      data_[a.fun()][a.name()].seen_subsumed = false;
    }

    learnt->resize(Clause::Normalize<Clause::kGuaranteeInvalid>(learnt->size(), learnt->data()));

    if (learnt->size() == 1) {
      *btlevel = kRootLevel;
    } else {
      assert(learnt->size() >= 2);
      uref_t max = 1;
      *btlevel = level_of_complementary((*learnt)[max]);
      for (uref_t i = 2, s = learnt->size(); i != s; ++i) {
        level_t l = level_of_complementary((*learnt)[i]);
        if (*btlevel < l) {
          max = i;
          *btlevel = l;
        }
      }
      std::swap((*learnt)[1], (*learnt)[max]);
    }
    assert(level_of(trail_a) > *btlevel && *btlevel >= kRootLevel);
    assert(std::all_of(learnt->begin(), learnt->end(), [this](Lit a) -> bool { return falsifies(a); }));
    assert(std::all_of(learnt->begin(), learnt->end(), [this](Lit a) -> bool { return !satisfies(a); }));
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const Data& d) -> bool { return !d.seen_subsumed; }); }));
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const Data& d) -> bool { return !d.wanted; }); }));
  }

  void NewLevel() { level_size_.push_back(trail_.size()); }

  void Enqueue(const Lit a, const cref_t reason) {
    assert(data_[a.fun()][a.name()].occurs);
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    if (m.null() && (p || !data_[f][n].model_neq)) {
      assert(domain_size_[a.fun()] >= 1 + !a.pos());
      assert(!satisfies(a));
      trail_.push_back(a);
      data_[f][n].Update(!p, current_level(), reason);
      if (p) {
        model_[f] = n;
      } else if (--domain_size_[f] == 1) {
#ifdef NAME_ORDER
        name_order_[f].Remove(n);
        const Name m = name_order_[f].Top();
#else
        const Name m = CandidateName(f);
#endif
        assert(!satisfies(Lit::Eq(f, m)) && !falsifies(Lit::Eq(f, m)));
        trail_.push_back(Lit::Eq(f, m));
        data_[f][m].Update(false, current_level(), kDomainRef);
        model_[f] = m;
        assert(satisfies(Lit::Eq(f, m)));
      } else {
        fun_order_.BumpMax(f);
#ifdef NAME_ORDER
        name_order_[f].Remove(n);
#endif
      }
    }
    assert(satisfies(a));
#ifdef NAME_ORDER
    assert(domain_size_[f] == name_order_[f].size());
#endif
  }

  void Backtrack(const level_t l) {
    for (auto a = trail_.cbegin() + level_size_[l], e = trail_.cend(); a != e; ++a) {
      const bool p = a->pos();
      const Fun f = a->fun();
      const Name n = a->name();
      model_[f] = Name();
      if (p) {
        if (!data_[f][n].model_neq) {
          data_[f][n].Reset();
        }
        if (!fun_order_.Contains(f)) {
          fun_order_.Insert(f);
        }
      } else {
        data_[f][n].Reset();
        domain_size_[f]++;
#ifdef NAME_ORDER
        name_order_[f].Insert(n);
#endif
      }
#ifdef NAME_ORDER
      assert(domain_size_[f] == name_order_[f].size());
#endif
    }
    trail_.resize(level_size_[l]);
    trail_head_ = trail_.size();
    level_size_.resize(l);
  }

#ifndef NAME_ORDER
  Name CandidateName(const Fun f) {
    assert(!f.null() && model_[f].null());
    const std::vector<Name>& names = names_[f];
    const int size = names.size();
#ifdef PHASING
    const int offset = name_index_[f];
    for (int i = offset; i >= 0; --i) {
      const Name n = names[i];
      assert(!n.null() && data_[f][n].occurs);
      if (!data_[f][n].model_neq) {
        name_index_[f] = i;
        return n;
      }
    }
    for (int i = size - 1; i > offset; --i) {
      const Name n = names[i];
      assert(!n.null() && data_[f][n].occurs);
      if (!data_[f][n].model_neq) {
        name_index_[f] = i;
        return n;
      }
    }
#else
    for (int i = size - 1; i >= 0; --i) {
      const Name n = names[i];
      if (!n.null() && data_[f][n].occurs && !data_[f][n].model_neq) {
        return n;
      }
    }
#endif
    return Name();
  }
#endif

  bool satisfies(const Lit a, const level_t l = -1) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((p && m == n) ||
            (!p && ((!m.null() && m != n) || data_[f][n].model_neq))) &&
           (l < 0 || data_[f][n].level <= l);
  }

  bool falsifies(const Lit a, const level_t l = -1) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((!p && m == n) ||
            (p && ((!m.null() && m != n) || data_[f][n].model_neq))) &&
           (l < 0 || data_[f][n].level <= l);
  }

  bool satisfies(const Clause& c, const level_t l = -1) const {
    return std::any_of(c.begin(), c.end(), [this, l](Lit a) -> bool { return satisfies(a, l); });
  }

  bool falsifies(const Clause& c, const level_t l = -1) const {
    return std::all_of(c.begin(), c.end(), [this, l](Lit a) -> bool { return falsifies(a, l); });
  }

  level_t level_of(const Lit a) const {
    assert(satisfies(a));
    assert(!a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return !p && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level;
  }

  level_t level_of_complementary(const Lit a) const {
    assert(falsifies(a));
    assert(a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return p && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level;
  }

  cref_t reason_of(const Lit a) const {
    assert(satisfies(a));
    assert(!a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return !p && data_[f][n].model_neq ? data_[f][n].reason : data_[f][m].reason;
  }

  level_t current_level() const { return level_size_.size(); }

  void CapacitateMaps(const Fun f, const Name n, const Name extra_n) {
    const int max_ni = int(n) > int(extra_n) ? int(n) : int(extra_n);
    const int fi = int(f) >= funs_.upper_bound() ? int(f) : -1;
    const int ni = data_.upper_bound() == 0 || max_ni >= data_[0].upper_bound() ? max_ni : -1;
    const int fig = (fi + 1) * 1.5;
    const int nig = ni >= 0 ? (ni + 1) * 3 / 2 : data_[0].upper_bound();
    if (fi >= 0) {
      funs_.Capacitate(fig);
      names_.Capacitate(fig);
#ifdef PHASING
      name_index_.Capacitate(fig);
#endif
      watchers_.Capacitate(fig);
      model_.Capacitate(fig);
      data_.Capacitate(fig);
      domain_size_.Capacitate(fig);
      fun_order_.Capacitate(fig);
#ifdef NAME_ORDER
      name_order_.Capacitate(fig);
#endif
    }
    if (fi >= 0 || ni >= 0) {
      for (internal::DenseMap<Name, Data>& ds : data_) {
        ds.Capacitate(nig);
      }
#ifdef NAME_ORDER
      for (ActivityOrder<Name>& ao : name_order_) {
        ao.Capacitate(nig);
      }
#endif
    }
    if (ni >= 0) {
      for (internal::DenseMap<Name, Data>& ds : data_) {
        ds.Capacitate(nig);
      }
    }
  }

  // empty_clause_ is true iff the empty clause has been derived.
  bool empty_clause_ = false;

  // clauses_ is the sequence of clauses added initially or learnt.
  Clause::Factory clause_factory_;
  std::vector<cref_t> clauses_ = std::vector<cref_t>(1, 0);

  // funs_ is the set of functions that occur in clauses.
  // names_ is the set of names that occur in clauses plus extra names.
  internal::DenseSet<Fun>                    funs_;
  internal::DenseMap<Fun, std::vector<Name>> names_;
#ifdef PHASING
  internal::DenseMap<Name, int>              name_index_;
#endif

  // watchers_ maps every function to a sequence of clauses that watch it.
  // Every clause watches two functions, and when a literal with this function
  // is propagated, the watching clauses are inspected.
  internal::DenseMap<Fun, std::vector<cref_t>> watchers_;

  // trail_ is a sequence of literals in the order they were derived.
  // level_size_ groups the literals of trail_ into chunks by their level at
  // which they were derived, where level_size_[l] determines the number of
  // literals set or derived up to level l.
  // trail_head_ is the index of the first literal of trail_ that hasn't been
  // propagated yet.
  std::vector<Lit>     trail_;
  std::vector<uref_t>  level_size_ = std::vector<uref_t>(1, trail_.size());
  uref_t               trail_head_ = 0;

  // model_ is an assignment of functions to names, i.e., positive literals.
  // data_ is meta data for every function and name pair (cf. Data).
  // domain_size_ is the number of candidates for every function.
  internal::DenseMap<Fun, Name>                           model_;
  internal::DenseMap<Fun, internal::DenseMap<Name, Data>> data_;
  internal::DenseMap<Fun, int>                            domain_size_;

  // fun_order_ ranks functions by their activity.
  // name_order_ ranks functions by their activity.
  ActivityOrder<Fun>                           fun_order_;
#ifdef NAME_ORDER
  internal::DenseMap<Fun, ActivityOrder<Name>> name_order_;
#endif
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

