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
#include <limbo/format/cpp/syntax.h>

#include "game.h"
#include "timer.h"

class KnowledgeBase {
 public:
  KnowledgeBase(const Game* g, int max_k)
      : max_k_(max_k),
        VAL_(ctx_.CreateSort()),
        val_(ctx_.CreateFunction(VAL_, 2)) {
    limbo::format::RegisterSort(VAL_, "");
    limbo::format::RegisterSymbol(val_, "val");
    using namespace limbo::format::cpp;
    for (std::size_t i = 1; i <= 9; ++i) {
      vals_.push_back(ctx_.CreateName(VAL_));
      std::stringstream ss;
      ss << i;
      limbo::format::RegisterSymbol(vals_.back().symbol(), ss.str());
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        for (std::size_t yy = 1; yy <= 9; ++yy) {
          if (y != yy) {
            ctx_.AddClause(val(x, y) != val(x, yy));
          }
        }
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t xx = 1; xx <= 9; ++xx) {
        for (std::size_t y = 1; y <= 9; ++y) {
          if (x != xx) {
            ctx_.AddClause(val(x, y) != val(xx, y));
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
                  ctx_.AddClause(val(x, y) != val(xx, yy));
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
        ctx_.AddClause(limbo::Clause(lits.begin(), lits.end()));
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        int i = g->get(Point(x, y));
        if (i != 0) {
          ctx_.AddClause(val(x, y) == n(i));
        }
      }
    }
  }

  int max_k() const { return max_k_; }

  limbo::Solver* solver() { return ctx_.solver(); }
  const limbo::Solver& solver() const { return ctx_.solver(); }
  const limbo::Setup& setup() const { return solver().setup(); }

  void Add(Point p, int i) {
    ctx_.AddClause(val(p) == n(i));
  }

  limbo::internal::Maybe<int> Val(Point p, int k) {
    t_.start();
    const limbo::internal::Maybe<limbo::Term> r = solver()->Determines(k, val(p), limbo::Solver::kConsistencyGuarantee);
    assert(std::all_of(limbo::internal::int_iterator<size_t>(1), limbo::internal::int_iterator<size_t>(9),
           [&](size_t i) { return solver()->Entails(k, val(p) == n(i), limbo::Solver::kConsistencyGuarantee) ==
                                  (r && r.val == n(i)); }));
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
  limbo::format::cpp::HiTerm n(std::size_t n) const {
    return vals_[n-1];
  }

  limbo::format::cpp::HiTerm val(Point p) const {
    return val(p.x, p.y);
  }

  limbo::format::cpp::HiTerm val(std::size_t x, std::size_t y) const {
    return val_(vals_[x-1], vals_[y-1]);
  }

  int max_k_;

  limbo::format::cpp::Context ctx_;

  limbo::Symbol::Sort VAL_;
  limbo::format::cpp::HiSymbol val_;
  std::vector<limbo::format::cpp::HiTerm> vals_;

  Timer t_;
};

#endif  // EXAMPLES_SUDOKU_KB_H_

