// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_KB_H_
#define EXAMPLES_MINESWEEPER_KB_H_

#include <sstream>
#include <vector>

#include <lela/solver.h>
#include <lela/internal/maybe.h>
#include <lela/internal/iter.h>
#include <lela/format/output.h>
#include <lela/format/cpp/syntax.h>

#include "game.h"
#include "timer.h"

namespace util {

template<class Container, class ConstIterator>
void Subsets(const ConstIterator first,
             const ConstIterator last,
             const size_t n,
             const Container& current,
             std::set<Container>* subsets) {
  if (current.size() == n) {
    subsets->insert(current);
    return;
  }
  if (first == last ||
      current.size() + static_cast<ssize_t>(std::distance(first, last)) < n) {
    return;
  }
  Subsets(std::next(first), last, n, current, subsets);
  Container current1 = current;
  current1.push_back(*first);
  Subsets(std::next(first), last, n, current1, subsets);
}

template<class Container>
std::set<Container> Subsets(const Container& s, size_t n) {
  std::set<Container> ss;
  Subsets(s.begin(), s.end(), n, {}, &ss);
  return ss;
}

}  // namespace util


class KnowledgeBase {
 public:
  KnowledgeBase(const Game* g, size_t max_k)
      : g_(g),
        max_k_(max_k),
        Bool(ctx_.CreateSort()),
        XPos(ctx_.CreateSort()),
        YPos(ctx_.CreateSort()),
        T(ctx_.CreateName(Bool)),
#ifdef USE_DETERMINES
        F(ctx_.CreateName(Bool)),
#endif
        MineF(ctx_.CreateFunction(Bool, 2)) {
    lela::format::output::RegisterSort(Bool, "");
    lela::format::output::RegisterSort(XPos, "");
    lela::format::output::RegisterSort(YPos, "");
    lela::format::output::RegisterSymbol(T.symbol(), "T");
#ifdef USE_DETERMINES
    lela::format::output::RegisterSymbol(F.symbol(), "F");
#endif
    lela::format::output::RegisterSymbol(MineF, "Mine");
    X.resize(g_->width());
    for (size_t i = 0; i < g_->width(); ++i) {
      X[i] = ctx_.CreateName(XPos);
      std::stringstream ss;
      ss << "#X" << i;
      lela::format::output::RegisterSymbol(X[i].symbol(), ss.str());
    }
    Y.resize(g_->height());
    for (size_t i = 0; i < g_->height(); ++i) {
      Y[i] = ctx_.CreateName(YPos);
      std::stringstream ss;
      ss << "#Y" << i;
      lela::format::output::RegisterSymbol(Y[i].symbol(), ss.str());
    }
    processed_.resize(g_->n_fields(), false);
  }

  size_t max_k() const { return max_k_; }

  const lela::Solver& solver() const { return ctx_.solver(); }
  const lela::Setup& setup() const { return solver().setup(); }

  lela::internal::Maybe<bool> IsMine(Point p, int k) {
    t_.start();
    lela::internal::Maybe<bool> r = lela::internal::Nothing;
#ifdef USE_DETERMINES
    // Doesn't work because we represent 'false' with "/= T".
    // If we used a second name, "F", and constraints that every
    // field is T or F, it would be fine.
    lela::internal::Maybe<lela::Term> is_mine = solver()->Determines(k, Mine(p), lela::Solver::kConsistencyGuarantee);
    assert(!is_mine || is_mine.val == T || is_mine.val == F);
    assert(solver()->Entails(k, *lela::Formula::Factory::Atomic(lela::Clause{MineLit(true, p)}),
                             lela::Solver::kConsistencyGuarantee) == (is_mine && is_mine.val == T));
    assert(solver()->Entails(k, *lela::Formula::Factory::Atomic(lela::Clause{MineLit(false, p)}),
                             lela::Solver::kConsistencyGuarantee) == (is_mine && is_mine.val == F));
    if (is_mine) {
      assert(!is_mine.val.null());
      r = lela::internal::Just(is_mine.val == T);
      assert(g_->mine(p) == r.val);
    }
#else
    lela::Formula::Ref yes_mine = lela::Formula::Factory::Atomic(lela::Clause{MineLit(true, p)});
    lela::Formula::Ref no_mine = lela::Formula::Factory::Atomic(lela::Clause{MineLit(false, p)});
    if (solver()->Entails(k, *yes_mine, lela::Solver::kConsistencyGuarantee)) {
      assert(g_->mine(p));
      r = lela::internal::Just(true);
    } else if (solver()->Entails(k, *no_mine, lela::Solver::kConsistencyGuarantee)) {
      assert(!g_->mine(p));
      r = lela::internal::Just(false);
    }
#endif
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
  lela::Term Mine(Point p) const {
    return MineF(X[p.x], Y[p.y]);
  }

  lela::Literal MineLit(bool is, Point p) const {
    lela::Term t = Mine(p);
#ifdef USE_DETERMINES
    return is ? lela::Literal::Eq(t, T) : lela::Literal::Eq(t, F);
#else
    return is ? lela::Literal::Eq(t, T) : lela::Literal::Neq(t, T);
#endif
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
        AddClause(lela::Clause{MineLit(true, p)});
        return true;
      }
      case Game::HIT_MINE: {
        AddClause(lela::Clause{MineLit(true, p)});
        return true;
      }
      default: {
        const std::vector<Point>& ns = g_->neighbors_of(p);
        const int n = ns.size();
        for (const std::vector<Point>& ps : util::Subsets(ns, n - m + 1)) {
          AddClause(MineClause(true, ps));
        }
        for (const std::vector<Point>& ps : util::Subsets(ns, m + 1)) {
          AddClause(MineClause(false, ps));
        }
        AddClause(lela::Clause{MineLit(false, p)});
        return true;
      }
    }
    throw;
  }

  void UpdateRemainingMines(size_t m, size_t n) {
    std::vector<Point> fields;
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!g_->opened(index) && !g_->flagged(index)) {
        fields.push_back(g_->to_point(index));
      }
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, n - m + 1)) {
      AddClause(MineClause(true, ps));
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, m + 1)) {
      AddClause(MineClause(false, ps));
    }
  }

  void AddClause(const lela::Clause& c) {
#ifdef USE_DETERMINES
    for (lela::Literal a : c) {
      lela::Term t = a.lhs();
      if (closure_added_.find(t) == closure_added_.end()) {
        solver()->AddClause(lela::Clause{lela::Literal::Eq(t, T), lela::Literal::Eq(t, F)});
        closure_added_.insert(t);
      }
    }
#endif
    solver()->AddClause(c);
  }

  lela::Solver* solver() { return ctx_.solver(); }

  const Game* g_;
  size_t max_k_;

  lela::format::cpp::Context ctx_;

  lela::Symbol::Sort Bool;
  lela::Symbol::Sort XPos;
  lela::Symbol::Sort YPos;
  lela::format::cpp::HiTerm T;               // name for positive truth value
#ifdef USE_DETERMINES
  lela::format::cpp::HiTerm F;               // name for negative truth value
#endif
  std::vector<lela::format::cpp::HiTerm> X;  // names for X positions
  std::vector<lela::format::cpp::HiTerm> Y;  // names for Y positions
  lela::format::cpp::HiSymbol MineF;

#ifdef USE_DETERMINES
  std::unordered_set<lela::Term> closure_added_;
#endif

  std::vector<bool> processed_;
  size_t n_rem_mines_ = 11;
  size_t n_rem_fields_ = 11;
  Timer t_;
};

#endif  // EXAMPLES_MINESWEEPER_KB_H_

