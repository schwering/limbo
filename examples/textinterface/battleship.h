// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_BATTLESHIP_GAME_H_
#define EXAMPLES_BATTLESHIP_GAME_H_

#include <cassert>
#include <cmath>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <random>
#include <set>
#include <vector>

#include <lela/term.h>
#include <lela/clause.h>
#include <lela/literal.h>

struct Point {
  Point() {}
  Point(size_t x, size_t y) : x(x), y(y) {}

  bool operator<(const Point& p) const {
    return x < p.x || (x == p.x && y < p.y);
  }

  bool operator==(const Point& p) const {
    return x == p.x && y == p.y;
  }

  static bool Adjacent(Point p, Point q) {
    const size_t xd = std::max(p.x, q.x) - std::min(p.x, q.x);
    const size_t yd = std::max(p.y, q.y) - std::min(p.y, q.y);
    return xd <= 1 && yd <= 1;
  }

  static double Distance(Point p, Point q) {
    double x = p.x - q.x;
    double y = p.y - q.y;
    return ::sqrt(x*x + y*y);
  }

  size_t x;
  size_t y;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << "(" << p.x << " | " << p.y << ")";
}

class BattleshipGame {
 public:
  BattleshipGame(size_t width, size_t height, const std::vector<size_t>& ships, size_t seed = 0) :
      width_(width),
      height_(height),
      seed_(seed),
      distribution_(0, n_fields() - 1) {
    fired_.resize(n_fields(), false);
    ships_.resize(n_fields(), false);
    neighbors_.resize(n_fields());
    generator_.seed(n_fields() + seed);
    for (size_t i = 0; i < ships.size(); ++i) {
      const size_t ship_size = i + 1;
      const size_t n_ships = ships[i];
      for (size_t j = 0; j < n_ships; ++j) {
        PlaceRandom(ship_size);
      }
    }
  }

  size_t n_fields() const { return width_ * height_; }
  size_t width() const { return width_; }
  size_t height() const { return height_; }

  const std::vector<Point>& neighbors_of(Point p) const {
    std::vector<Point>& vec = neighbors_[to_index(p)];
    if (!vec.empty()) {
      return vec;
    }
    for (int i = -1; i <= 1; ++i) {
      for (int j = -1; j <= 1; ++j) {
        if (i != 0 || j != 0) {
          Point q(p.x + i, p.y + j);
          if (valid(q)) {
            vec.push_back(q);
          }
        }
      }
    }
    return vec;
  }

  Point RandomPoint() {
    return to_point(distribution_(generator_));
  }

  Point to_point(size_t index) const {
    Point p(index / height_, index % height_);
    assert(to_index(p) == index);
    return p;
  }

  size_t to_index(const Point p) const {
    return height_ * p.x + p.y;
  }

  bool valid(const Point p) const {
    return p.x < width_ && p.y < height_;
  }

  bool ship(const size_t index) const {
    assert(index < ships_.size());
    return ships_[index];
  }

  bool ship(const Point p) const {
    assert(valid(p));
    return ship(to_index(p));
  }

  bool fired(const size_t index) const {
    assert(index < fired_.size());
    return fired_[index];
  }

  bool fired(const Point p) const {
    assert(valid(p));
    return fired(to_index(p));
  }

  bool Fire(const size_t index) {
    fired_[index] = true;
    return ships_[index];
  }

  bool Fire(const Point p) {
    assert(valid(p));
    return Fire(to_index(p));
  }

  size_t seed() const { return seed_; }
  size_t n_hits() const {
    size_t n = 0;
    for (size_t i = 0; i < n_fields(); ++i) {
      if (ship(i) && fired(i)) {
        ++n;
      }
    }
    return n;
  }

