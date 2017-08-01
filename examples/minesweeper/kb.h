// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_KB_H_
#define EXAMPLES_MINESWEEPER_KB_H_

#include <sstream>
#include <vector>

#include <limbo/solver.h>
#include <limbo/internal/maybe.h>
#include <limbo/internal/iter.h>
#include <limbo/format/output.h>

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
        solver_(limbo::Symbol::Factory::Instance(), limbo::Term::Factory::Instance()),
        Bool(CreateSort()),
        XPos(CreateSort()),
        YPos(CreateSort()),
        T(CreateName(Bool)),
#ifdef USE_DETERMINES
        F(CreateName(Bool)),
#endif
        MineF(CreateFunctionSymbol(Bool, 2)) {
    limbo::format::RegisterSort(Bool, "");
    limbo::format::RegisterSort(XPos, "");
    limbo::format::RegisterSort(YPos, "");
    limbo::format::RegisterSymbol(T.symbol(), "T");
#ifdef USE_DETERMINES
    limbo::format::RegisterSymbol(F.symbol(), "F");
#endif
    limbo::format::RegisterSymbol(MineF, "Mine");
    X.resize(g_->width());
    for (size_t i = 0; i < g_->width(); ++i) {
      X[i] = CreateName(XPos);
      std::stringstream ss;
      ss << "#X" << i;
      limbo::format::RegisterSymbol(X[i].symbol(), ss.str());
    }
    Y.resize(g_->height());
    for (size_t i = 0; i < g_->height(); ++i) {
      Y[i] = CreateName(YPos);
      std::stringstream ss;
      ss << "#Y" << i;
      limbo::format::RegisterSymbol(Y[i].symbol(), ss.str());
    }
    processed_.resize(g_->n_fields(), false);
  }

  size_t max_k() const { return max_k_; }

  limbo::Solver& solver() { UpdateSolver(); return solver_; }
  const limbo::Solver& solver() const { const_cast<KnowledgeBase&>(*this).UpdateSolver(); return solver_; }
  const limbo::Setup& setup() const { const_cast<KnowledgeBase&>(*this).UpdateSolver(); return solver().setup(); }

  limbo::internal::Maybe<bool> IsMine(Point p, int k) {
    t_.start();
    limbo::internal::Maybe<bool> r = limbo::internal::Nothing;
#ifdef USE_DETERMINES
    // In this case we need a second name to represent falsity. The reason is
    // that without such a name, falsity is represented by "/= T", which is for
    // Determines() just means the term's value is not determined, whereas we'd
    // want it to be determined as false.
    limbo::internal::Maybe<limbo::Term> is_mine = solver().Determines(k, Mine(p), limbo::Solver::kConsistencyGuarantee);
    assert(!is_mine || is_mine.val == T || is_mine.val == F);
    assert(solver().Determines(k, Mine(p), limbo::Solver::kNoConsistencyGuarantee) == is_mine);
    assert(solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(true, p)}),
                            limbo::Solver::kConsistencyGuarantee) ==
           (is_mine && is_mine.val == T));
    assert(solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(true, p)}),
                            limbo::Solver::kConsistencyGuarantee) ==
           solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(true, p)}),
                            limbo::Solver::kNoConsistencyGuarantee));
    assert(solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(false, p)}),
                            limbo::Solver::kConsistencyGuarantee) ==
           solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(false, p)}),
                            limbo::Solver::kNoConsistencyGuarantee));
    assert(solver().Entails(k, *limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(false, p)}),
                            limbo::Solver::kConsistencyGuarantee) ==
           (is_mine && is_mine.val == F));
    if (is_mine) {
      assert(!is_mine.val.null());
      r = limbo::internal::Just(is_mine.val == T);
      assert(g_->mine(p) == r.val);
    }
