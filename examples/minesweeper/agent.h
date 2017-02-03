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
      *os_ << "Exploring X and Y coordinates: " << p.x << " " << p.y << " chosen at random" << std::endl;
      g_->OpenWithFrontier(p);
      last_point_ = p;
      return -1;
    }

    // First look for a field which is known not to be a mine.
    for (int k = 0; k <= kb_->max_k(); ++k) {
      std::vector<bool> inspected(g_->n_fields(), false);
      const std::size_t max_radius = std::max(last_point_.x, g_->width() - last_point_.x) +
                                     std::max(last_point_.y, g_->height() - last_point_.y);
      for (std::size_t radius = 0; radius <= max_radius; ++radius) {
        std::vector<bool> on_circle = Circle(last_point_, radius);
        for (std::size_t i = 0; i < g_->n_fields(); ++i) {
          if (inspected[i] || !on_circle[i]) {
            continue;
          }
          const Point p = g_->to_point(i);
          if (g_->opened(p) || g_->flagged(p)) {
            continue;
          }
          const lela::internal::Maybe<bool> r = kb_->IsMine(p, k);
          if (r.yes) {
            if (r.val) {
              *os_ << "Flagging X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
              g_->Flag(p);
            } else {
              *os_ << "Exploring X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
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
      *os_ << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is just a guess." << std::endl;
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
  std::vector<bool> Circle(Point p, int radius) {
    const int x0 = p.x;
    const int y0 = p.y;
    std::vector<bool> circle(g_->n_fields());
    int x = radius;
    int y = 0;
    int err = 0;
    while (x >= y) {
      set_circle(&circle, x0 + x, y0 + y);
      set_circle(&circle, x0 + y, y0 + x);
      set_circle(&circle, x0 - y, y0 + x);
      set_circle(&circle, x0 - x, y0 + y);
      set_circle(&circle, x0 - x, y0 - y);
      set_circle(&circle, x0 - y, y0 - x);
      set_circle(&circle, x0 + y, y0 - x);
      set_circle(&circle, x0 + x, y0 - y);
      if (err <= 0) {
        y += 1;
        err += 2*y + 1;
      }
      if (err > 0) {
        x -= 1;
        err -= 2*x + 1;
      }
    }
    return circle;
  }

  void set_circle(std::vector<bool>* circle, int xx, int yy) {
    for (int x = xx-1; x <= xx+1; ++x) {
      for (int y = yy-1; y <= yy+1; ++y) {
        if (0 <= x && x <= g_->width() && 0 <= y && y <= g_->height()) {
          (*circle)[g_->to_index(Point(x, y))] = true;
        }
      }
    }
  }

  Game* g_;
  KnowledgeBase* kb_;
  std::ostream* os_;
  Point last_point_;
};

#endif  // EXAMPLES_MINESWEEPER_AGENT_H_