 private:
  void PlaceRandom(size_t n) {
    std::vector<Point> ps;
    bool hori;
    Point head;
try_again:
    hori = false;// distribution_(generator_) % 2 == 0;
    ps.clear();
    for (size_t i = 0; i < n; ++i) {
      const size_t x0 = hori ? i : 0;
      const size_t y0 = hori ? 0 : i;
      const Point p = i == 0 ? RandomPoint() : Point(ps[0].x + x0, ps[0].y + y0);
      ps.push_back(p);
      if (!valid(p)) {
        goto try_again;
      }
      if (ship(p)) {
        goto try_again;
      }
      for (Point q : neighbors_of(p)) {
        if (ship(q)) {
          goto try_again;
        }
      }
    }
    for (Point p : ps) {
      set_ship(p);
    }
  }

  void set_ship(Point p) {
    assert(!ships_[to_index(p)]);
    ships_[to_index(p)] = true;
  }

  const size_t width_;
  const size_t height_;
  const size_t seed_;
  std::vector<bool> ships_;
  std::vector<bool> fired_;
  mutable std::vector<std::vector<Point>> neighbors_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<size_t> distribution_;
};

std::ostream& operator<<(std::ostream& os, const BattleshipGame& g) {
  const int width = 3;
  os << std::setw(width) << "";
  for (size_t x = 0; x < g.width(); ++x) {
    os << std::setw(width) << (x+1);
  }
  os << std::endl;
  for (size_t y = 0; y < g.height(); ++y) {
    os << std::setw(width) << (y+1);
    for (size_t x = 0; x < g.width(); ++x) {
      Point p(x, y);
      char label = ' ';
      if (g.ship(p) && g.fired(p)) {
        label = 'H';
      } else if (g.ship(p)) {
        label = 'S';
      } else if (g.fired(p)) {
        label = 'M';
      }
      os << std::setw(width) << label;
    }
    os << std::endl;
  }
  return os;
}

struct BattleshipCallbacks {
  template<typename T>
  bool operator()(T* ctx, const std::string& proc, const std::vector<lela::Term>& args) {
    if (proc == "bs_init" && !bs_) {
      if (args.size() == 4) {
        bs_ = std::make_shared<BattleshipGame>(1, 4, std::vector<size_t>{0, 0, 1});
      } else if (args.size() == 4*4) {
        bs_ = std::make_shared<BattleshipGame>(4, 4, std::vector<size_t>{0, 1, 1});
      }
      ps_ = args;
    } else if (bs_ && proc == "bs_print") {
      std::cout << *bs_ << std::endl;
    } else if (bs_ && proc == "bs_fire" && args.size() == 1) {
      Fire(ctx, Lookup(args[0]));
    } else if (bs_ && proc == "bs_fire_random") {
      for (;;) {
        const Point p = bs_->RandomPoint();
        if (!bs_->fired(p)) {
          Fire(ctx, bs_->RandomPoint());
          break;
        }
      }
    } else {
      return false;
    }
    return true;
  }

 private:
  template<typename T>
  void Fire(T* ctx, Point p) {
    const lela::Term t = Lookup(p);
    const bool is_water = !bs_->Fire(p);
    const lela::Term Water = ctx->CreateTerm(ctx->LookupFunction("water"), {t});
    const lela::Term Fired = ctx->CreateTerm(ctx->LookupFunction("fired"), {t});
    const lela::Term True = ctx->LookupName("T");
    ctx->kb()->Add(lela::Clause{is_water ? lela::Literal::Eq(Water, True) : lela::Literal::Neq(Water, True)});
    ctx->kb()->Add(lela::Clause{lela::Literal::Eq(Fired, True)});
  }

  Point Lookup(lela::Term t) {
    for (size_t i = 0; i < ps_.size(); ++i) {
      if (ps_[i] == t) {
        return bs_->to_point(i);
      }
    }
    throw;
  }

  lela::Term Lookup(Point p) {
    return ps_[bs_->to_index(p)];
  }

  std::shared_ptr<BattleshipGame> bs_;
  std::vector<lela::Term> ps_;
};

#endif  // EXAMPLES_BATTLESHIP_GAME_H_

