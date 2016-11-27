// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef EXAMPLES_MINESWEEPER_GAME_H_
#define EXAMPLES_MINESWEEPER_GAME_H_

#include <cassert>
#include <cmath>

#include <algorithm>
#include <ostream>
#include <random>
#include <set>
#include <vector>

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

 private:
  friend std::ostream& operator<<(std::ostream& os, const Point& p);
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << "(" << p.x << " | " << p.y << ")";
}


namespace util {

template<class T>
T faculty(T n) {
  T r = 1;
  while (n > 0) {
    r *= n;
    --n;
  }
  return r;
}

template<class T>
T choice(T n, T k) {
  return faculty(n) / faculty(k) / faculty(n - k);
}

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


class Game {
 public:
  static constexpr int HIT_MINE = -1;
  static constexpr int UNEXPLORED = -2;
  static constexpr int FLAGGED = -4;

  Game(size_t width, size_t height, size_t n_mines, size_t seed = 0)
      : width_(std::max(width, height)),
        height_(std::min(width, height)),
        n_mines_(n_mines),
        seed_(seed),
        distribution_(0, n_fields() - 1) {
    mines_.resize(n_fields(), false);
    opens_.resize(n_fields(), false);
    flags_.resize(n_fields(), false);
    frontier_.resize(n_fields(), false);
    neighbors_.resize(n_fields());
    assert(mines_.size() == n_fields());
    assert(n_mines_ + 9 <= n_fields());
    generator_.seed(n_fields() * n_mines + seed);
#ifndef NDEBUG
    for (size_t i = 0 ; i < n_fields(); ++i) {
      for (size_t j = 0; j < n_fields(); ++j) {
        assert((i == j) == (to_point(i) == to_point(j)));
      }
    }
#endif
  }

  size_t n_fields() const { return width_ * height_; }
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  size_t n_mines() const { return n_mines_; }

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

  void set_mine(const Point p, const bool is_mine) {
    assert(valid(p));
    mines_[to_index(p)] = is_mine;
  }

  bool mine(const size_t index) const {
    return mines_[index];
  }

  bool mine(const Point p) const {
    assert(valid(p));
    return mine(to_index(p));
  }

  bool opened(const size_t index) const {
    return opens_[index];
  }

  bool opened(const Point p) const {
    assert(valid(p));
    return opened(to_index(p));
  }

  bool flagged(const size_t index) const {
    return flags_[index];
  }

  bool flagged(const Point p) const {
    assert(valid(p));
    return flagged(to_index(p));
  }

  bool frontier(const size_t index) const {
    return frontier_[index];
  }

  bool frontier(const Point p) const {
    assert(valid(p));
    return frontier(to_index(p));
  }

  int Open(const Point p) {
    // Place mines at first open
    if (n_opens() == 0) {
      for (size_t n = 0; n < n_mines_; ) {
        const Point q = RandomPoint();
        if (!mine(q) && !Point::Adjacent(p, q)) {
          set_mine(q, true);
          ++n;
        }
      }
    }

    assert(valid(p));
    assert(!opened(p));
    assert(!flagged(p));
    const size_t index = to_index(p);
    opens_[index] = true;
    frontier_[index] = false;
    for (const Point q : neighbors_of(p)) {
      assert(valid(q));
      if (!opened(q) && !flagged(q)) {
        frontier_[to_index(q)] = true;
      }
    }
    ++n_opens_;
    assert(opened(p));

    assert(valid(p));
    const int s = state(p);
    hit_mine_ |= s == HIT_MINE;
    return s;
  }

  int OpenWithFrontier(Point p) {
    int s = Open(p);
    if (s == 0) {
      for (const Point q : neighbors_of(p)) {
        if (!opened(q)) {
          OpenWithFrontier(q);
        }
      }
    }
    return s;
  }

  void Flag(const Point p) {
    assert(valid(p));
    assert(mine(p));
    assert(!flagged(p));
    const size_t index = to_index(p);
    flags_[index] = true;
    frontier_[index] = false;
    ++n_flags_;
    assert(flagged(p));
  }

  int state(Point p) const {
    assert(valid(p));
    if (flagged(p)) {
      return FLAGGED;
    }
    if (!opened(p)) {
      return UNEXPLORED;
    }
    if (mine(p)) {
      return HIT_MINE;
    }
    int adjacent_mines = 0;
    for (const Point q : neighbors_of(p)) {
      if (mine(q)) {
        ++adjacent_mines;
      }
    }
    return adjacent_mines;
  }

  int state_minus_flags(Point p) const {
    assert(valid(p));
    if (flagged(p)) {
      return FLAGGED;
    }
    if (!opened(p)) {
      return UNEXPLORED;
    }
    if (mine(p)) {
      return HIT_MINE;
    }
    int adjacent_mines = 0;
    for (const Point q : neighbors_of(p)) {
      if (mine(q) && !flagged(q)) {
        ++adjacent_mines;
      }
    }
    return adjacent_mines;
  }

  int unopened_unflagged_neighbors(Point p) const {
    int n = 0;
    for (const Point q : neighbors_of(p)) {
      if (!opened(q) && !flagged(q)) {
        ++n;
      }
    }
    return n;
  }

  size_t n_opens() const { return n_opens_; }
  size_t n_flags() const { return n_flags_; }
  size_t seed() const { return seed_; }
  bool hit_mine() const { return hit_mine_; }
  bool all_explored() const { return n_opens() + n_mines_ == n_fields(); }

 private:
  const size_t width_;
  const size_t height_;
  const size_t n_mines_;
  const size_t seed_;
  size_t n_opens_ = 0;
  size_t n_flags_ = 0;
  bool hit_mine_ = false;
  std::vector<bool> mines_;
  std::vector<bool> opens_;
  std::vector<bool> flags_;
  std::vector<bool> frontier_;
  mutable std::vector<std::vector<Point>> neighbors_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<size_t> distribution_;
};

#endif  // EXAMPLES_MINESWEEPER_GAME_H_

