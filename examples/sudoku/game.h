// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_GAME_H_
#define EXAMPLES_SUDOKU_GAME_H_

#include <cassert>
#include <cmath>

#include <algorithm>
#include <ostream>
#include <random>
#include <set>
#include <string>
#include <vector>

struct Point {
  Point() {}
  Point(std::size_t x, std::size_t y) : x(x), y(y) {}

  bool operator<(const Point& p) const {
    return x < p.x || (x == p.x && y < p.y);
  }

  bool operator==(const Point& p) const {
    return x == p.x && y == p.y;
  }

  std::size_t x;
  std::size_t y;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << "(" << p.x << " | " << p.y << ")";
}


class Game {
 public:
  explicit Game(const std::string& cfg) {
    for (int i = 0; i < 9*9; ++i) {
      const char c = i < static_cast<int>(cfg.length()) ? cfg.at(i) : 0;
      const char n = '1' <= c && c <= '9' ? c - '1' + 1 : 0;
      const int x = (i % 9) + 1;
      const int y = (i / 9) + 1;
      set(x, y, n);
    }
  }

  int get(int x, int y) const { return cells_[x-1][y-1]; }
  void set(int x, int y, int n) { cells_[x-1][y-1] = n; }

  int get(Point p) const { return get(p.x, p.y); }
  void set(Point p, int n) { set(p.x, p.y, n); }

  bool solved() const {
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        if (get(x, y) == 0) {
          return false;
        }
      }
    }
    return true;
  }

  bool legal_solution() const {
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        for (std::size_t yy = 1; yy <= 9; ++yy) {
          if (y != yy && get(x, y) == get(x, yy)) {
            return false;
          }
        }
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t xx = 1; xx <= 9; ++xx) {
        for (std::size_t y = 1; y <= 9; ++y) {
          if (x != xx && get(x, y) == get(xx, y)) {
            return false;
          }
        }
      }
    }
    for (std::size_t i = 1; i < 3; ++i) {
      for (std::size_t j = 1; j < 3; ++j) {
        for (std::size_t x = 3*i-2; x <= 3*i; ++x) {
          for (std::size_t xx = 3*i-2; xx <= 3*i; ++xx) {
            for (std::size_t y = 3*j-2; y <= 3*j; ++y) {
              for (std::size_t yy = 3*j-2; yy <= 3*j; ++yy) {
                if ((x != xx || y != yy) && get(x, y) == get(xx, yy)) {
                  return false;
                }
              }
            }
          }
        }
      }
    }
    for (std::size_t x = 1; x <= 9; ++x) {
      for (std::size_t y = 1; y <= 9; ++y) {
        if (get(x, y) < 1 || get(x, y) > 9) {
          return false;
        }
      }
    }
    return true;
  }

 private:
  int cells_[9][9];
};

#endif  // EXAMPLES_SUDOKU_GAME_H_

