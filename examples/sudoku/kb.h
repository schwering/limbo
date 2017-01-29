// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_KB_H_
#define EXAMPLES_SUDOKU_KB_H_

#include <sstream>
#include <vector>

#include <lela/solver.h>
#include <lela/internal/maybe.h>
#include <lela/internal/iter.h>
#include <lela/format/output.h>
#include <lela/format/cpp/syntax.h>

#include "game.h"
#include "timer.h"

class KnowledgeBase {
 public:
  KnowledgeBase(const Game* g, int max_k)
      : max_k_(max_k),
        VAL_(ctx_.CreateSort()),
        val_(ctx_.CreateFunction(VAL_, 2)) {
    lela::format::output::RegisterSort(VAL_, "");
    lela::format::output::RegisterSymbol(val_, "val");
    using namespace lela::format::cpp;
    for (std::size_t i = 0; i < 9; ++i) {
      vals_.push_back(ctx_.CreateName(VAL_));
      std::stringstream ss;
      ss << i + 1;
      lela::format::output::RegisterSymbol(vals_.back().symbol(), ss.str());
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        for (std::size_t yy = 0; yy < 9; ++yy) {
          if (y != yy) {
            ctx_.AddClause(val(x, y) != val(x, yy));
          }
        }
      }
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t xx = 0; xx < 9; ++xx) {
        for (std::size_t y = 0; y < 9; ++y) {
          if (x != xx) {
            ctx_.AddClause(val(x, y) != val(xx, y));
          }
        }
      }
    }
    for (std::size_t i = 0; i < 3; ++i) {
      for (std::size_t j = 0; j < 3; ++j) {
        for (std::size_t x = 3*i; x < 3*i+3; ++x) {
          for (std::size_t xx = 3*i; xx < 3*i+3; ++xx) {
            for (std::size_t y = 3*j; y < 3*j+3; ++y) {
              for (std::size_t yy = 3*j; yy < 3*j+3; ++yy) {
                if (x != xx || y != yy) {
                  ctx_.AddClause(val(x, y) != val(xx, yy));
                }
              }
            }
          }
        }
      }
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        std::vector<lela::Literal> lits;
        for (std::size_t i = 0; i < 9; ++i) {
          lits.push_back(lela::Literal::Eq(val(x, y), n(i)));
        }
        ctx_.AddClause(lela::Clause(lits.begin(), lits.end()));
      }
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        int i = g->get(Point(x, y));
        if (i != 0) {
          ctx_.AddClause(val(x, y) == n(i-1));
        }
      }
    }
  }

  int max_k() const { return max_k_; }

  lela::Solver* solver() { return ctx_.solver(); }
  const lela::Solver& solver() const { return ctx_.solver(); }
  const lela::Setup& setup() const { return solver().setup(); }

  void Add(Point p, int i) {
    ctx_.AddClause(val(p) == n(i-1));
  }

  lela::internal::Maybe<int> Val(Point p, int k) {
    t_.start();
    for (std::size_t i = 0; i < 9; ++i) {
      //using lela::format::output::operator<<;
      //std::cout << "Entails(" << k << ", " << *(val(p) == n(i)) << ")?" << std::endl;
      if (solver()->Entails(k, val(p) == n(i))) {
        return lela::internal::Just(i+1);
      }
    }
    t_.stop();
    return lela::internal::Nothing;
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

 private:
  lela::format::cpp::HiTerm n(std::size_t n) const {
    return vals_[n];
  }

  lela::format::cpp::HiTerm val(Point p) const {
    return val(p.x, p.y);
  }

  lela::format::cpp::HiTerm val(std::size_t x, std::size_t y) const {
    return val_(vals_[x], vals_[y]);
  }

  int max_k_;

  lela::format::cpp::Context ctx_;

  lela::Symbol::Sort VAL_;
  lela::format::cpp::HiSymbol val_;
  std::vector<lela::format::cpp::HiTerm> vals_;

  Timer t_;
};

#endif  // EXAMPLES_SUDOKU_KB_H_

