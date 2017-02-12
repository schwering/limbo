// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_AGENT_H_
#define EXAMPLES_MINESWEEPER_AGENT_H_

#include <algorithm>
#include <iostream>

#include "game.h"
#include "kb.h"

class Agent {
 public:
  virtual ~Agent() {}
  virtual int Explore() = 0;
};

class HumanAgent : public Agent {
 public:
  explicit HumanAgent(Game* g, std::ostream* os, std::istream* is) : g_(g), os_(os), is_(is) {}

  int Explore() override {
    for (;;) {
      Point p;
      *os_ << "Exploring X and Y coordinates: ";
      *is_ >> p.x >> p.y;
      if (!g_->valid(p) || g_->opened(p)) {
        std::cout << "Invalid coordinates, repeat" << std::endl;
        continue;
      }
      g_->OpenWithFrontier(p);
      return 0;
    }
  }

 private:
  Game* g_;
  std::ostream* os_;
  std::istream* is_;
};

class KnowledgeBaseAgent : public Agent {
 public:
  explicit KnowledgeBaseAgent(Game* g, KnowledgeBase* kb, std::ostream* os) : g_(g), kb_(kb), os_(os) {}

  int Explore() override {
    kb_->Sync();

    // First open a random point which is not at the edge of the field.
    if (g_->n_opens() == 0) {
      Point p;
      do {
        p = g_->RandomPoint();
      } while (g_->neighbors_of(p).size() < 8);
      *os_ << "Exploring " << p << ", chosen at random." << std::endl;
      g_->OpenWithFrontier(p);
      last_point_ = p;
      return -1;
    }

    // First look for a field which is known not to be a mine.
    for (size_t k = 0; k <= kb_->max_k(); ++k) {
      std::vector<bool> inspected(g_->n_fields(), false);
      const std::size_t max_radius = std::max(std::max(last_point_.x, g_->width() - last_point_.x),
                                              std::max(last_point_.y, g_->height() - last_point_.y));
      for (std::size_t radius = 0; radius <= max_radius; ++radius) {
        std::vector<bool> on_rectangle = Rectangle(last_point_, radius);
        for (std::size_t i = 0; i < g_->n_fields(); ++i) {
          if (inspected[i] || !on_rectangle[i]) {
            continue;
          }
          const Point p = g_->to_point(i);
          if (g_->opened(p) || g_->flagged(p)) {
            continue;
          }
          const lela::internal::Maybe<bool> r = kb_->IsMine(p, k);
          if (r.yes) {
            if (r.val) {
              *os_ << "Flagging " << p << ", found at split level " << k << "." << std::endl;
              g_->Flag(p);
            } else {
              *os_ << "Exploring " << p << ", found at split level " << k << "." << std::endl;
              g_->OpenWithFrontier(p);
            }
            last_point_ = p;
            return k;
          }
          inspected[i] = true;
        }
      }
    }

    // Didn't find any reliable action, so we need to guess.
    for (std::size_t i = 0; i < g_->n_fields(); ++i) {
      const Point p = g_->to_point(i);
      if (g_->opened(p) || g_->flagged(p)) {
        continue;
      }
      *os_ << "Exploring " << p << ", which is just a guess." << std::endl;
      g_->OpenWithFrontier(p);
      last_point_ = p;
      return kb_->max_k() + 1;
    }

    // That's weird, this case should never occur, because our reasoning should
    // be correct.
    std::cerr << __FILE__ << ":" << __LINE__ << ": Why didn't we choose a point?" << std::endl;
    assert(false);
    return -2;
  }

 private:
  std::vector<bool> Rectangle(Point p, int radius) {
    const int x0 = p.x;
    const int y0 = p.y;
    std::vector<bool> rectangle(g_->n_fields());
    for (int x = -radius; x <= radius; ++x) {
      set_rectangle(&rectangle, x0 + x, y0 + radius);
      set_rectangle(&rectangle, x0 + x, y0 - radius);
    }
    for (int y = -radius; y <= radius; ++y) {
      set_rectangle(&rectangle, x0 + radius, y0 - y);
      set_rectangle(&rectangle, x0 - radius, y0 - y);
    }
    return rectangle;
  }

  void set_rectangle(std::vector<bool>* rectangle, int x, int y) {
    if (0 <= x && x < static_cast<int>(g_->width()) && 0 <= y && y < static_cast<int>(g_->height())) {
      (*rectangle)[g_->to_index(Point(x, y))] = true;
    }
  }

  Game* g_;
  KnowledgeBase* kb_;
  std::ostream* os_;
  Point last_point_;
};

#endif  // EXAMPLES_MINESWEEPER_AGENT_H_

