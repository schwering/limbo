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
  explicit Game(const char* cfg) {
    for (int i = 0; i < 9*9; ++i) {
      int x = i % 9;
      int y = i / 9;
      cells_[x][y] = '1' <= cfg[i] && cfg[i] <= '9' ? cfg[i] - '1' + 1 : 0;
    }
  }

  int get(Point p) const { return cells_[p.x][p.y]; }
  void set(Point p, int n) { cells_[p.x][p.y] = n; }

  bool solved() const {
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        if (cells_[x][y] == 0) {
          return false;
        }
      }
    }
    return true;
  }

  bool legal_solution() const {
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        for (std::size_t yy = 0; yy < 9; ++yy) {
          if (y != yy && cells_[x][y] == cells_[x][yy]) {
            return false;
          }
        }
      }
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t xx = 0; xx < 9; ++xx) {
        for (std::size_t y = 0; y < 9; ++y) {
          if (x != xx && cells_[x][y] == cells_[xx][y]) {
            return false;
          }
        }
      }
    }
    for (std::size_t i = 0; i < 3; ++i) {
      for (std::size_t j = 0; j < 3; ++j) {
        for (std::size_t x = 3*i; x < 3*i+3; ++x) {
          for (std::size_t xx = 3*i; xx < 3*i+3; ++xx) {
            for (std::size_t y = 3*j; y < 3*j+3; ++y) {
              for (std::size_t yy = 3*j; yy < 3*j+3; ++yy) {
                if ((x != xx || y != yy) && cells_[x][y] == cells_[xx][yy]) {
                  return false;
                }
              }
            }
          }
        }
      }
    }
    for (std::size_t x = 0; x < 9; ++x) {
      for (std::size_t y = 0; y < 9; ++y) {
        if (cells_[x][y] < 1 || cells_[x][y] > 9) {
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

