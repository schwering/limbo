// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_KB_H_
#define EXAMPLES_SUDOKU_KB_H_

#include <sstream>
#include <vector>

#include <limbo/solver.h>
#include <limbo/internal/maybe.h>
#include <limbo/internal/iter.h>
#include <limbo/format/output.h>

#include "game.h"
#include "timer.h"

class KnowledgeBase {
 public:
  KnowledgeBase(const Game* g, int max_k)
      : max_k_(max_k),
        solver_(limbo::Symbol::Factory::Instance(), limbo::Term::Factory::Instance()),
        VAL_(CreateNonrigidSort()),
        val_(CreateFunctionSymbol(VAL_, 2)) {
    limbo::format::RegisterSort(VAL_, "");
    limbo::format::RegisterSymbol(val_, "val");
    for (std::size_t i = 1; i <= 9; ++i) {
      vals_.push_back(CreateName(VAL_));
      std::stringstream ss;
      ss << i;
      limbo::format::RegisterSymbol(vals_.back().symbol(), ss.str());
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        for (std::size_t yy = 1; yy <= 9; ++yy) {
          if (y != yy) {
            Add(limbo::Literal::Neq(val(x, y), val(x, yy)));
          }
        }
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t xx = 1; xx <= 9; ++xx) {
        for (std::size_t y = 1; y <= 9; ++y) {
          if (x != xx) {
            Add(limbo::Literal::Neq(val(x, y), val(xx, y)));
          }
        }
      }
    }
    for (std::size_t i = 1; i <= 3; ++i) {
      for (std::size_t j = 1; j <= 3; ++j) {
        for (std::size_t x = 3*i-2; x <= 3*i; ++x) {
          for (std::size_t xx = 3*i-2; xx <= 3*i; ++xx) {
            for (std::size_t y = 3*j-2; y <= 3*j; ++y) {
              for (std::size_t yy = 3*j-2; yy <= 3*j; ++yy) {
                if (x != xx || y != yy) {
                  Add(limbo::Literal::Neq(val(x, y), val(xx, yy)));
                }
              }
            }
          }
        }
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        std::vector<limbo::Literal> lits;
        for (std::size_t i = 1; i <= 9; ++i) {
          lits.push_back(limbo::Literal::Eq(val(x, y), n(i)));
        }
        Add(limbo::Clause(lits.begin(), lits.end()));
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        int i = g->get(Point(x, y));
        if (i != 0) {
          Add(limbo::Literal::Eq(val(x, y), n(i)));
        }
      }
    }
  }

  int max_k() const { return max_k_; }

  limbo::Solver& solver() { UpdateSolver(); return solver_; }
  const limbo::Solver& solver() const { const_cast<KnowledgeBase&>(*this).UpdateSolver(); return solver_; }
  const limbo::Setup& setup() const { const_cast<KnowledgeBase&>(*this).UpdateSolver(); return solver().setup(); }

  void Add(Point p, int i) { Add(limbo::Clause{limbo::Literal::Eq(val(p), n(i))}); }

  limbo::internal::Maybe<int> Val(Point p, int k) {
    t_.start();
    UpdateSolver();
    const limbo::internal::Maybe<limbo::Term> r = solver().Determines(k, val(p), limbo::Solver::kConsistencyGuarantee);
    assert(std::all_of(limbo::internal::int_iterator<size_t>(1), limbo::internal::int_iterator<size_t>(9),
           [&](size_t i) {
             limbo::Formula::Ref f = limbo::Formula::Factory::Atomic(limbo::Clause{limbo::Literal::Eq(val(p), n(i))});
             return solver().Entails(k, *f) == (r && r.val == n(i));
           }));
    if (r) {
      assert(!r.val.null());
      for (std::size_t i = 1; i <= 9; ++i) {
        if (r.val.null() || r.val == n(i)) {
          return limbo::internal::Just(i);
        }
      }
    }
    t_.stop();
    return limbo::internal::Nothing;
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

 private:
  void Add(const limbo::Literal a) { Add(limbo::Clause{a}); }
  void Add(const limbo::Clause& c) { clauses_.push_back(c); }

  void UpdateSolver() {
    if (n_processed_clauses_ == clauses_.size()) {
      return;
    }
    for (auto it = clauses_.begin() + n_processed_clauses_; it != clauses_.end(); ++it) {
      limbo::Formula::FlattenClause(&*it, nullptr,
                                    limbo::Symbol::Factory::Instance(),
                                    limbo::Term::Factory::Instance());
    }
    solver_.grounder().AddClauses(clauses_.begin() + n_processed_clauses_, clauses_.end());
    solver_.grounder().Consolidate();
    n_processed_clauses_ = clauses_.size();
  }

  limbo::Term n(std::size_t n) const {
    return vals_[n-1];
  }

  limbo::Term val(Point p) const {
    return val(p.x, p.y);
  }

  limbo::Term val(std::size_t x, std::size_t y) const {
    return CreateFunction(val_, limbo::Term::Vector{vals_[x-1], vals_[y-1]});
  }

  limbo::Symbol::Sort CreateNonrigidSort() const {
    return limbo::Symbol::Factory::Instance()->CreateNonrigidSort();
  }

  limbo::Symbol CreateFunctionSymbol(limbo::Symbol::Sort sort, limbo::Symbol::Arity arity) const {
    return limbo::Symbol::Factory::Instance()->CreateFunction(sort, arity);
  }

  limbo::Term CreateName(limbo::Symbol::Sort sort) const {
    return limbo::Term::Factory::Instance()->CreateTerm(limbo::Symbol::Factory::Instance()->CreateName(sort));
  }

  limbo::Term CreateVariable(limbo::Symbol::Sort sort) const {
    return limbo::Term::Factory::Instance()->CreateTerm(limbo::Symbol::Factory::Instance()->CreateVariable(sort));
  }

  limbo::Term CreateFunction(limbo::Symbol symbol, const limbo::Term::Vector& args) const {
    return limbo::Term::Factory::Instance()->CreateTerm(symbol, args);
  }

  int max_k_;

  std::vector<limbo::Clause> clauses_;
  size_t n_processed_clauses_ = 0;

  limbo::Solver solver_;

  limbo::Symbol::Sort VAL_;
  limbo::Symbol val_;
  limbo::Term::Vector vals_;

  Timer t_;
};

#endif  // EXAMPLES_SUDOKU_KB_H_

