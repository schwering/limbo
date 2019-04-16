// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Satisfiability solver for grounded clauses with functions and names.
// The life-cycle works as follows:
//
//  1. AddLiteral() and/or AddClause() to populate the knowledge base.
//  2. Init() to prepare for solving.
//  3. Simplify() [opt.] to eliminate unsatisfied clauses etc.
//  4. Solve() to find an assignment.
//  5. value() and size() to access model.
//  6. Reset() (optional) to reset the current assignment.
//  7. AddLiteral() or AddClause() to extend knowledge base.
//  8. Simplify() [opt.] to reset the assignment (!) and simplify the clauses.
//  9. Solve() to find an assignment.
// 10. value() and size() to access model.
// 11. Repeat from step 6.

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
  enum class Truth : char { kUnsat = -1, kUnknown = 0, kSat = 1 };
  enum class Level : int  { kNull = 0, kRoot = 1 };
  using CRef = Clause::Factory::CRef;

  explicit Sat() = default;

  Sat(const Sat&)            = delete;
  Sat& operator=(const Sat&) = delete;
  Sat(Sat&&)                 = default;
  Sat& operator=(Sat&&)      = default;

  template<typename ExtraNameFactory>
  void AddLiteral(const Lit a, ExtraNameFactory extra_name = ExtraNameFactory()) {
    Register(a.fun(), a.name(), extra_name(a.fun()));
    if (falsifies(a)) {
      empty_clause_ = true;
    } else {
      Enqueue(a, CRef::kNull);
    }
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
        data_[f][n].Reset();
      }
      data_[f][n].reason = CRef::kNull;
      assert(satisfies(a));
    }
    trail_.resize(n_units);
    trail_head_ = trail_.size();
  }

  const std::vector<CRef>& clauses()       const { return clauses_; }
  const Clause&            clause(CRef cr) const { return clausef_[cr]; }

  int  size()       const { return trail_eqs_; }
  Name value(Fun f) const { return model_[f]; }

  bool     propagate_with_learnt()       const { return propagate_with_learned_; }
  void set_propagate_with_learnt(bool b)       { propagate_with_learned_ = b; }

  template<typename ConflictPredicate, typename DecisionPredicate>
  Truth Solve(ConflictPredicate conflict_predicate = ConflictPredicate(),
              DecisionPredicate decision_predicate = DecisionPredicate()) {
    if (empty_clause_) {
      return Truth::kUnsat;
    }
    std::vector<Lit> learnt;
    bool go = true;
    while (go) {
      const CRef conflict = Propagate();
      if (conflict != CRef::kNull) {
        if (current_level() == Level::kRoot) {
          return Truth::kUnsat;
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
          assert(!satisfies(c[0]) && !falsifies(c[0]));
          assert(std::all_of(c.begin() + 1, c.end(), [this](Lit a) -> bool { return falsifies(a); }));
          clauses_.push_back(cr);
          UpdateWatchers(cr, c);
          Enqueue(learnt[0], cr);
        } else {
          Enqueue(learnt[0], CRef::kNull);
        }
        learnt.clear();
        DecayFun();
      } else {
        Fun f = NextFun();
        if (f.null()) {
          return Truth::kSat;
        }
        Name n = NextName(f);
        if (n.null()) {
          return Truth::kUnsat;
        }
        AddNewLevel();
        const Lit a = Lit::Eq(f, n);
        EnqueueEq(a, CRef::kNull);
        go &= decision_predicate(current_level(), a);
      }
      assert(level_size_.back() < trail_.size());
    }
    Backtrack(Level::kRoot);
    return Truth::kUnknown;
  }

 private:
  struct FunNameData {
    static constexpr bool kModelNeq = true;
    static constexpr bool kModelEq  = false;

    explicit FunNameData()
        : occurs(0), popped(0), model_neq(0), seen_subsumed(0), wanted(0), level(0), reason(CRef::kNull) {}

    FunNameData(const FunNameData&)            = delete;
    FunNameData& operator=(const FunNameData&) = delete;
    FunNameData(FunNameData&&)                 = default;
    FunNameData& operator=(FunNameData&&)      = default;

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

    unsigned occurs        :  1;  // true iff f occurs with n in added clauses or literals
    unsigned popped        :  1;  // true iff n has been popped from domain_[f]
    unsigned model_neq     :  1;  // true iff f != n was set or derived
    unsigned seen_subsumed :  1;  // true iff a literal subsumed by f = n / f != n on the trail (helper for Analyze())
    unsigned wanted        :  1;  // true iff a literal complementary to f = n / f != n is wanted (helper for Analyze())
    unsigned level         : 27;  // level at which f = n or f != n was set or derived
    CRef reason;                  // clause which derived f = n or f != n
  };

  struct ActivityCompare {
    explicit ActivityCompare(const internal::DenseMap<Fun, double>* activity) : activity_(activity) {}
    bool operator()(const Fun f1, const Fun f2) const { return (*activity_)[f1] > (*activity_)[f2]; }
   private:
    const internal::DenseMap<Fun, double>* activity_;
  };


  static_assert(sizeof(FunNameData) == 4 + sizeof(CRef), "FunNameData should be 4 + 4 bytes");

  static constexpr double kBumpStepInit = 1.0;
  static constexpr double kActivityThreshold = 1e100;
  static constexpr double kDecayFactor = 0.95;


  void Bump(Clause& c) {
    c.set_activity(c.activity() + clause_bump_step_);
    if (c.activity() > kActivityThreshold) {
      for (const CRef cr : clauses_) {
        Clause& c = clausef_[cr];
        c.set_activity(c.activity() / kActivityThreshold);
      }
      clause_bump_step_ /= kActivityThreshold;
    }
  }

  void Bump(const Fun f, const double bump) {
    fun_activity_[f] += bump;
    if (fun_activity_[f] > kActivityThreshold) {
      for (double& a : fun_activity_) {
        a /= kActivityThreshold;
      }
      fun_bump_step_ /= kActivityThreshold;
    }
    if (fun_queue_.Contains(f)) {
      if (bump >= 0) {
        fun_queue_.Increase(f);
      } else {
        fun_queue_.Decrease(f);
      }
    }
  }

  void BumpToFront(const Fun f) { Bump(f, fun_activity_[fun_queue_.top()] - fun_activity_[f] + fun_bump_step_); }
  void Bump(const Fun f)        { Bump(f, fun_bump_step_); }

  void DecayClause() { clause_bump_step_ /= kDecayFactor; }
  void DecayFun()    { fun_bump_step_    /= kDecayFactor; }

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
        Bump(c);
      }
    }
    ws.resize(cr_ptr2 - begin);
    DecayClause();
    return conflict;
  }

  void Analyze(CRef conflict, std::vector<Lit>* const learnt, Level* const btlevel) {
    assert(std::all_of(data_.begin(), data_.end(),
                       [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(),
                       [](const FunNameData& d) -> bool { return !d.seen_subsumed && !d.wanted; }); }));
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
    auto want_complementary_on_level = [this](const Lit a, Level l) -> void {
      assert(falsifies(a));
      assert(Level(data_[a.fun()][a.name()].level) <= l);
      assert(a.pos() || Level(data_[a.fun()][a.name()].level) == l);
      assert(a.pos() || model_[a.fun()] == a.name());
      assert(!a.pos() || Level(data_[a.fun()][a.name()].level) != l || data_[a.fun()][a.name()].model_neq);
      assert(!a.pos() || Level(data_[a.fun()][a.name()].level) == l || !model_[a.fun()].null());
      assert(!a.pos() || Level(data_[a.fun()][a.name()].level) == l || Level(data_[a.fun()][model_[a.fun()]].level) == l);
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      data_[f][Level(data_[f][n].level) == l ? n : m].wanted = true;
    };
    // wanted_complementary_on_level(a,l) iff a on level l is wanted.
    auto wanted_complementary_on_level = [this](const Lit a, Level l) -> bool {
      assert(falsifies(a));
      assert(Level(data_[a.fun()][a.name()].level) <= l);
      const bool p = a.pos();
      const Fun f = a.fun();
      const Name n = a.name();
      const Name m = model_[f];
      return (!p && data_[f][n].wanted) ||
             (p && ((Level(data_[f][n].level) == l && data_[f][n].wanted) ||
                    (!m.null() && data_[f][m].wanted)));
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
      Bump(a.fun());
    };

    do {
      assert(conflict != CRef::kNull);
      if (conflict == CRef::kDomain) {
        assert(!trail_a.null());
        assert(trail_a.pos());
        const Fun f = trail_a.fun();
        for (int i = 1; i < data_[f].upper_bound(); ++i) {
          const Name n = Name::FromId(i);
          if (data_[f][n].occurs) {
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
      data_[trail_a.fun()][trail_a.name()].wanted = false;
      --depth;
      conflict = reason_of(trail_a);
    } while (depth > 0);
    (*learnt)[0] = trail_a.flip();

    for (const Lit a : *learnt) {
      data_[a.fun()][a.name()].seen_subsumed = false;
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
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const FunNameData& d) -> bool { return !d.seen_subsumed; }); }));
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) -> bool { return std::all_of(ds.begin(), ds.end(), [](const FunNameData& d) -> bool { return !d.wanted; }); }));
  }

  void AddNewLevel() { level_size_.push_back(trail_.size()); }

  void Enqueue(const Lit a, const CRef reason) {
    assert(data_[a.fun()][a.name()].occurs);
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
      assert(domain_[a.fun()].size() >= !a.pos());
      trail_.push_back(a);
      model_[f] = n;
      data_[f][n].Update(FunNameData::kModelEq, current_level(), reason);
      ++trail_eqs_;
    }
    assert(satisfies(a));
  }

  void EnqueueNeq(const Lit a, const CRef reason) {
    assert(!a.pos());
    assert(!falsifies(a));
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    if (m.null() && !data_[f][n].model_neq) {
      assert(!data_[a.fun()][a.name()].popped);
      assert(domain_[a.fun()].size() >= !a.pos());
      assert(!satisfies(a));
      trail_.push_back(a);
      data_[f][n].Update(FunNameData::kModelNeq, current_level(), reason);
      ++trail_neqs_[f];
      if (domain_size_[f] - trail_neqs_[f] == 1) {
        const Name m = NextName(f);
        assert(!satisfies(Lit::Eq(f, m)) && !falsifies(Lit::Eq(f, m)));
        EnqueueEq(Lit::Eq(f, m), CRef::kDomain);
      } else {
        BumpToFront(f);
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
        --trail_eqs_;
        if (!fun_queue_.Contains(f)) {
          fun_queue_.Insert(f);
        }
      } else {
        --trail_neqs_[f];
      }
      if (data_[f][n].popped) {
        domain_[f].PushBack(n);
        data_[f][n].popped = false;
      }
      data_[f][n].Reset();
      assert(!satisfies(*a) && !falsifies(*a));
      assert(domain_size_[f] - trail_neqs_[f] <= domain_[f].size());
    }
    trail_.resize(level_size_[int(l)]);
    trail_head_ = trail_.size();
    level_size_.resize(int(l));
  }

  Fun NextFun() {
    Fun f;
    do {
      f = fun_queue_.top();
      if (f.null()) {
        break;
      }
      fun_queue_.Remove(f);
    } while (!model_[f].null());
    return f;
  }

  Name NextName(const Fun f) {
    Name n = Name();
    do {
      if (domain_[f].empty()) {
        break;
      }
      n = domain_[f].PopFront();
      data_[f][n].popped = true;
    } while (data_[f][n].model_neq);
    return n;
  }

  bool satisfies(const Lit a) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((p && m == n) ||
            (!p && ((!m.null() && m != n) || data_[f][n].model_neq)));
  }

  bool falsifies(const Lit a) const {
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return ((!p && m == n) ||
            (p && ((!m.null() && m != n) || data_[f][n].model_neq)));
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
    return Level(!p && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level);
  }

  Level level_of_complementary(const Lit a) const {
    assert(falsifies(a));
    assert(a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return Level(p && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level);
  }

  CRef reason_of(const Lit a) const {
    assert(satisfies(a));
    assert(!a.pos() || model_[a.fun()] == a.name());
    const bool p = a.pos();
    const Fun f = a.fun();
    const Name n = a.name();
    const Name m = model_[f];
    return !p && data_[f][n].model_neq ? data_[f][n].reason : data_[f][m].reason;
  }

  Level current_level() const { return Level(level_size_.size()); }

  void Register(const Fun f, const Name n, const Name extra_n) {
    CapacitateMaps(f, n, extra_n);
    if (!fun_queue_.Contains(f)) {
      fun_queue_.Insert(f);
      if (!data_[f][extra_n].occurs) {
        data_[f][extra_n].occurs = true;
        domain_[f].PushBack(extra_n);
        ++domain_size_[f];
      }
    }
    if (!data_[f][n].occurs) {
      data_[f][n].occurs = true;
      domain_[f].PushBack(n);
      ++domain_size_[f];
    }
  }

  void CapacitateMaps(const Fun f, const Name n, const Name extra_n) {
    const int max_ni = int(n) > int(extra_n) ? int(n) : int(extra_n);
    const int fi = int(f) >= data_.upper_bound() ? int(f) : -1;
    const int ni = data_.upper_bound() == 0 || max_ni >= data_[0].upper_bound() ? max_ni : -1;
    const int fig = (fi + 1) * 1.5;
    const int nig = ni >= 0 ? (ni + 1) * 3 / 2 : data_[0].upper_bound();
    if (fi >= 0) {
      fun_queue_.Capacitate(fig);
      fun_activity_.Capacitate(fig);
      domain_.Capacitate(fig);
      domain_size_.Capacitate(fig);
      watchers_.Capacitate(fig);
      model_.Capacitate(fig);
      data_.Capacitate(fig);
      trail_neqs_.Capacitate(fig);
    }
    if (fi >= 0 || ni >= 0) {
      for (internal::DenseMap<Name, FunNameData>& ds : data_) {
        ds.Capacitate(nig);
      }
    }
    if (ni >= 0) {
      for (internal::DenseMap<Name, FunNameData>& ds : data_) {
        ds.Capacitate(nig);
      }
    }
  }


  // empty_clause_ is true iff the empty clause has been derived.
  bool empty_clause_ = false;

  // clauses_ is the sequence of clauses added initially or learnt.
  // propagate_with_learnt_ is true iff learnt clauses are not considered in
  //    Propagate().
  Clause::Factory   clausef_;
  std::vector<CRef> clauses_                = std::vector<CRef>(1, CRef::kNull);
  bool              propagate_with_learned_ = true;
  double            clause_bump_step_       = kBumpStepInit;

  // domain_size_ is the number of names that occured per function.
  // domain_ contains at most domain_size_ names n that are potential values
  //    for each f; some of those may already be excluded by f != n on the
  //    trail.
  internal::DenseMap<Fun, int>              domain_size_;
  internal::DenseMap<Fun, RingBuffer<Name>> domain_;

  // watchers_ maps every function to a sequence of clauses that watch it;
  //    every clause watches two functions, and when a literal with this
  //    function is propagated, the watching clauses are inspected.
  internal::DenseMap<Fun, std::vector<CRef>> watchers_;

  // trail_ is a sequence of literals in the order they were derived.
  // level_size_ groups the literals of trail_ into chunks by their level at
  //    which they were derived, where level_size_[l] determines the number of
  //    literals set or derived up to level l.
  // trail_head_ is the index of the first literal of trail_ that hasn't been
  //    propagated yet.
  // trail_eqs_ is the number of equalities on the trail.
  // trail_neqs_ is the number of inequalities on the trail for a function.
  std::vector<Lit>             trail_;
  std::vector<int>             level_size_ = std::vector<int>(1, trail_.size());
  int                          trail_head_ = 0;
  int                          trail_eqs_  = 0;
  internal::DenseMap<Fun, int> trail_neqs_;

  // model_ is an assignment of functions to names, i.e., positive literals.
  // data_ is meta data for every function and name pair (cf. FunNameData).
  internal::DenseMap<Fun, Name>                                  model_;
  internal::DenseMap<Fun, internal::DenseMap<Name, FunNameData>> data_;

  // fun_activity_ assigns an activity to each function.
  // fun_queue_ ranks the clauses by activity (highest first).
  internal::DenseMap<Fun, double>         fun_activity_;
  internal::MinHeap<Fun, ActivityCompare> fun_queue_{ActivityCompare(&fun_activity_)};
  double                                  fun_bump_step_ = kBumpStepInit;
};

}  // namespace limbo

#endif  // LIMBO_SAT_H_