#else
    limbo::Formula::Ref yes_mine = limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(true, p)});
    limbo::Formula::Ref no_mine = limbo::Formula::Factory::Atomic(limbo::Clause{MineLit(false, p)});
    if (solver().Entails(k, *yes_mine, limbo::Solver::kConsistencyGuarantee)) {
      assert(g_->mine(p));
      r = limbo::internal::Just(true);
    } else if (solver().Entails(k, *no_mine, limbo::Solver::kConsistencyGuarantee)) {
      assert(!g_->mine(p));
      r = limbo::internal::Just(false);
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
#ifdef END_GAME_CLAUSES
    const size_t m = g_->n_mines() - g_->n_flags();
    const size_t n = g_->n_fields() - g_->n_opens() - g_->n_flags();
    if (m < n_rem_mines_ && n < n_rem_fields_) {
      UpdateRemainingMines(m, n);
      n_rem_mines_ = m;
      n_rem_fields_ = n;
    }
#endif
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

 private:
  limbo::Term Mine(Point p) const { return CreateFunction(MineF, limbo::Term::Vector{X[p.x], Y[p.y]}); }

  limbo::Literal MineLit(bool is, Point p) const {
    limbo::Term t = Mine(p);
#ifdef USE_DETERMINES
    return limbo::Literal::Eq(t, is ? T : F);
#else
    return is ? limbo::Literal::Eq(t, T) : limbo::Literal::Neq(t, T);
#endif
  }

  limbo::Clause MineClause(bool sign, const std::vector<Point> ns) const {
    auto r = limbo::internal::transform_range(ns.begin(), ns.end(), [this, sign](Point p) { return MineLit(sign, p); });
    return limbo::Clause(r.begin(), r.end());
  }

  bool Update(Point p) {
    assert(g_->valid(p));
    const int m = g_->state(p);
    switch (m) {
      case Game::UNEXPLORED: {
        return false;
      }
      case Game::FLAGGED: {
        Add(limbo::Clause{MineLit(true, p)});
        return true;
      }
      case Game::HIT_MINE: {
        Add(limbo::Clause{MineLit(true, p)});
        return true;
      }
      default: {
        const std::vector<Point>& ns = g_->neighbors_of(p);
        const int n = ns.size();
        for (const std::vector<Point>& ps : util::Subsets(ns, n - m + 1)) {
          Add(MineClause(true, ps));
        }
        for (const std::vector<Point>& ps : util::Subsets(ns, m + 1)) {
          Add(MineClause(false, ps));
        }
        Add(limbo::Clause{MineLit(false, p)});
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
      Add(MineClause(true, ps));
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, m + 1)) {
      Add(MineClause(false, ps));
    }
  }

  void Add(const limbo::Literal a) { return Add(limbo::Clause{a}); }

  void Add(const limbo::Clause& c) {
#ifdef USE_DETERMINES
    for (limbo::Literal a : c) {
      limbo::Term t = a.lhs();
      if (closure_added_.find(t) == closure_added_.end()) {
        clauses_.push_back(limbo::Clause{limbo::Literal::Eq(t, T), limbo::Literal::Eq(t, F)});
        closure_added_.insert(t);
      }
    }
#endif
    clauses_.push_back(c);
  }

  void UpdateSolver() {
    if (n_processed_clauses_ == clauses_.size()) {
      return;
    }
    solver_.grounder().AddClauses(clauses_.begin() + n_processed_clauses_, clauses_.end());
    n_processed_clauses_ = clauses_.size();
  }

  limbo::Symbol::Sort CreateSort() const {
    return limbo::Symbol::Factory::Instance()->CreateSort();
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

  const Game* g_;
  size_t max_k_;

  std::vector<limbo::Clause> clauses_;
  size_t n_processed_clauses_;

  limbo::Solver solver_;

#ifdef USE_DETERMINES
  std::unordered_set<limbo::Term> closure_added_;
#endif

  limbo::Symbol::Sort Bool;
  limbo::Symbol::Sort XPos;
  limbo::Symbol::Sort YPos;
  limbo::Term T;               // name for positive truth value
#ifdef USE_DETERMINES
  limbo::Term F;               // name for negative truth value
#endif
  std::vector<limbo::Term> X;  // names for X positions
  std::vector<limbo::Term> Y;  // names for Y positions
  limbo::Symbol MineF;

  std::vector<bool> processed_;
#ifdef END_GAME_CLAUSES
  size_t n_rem_mines_ = 11;
  size_t n_rem_fields_ = 11;
#endif
  Timer t_;
};

#endif  // EXAMPLES_MINESWEEPER_KB_H_

