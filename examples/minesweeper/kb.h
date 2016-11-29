// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_KB_H_
#define EXAMPLES_MINESWEEPER_KB_H_

#include <sstream>
#include <vector>

#include <lela/solver.h>
#include <lela/internal/maybe.h>
#include <lela/internal/iter.h>
#include <lela/format/output.h>
#include <lela/format/syntax.h>

#include "game.h"
#include "timer.h"

class KnowledgeBase {
 public:
  KnowledgeBase(const Game* g, int max_k)
      : g_(g),
        max_k_(max_k),
        ctx_(solver_.sf(), solver_.tf()),
        Bool(ctx_.NewSort()),
        XPos(ctx_.NewSort()),
        YPos(ctx_.NewSort()),
        T(ctx_.NewName(Bool)),
        Mine(ctx_.NewFun(Bool, 2)) {
    lela::format::RegisterSort(Bool, "");
    lela::format::RegisterSort(XPos, "");
    lela::format::RegisterSort(YPos, "");
    lela::format::RegisterSymbol(T.symbol(), "T");
    lela::format::RegisterSymbol(Mine, "Mine");
    X.resize(g_->width());
    for (size_t i = 0; i < g_->width(); ++i) {
      X[i] = ctx_.NewName(XPos);
      std::stringstream ss;
      ss << "#X" << i;
      lela::format::RegisterSymbol(X[i].symbol(), ss.str());
    }
    Y.resize(g_->height());
    for (size_t i = 0; i < g_->height(); ++i) {
      Y[i] = ctx_.NewName(YPos);
      std::stringstream ss;
      ss << "#Y" << i;
      lela::format::RegisterSymbol(Y[i].symbol(), ss.str());
    }
    processed_.resize(g_->n_fields(), false);
  }

  int max_k() const { return max_k_; }

  const lela::Setup& setup() { return solver_.setup(); }

  lela::internal::Maybe<bool> IsMine(Point p, int k) {
    t_.start();
    lela::internal::Maybe<bool> r = lela::internal::Nothing;
    lela::Formula yes_mine = lela::Formula::Clause(lela::Clause{MineLit(true, p)});
    lela::Formula no_mine = lela::Formula::Clause(lela::Clause{MineLit(false, p)});
    if (solver_.Entails(k, yes_mine.reader())) {
      assert(g_->mine(p));
      r = lela::internal::Just(true);
    } else if (solver_.Entails(k, no_mine.reader())) {
      assert(!g_->mine(p));
      r = lela::internal::Just(false);
    }
    t_.stop();
    return r;
  }

  void Sync() {
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!processed_[index]) {
        processed_[index] = Update(g_->to_point(index));
      }
    }
    const size_t m = g_->n_mines() - g_->n_flags();
    const size_t n = g_->n_fields() - g_->n_opens() - g_->n_flags();
    if (m < n_rem_mines_ && n < n_rem_fields_) {
      UpdateRemainingMines(m, n);
      n_rem_mines_ = m;
      n_rem_fields_ = n;
    }
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

 private:
  lela::Literal MineLit(bool is, Point p) const {
    lela::Term t = Mine(X[p.x], Y[p.y]);
    return is ? lela::Literal::Eq(t, T) : lela::Literal::Neq(t, T);
  }

  lela::Clause MineClause(bool sign, const std::vector<Point> ns) const {
    auto r = lela::internal::transform_range(ns.begin(), ns.end(), [this, sign](Point p) { return MineLit(sign, p); });
    return lela::Clause(r.begin(), r.end());
  }

  bool Update(Point p) {
    assert(g_->valid(p));
    const int m = g_->state(p);
    switch (m) {
      case Game::UNEXPLORED: {
        return false;
      }
      case Game::FLAGGED: {
        solver_.AddClause(lela::Clause{MineLit(true, p)});
        return true;
      }
      case Game::HIT_MINE: {
        solver_.AddClause(lela::Clause{MineLit(true, p)});
        return true;
      }
      default: {
        const std::vector<Point>& ns = g_->neighbors_of(p);
        const int n = ns.size();
        for (const std::vector<Point>& ps : util::Subsets(ns, n - m + 1)) {
          solver_.AddClause(MineClause(true, ps));
        }
        for (const std::vector<Point>& ps : util::Subsets(ns, m + 1)) {
          solver_.AddClause(MineClause(false, ps));
        }
        solver_.AddClause(lela::Clause{MineLit(false, p)});
        return true;
      }
    }
  }

  void UpdateRemainingMines(size_t m, size_t n) {
    std::vector<Point> fields;
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!g_->opened(index) && !g_->flagged(index)) {
        fields.push_back(g_->to_point(index));
      }
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, n - m + 1)) {
      solver_.AddClause(MineClause(true, ps));
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, m + 1)) {
      solver_.AddClause(MineClause(false, ps));
    }
  }

  const Game* g_;
  int max_k_;

  lela::Solver solver_;
  lela::format::Context ctx_;

  lela::Symbol::Sort Bool;
  lela::Symbol::Sort XPos;
  lela::Symbol::Sort YPos;
  lela::format::HiTerm T;               // name for positive truth value
  std::vector<lela::format::HiTerm> X;  // names for X positions
  std::vector<lela::format::HiTerm> Y;  // names for Y positions
  lela::format::HiSymbol Mine;

  std::vector<bool> processed_;
  size_t n_rem_mines_ = 11;
  size_t n_rem_fields_ = 11;
  Timer t_;
};

#endif  // EXAMPLES_MINESWEEPER_KB_H_

