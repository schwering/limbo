// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_AGENT_H_
#define EXAMPLES_SUDOKU_AGENT_H_

#include <iostream>

#include <limbo/internal/maybe.h>

#include "game.h"
#include "kb.h"

class Agent {
 public:
  struct Result {
    Result() = default;
    Result(Point p, int n, int k) : p(p), n(n), k(k) {}
    Point p;
    int n;
    int k;
  };

  virtual ~Agent() {}
  virtual limbo::internal::Maybe<Result> Explore() = 0;
};

class KnowledgeBaseAgent : public Agent {
 public:
  KnowledgeBaseAgent(Game* g, KnowledgeBase* kb) : g_(g), kb_(kb) {}

  limbo::internal::Maybe<Result> Explore() override {
    for (int k = 0; k <= kb_->max_k(); ++k) {
      for (std::size_t x = 1; x <= 9; ++x) {
        for (std::size_t y = 1; y <= 9; ++y) {
          Point p(x, y);
          if (g_->get(p) == 0) {
            const limbo::internal::Maybe<int> r = kb_->Val(p, k);
            if (r) {
              const int n = r.val;
              kb_->Add(p, n);
              g_->set(p, n);
              return limbo::internal::Just(Result(p, n, k));
            }
          }
        }
      }
    }
    return limbo::internal::Nothing;
  }

 private:
  Game* g_;
  KnowledgeBase* kb_;
};

#endif  // EXAMPLES_SUDOKU_AGENT_H_

