// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_AGENT_H_
#define EXAMPLES_SUDOKU_AGENT_H_

#include <iostream>

#include "game.h"
#include "kb.h"

class Agent {
 public:
  virtual ~Agent() {}
  virtual bool Explore() = 0;
};

class KnowledgeBaseAgent : public Agent {
 public:
  explicit KnowledgeBaseAgent(Game* g, KnowledgeBase* kb, std::ostream* os) : g_(g), kb_(kb), os_(os) {}

  bool Explore() override {
    for (int k = 0; k <= kb_->max_k(); ++k) {
      for (std::size_t x = 0; x < 9; ++x) {
        for (std::size_t y = 0; y < 9; ++y) {
          Point p(x, y);
          if (g_->get(p) == 0) {
            const lela::internal::Maybe<int> r = kb_->Val(p, k);
            if (r) {
              *os_ << p << " = " << r.val << " found at split level " << k << std::endl;
              kb_->Add(p, r.val);
              g_->set(p, r.val);
              return true;
            }
          }
        }
      }
    }
    return false;
  }

 private:
  Game* g_;
  KnowledgeBase* kb_;
  std::ostream* os_;
};

#endif  // EXAMPLES_SUDOKU_AGENT_H_

