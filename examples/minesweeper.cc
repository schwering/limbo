// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <cassert>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <./formula.h>
#include <./maybe.h>

using namespace lela;

struct Point {
  Point() {}
  Point(size_t x, size_t y) : x(x), y(y) {}

  bool operator<(const Point& p) const {
    return x < p.x || (x == p.x && y < p.y);
  }

  std::set<Point> Neighbors() const {
    std::set<Point> s;
    s.insert(Point(x-1, y-1));
    s.insert(Point(x-1, y  ));
    s.insert(Point(x-1, y+1));
    s.insert(Point(x  , y-1));
    s.insert(Point(x  , y+1));
    s.insert(Point(x+1, y-1));
    s.insert(Point(x+1, y  ));
    s.insert(Point(x+1, y+1));
    return s;
  }

  size_t x;
  size_t y;
};

namespace util {

template<class T>
void Subsets(const typename std::set<T>::const_iterator first,
             const typename std::set<T>::const_iterator last,
             const size_t n,
             const std::set<T>& current,
             std::set<std::set<T>>& subsets) {
  if (current.size() == n) {
    subsets.insert(current);
    return;
  }
  if (first == last ||
      current.size() + static_cast<ssize_t>(std::distance(first, last)) < n) {
    return;
  }
  Subsets(std::next(first), last, n, current, subsets);
  std::set<T> current1 = current;
  current1.insert(*first);
  Subsets(std::next(first), last, n, current1, subsets);
}

template<class T>
std::set<std::set<T>> Subsets(const std::set<T>& s, size_t n) {
  std::set<std::set<T>> ss;
  Subsets(s.begin(), s.end(), n, {}, ss);
  return ss;
}

}  // namespace util

class Field {
 public:
  static constexpr int UNEXPLORED = -2;
  static constexpr int HIT_MINE = -1;

  Field(const size_t dimen, const size_t n_mines)
      : dimen_(dimen),
        distribution_(0, dimen_ - 1) {
    mines_.resize(dimen_ * dimen_, false);
    picks_.resize(dimen_ * dimen_, false);
    assert(mines_.size() == dimen_ * dimen_);
    assert(n_mines <= dimen_ * dimen_);
    for (n_mines_ = 0; n_mines_ < n_mines; ) {
      Point p;
      p.x = distribution_(generator_);
      p.y = distribution_(generator_);
      if (!mine(p)) {
        set_mine(p, true);
        ++n_mines_;
      }
    }
    assert(n_mines == n_mines_);
  }

  size_t dimension() const { return dimen_; }
  size_t n_mines() const { return n_mines_; }

  std::set<Point> NeighborsOf(Point p) const {
    return FilterNeighborsOf(p, [](const Point& p) { return true; });
  }

  template<class UnaryPredicate>
  std::set<Point> FilterNeighborsOf(Point p, UnaryPredicate pred) const {
    std::set<Point> s = p.Neighbors();
    for (auto it = s.begin(); it != s.end(); ) {
      if (!valid(*it) || !pred(*it)) {
        it = s.erase(it);
      } else {
        ++it;
      }
    }
    return s;
  }

  Point toPoint(size_t index) const {
    Point p(index / dimen_, index % dimen_);
    assert(dimen_ * p.x + p.y == index);
    return p;
  }

  bool valid(Point p) const {
    return p.x < dimen_ && p.y < dimen_;
  }

  void set_mine(Point p, bool is_mine) {
    assert(valid(p));
    mines_[dimen_ * p.x + p.y] = is_mine;
  }

  bool mine(Point p) const {
    assert(valid(p));
    return mines_[dimen_ * p.x + p.y];
  }

  bool picked(Point p) const {
    assert(valid(p));
    return picks_[dimen_ * p.x + p.y];
  }

  int PickRandom() {
    Point p;
    p.x = distribution_(generator_);
    p.y = distribution_(generator_);
    return Pick(p);
  }

  int Pick(Point p) {
    assert(!picked(p));
    picks_[dimen_ * p.x + p.y] = true;
    assert(picked(p));
    return state(p);
  }

  int PickWithFrontier(Point p) {
    int s = Pick(p);
    if (s == 0) {
      for (const Point q : NeighborsOf(p)) {
        if (!picked(q)) {
          PickWithFrontier(q);
        }
      }
    }
    return s;
  }

  int state(Point p) const {
    if (!picked(p)) {
      return UNEXPLORED;
    }
    if (mine(p)) {
      return HIT_MINE;
    }
    const Field* self = this;
    return FilterNeighborsOf(p, [self](const Point& q) { return self->mine(q); }).size();
  }

  bool all_explored() const {
    for (size_t x = 0; x < dimen_; ++x) {
      for (size_t y = 0; y < dimen_; ++y) {
        if (!picked(Point(x, y)) && !mine(Point(x, y))) {
          return false;
        }
      }
    }
    return true;
  }

 private:
  std::vector<bool> mines_;
  std::vector<bool> picks_;
  size_t dimen_;
  size_t n_mines_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<size_t> distribution_;
};

class Printer {
 public:
  void Print(std::ostream& os, const Field& f) {
    const int width = 3;
    os << std::setw(width) << "";
    for (size_t x = 0; x < f.dimension(); ++x) {
      os << std::setw(width) << x;
    }
    os << std::endl;
    for (size_t y = 0; y < f.dimension(); ++y) {
      os << std::setw(width) << y;
      for (size_t x = 0; x < f.dimension(); ++x) {
        os << std::setw(width) << label(f, Point(x, y));
      }
      os << std::endl;
    }
  }

  virtual std::string label(const Field& f, Point p) = 0;
};

class OmniscientPrinter : public Printer {
 public:
  std::string label(const Field& f, Point p) {
    return f.mine(p) ? "X" : "";
  }
};

class SimplePrinter : public Printer {
 public:
  std::string label(const Field& f, Point p) override {
    switch (f.state(p)) {
      case Field::UNEXPLORED: return "";
      case Field::HIT_MINE:   return "X";
      case 0:                 return ".";
      default: {
        std::stringstream ss;
        ss << f.state(p);
        return ss.str();
      }
    }
  }
};

class SetupPrinter : public Printer {
 public:
  SetupPrinter(const Field* f) : f_(f) {
    processed_.resize(f_->dimension() * f_->dimension(), false);
  }

  Literal MineLit(bool is, Point p) {
    return Literal({}, is, p.x * f_->dimension() + p.y, {});
  }

  Clause MineClause(bool sign, const std::set<Point> ns) {
    SimpleClause c;
    for (const Point p : ns) {
      c.insert(MineLit(sign, p));
    }
    return Clause(Ewff::TRUE, c);
  }

  std::string label(const Field& f, Point p) override {
    assert(&f == f_);
    UpdateKb();
    switch (f.state(p)) {
      case Field::UNEXPLORED: {
        SimpleClause yes_mine({MineLit(true, p)});
        SimpleClause no_mine({MineLit(false, p)});
        for (Setup::split_level k = 0; k < 3; ++k) {
          if (s_.Entails(yes_mine, k)) {
            assert(f_->mine(p));
            return "X";
          } else if (s_.Entails(no_mine, k)) {
            assert(!f_->mine(p));
            return "$";
          }
        }
        return "";
      }
      case Field::HIT_MINE: {
        return "X";
      }
      default: {
        const int m = f.state(p);
        if (m == 0) {
          return ".";
        }
        std::stringstream ss;
        ss << m;
        return ss.str();
      }
    }
  }

  const Setup& setup() const { return s_; }

 private:
  void UpdateKb() {
    for (size_t index = 0; index < f_->dimension() * f_->dimension(); ++index) {
      if (!processed_[index]) {
        processed_[index] = UpdateKb(f_->toPoint(index));
      }
    }
  }

  bool UpdateKb(Point p) {
    const int m = f_->state(p);
    switch (m) {
      case Field::UNEXPLORED: {
        return false;
      }
      case Field::HIT_MINE: {
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(true, p)}));
        return true;
      }
      default: {
        std::set<Point> ns = f_->NeighborsOf(p);
        const int n = ns.size();
        //std::cerr << "n = " << n << ", m = " << m << std::endl;
        for (const std::set<Point>& ps : util::Subsets(ns, n - m + 1)) {
          s_.AddClause(MineClause(true, ps));
        }
        for (const std::set<Point>& ps : util::Subsets(ns, m + 1)) {
          s_.AddClause(MineClause(false, ps));
        }
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(false, p)}));
        return true;
      }
    }
  }

  const Field* f_;
  Setup s_;
  std::vector<bool> processed_;
};

int main(int argc, char *argv[]) {
  size_t dimen = 9;
  size_t n_mines = 10;
  if (argc >= 2) {
    dimen = atoi(argv[1]);
    n_mines = dimen + 1;
  }
  if (argc >= 3) {
    n_mines = atoi(argv[2]);
  }
  Field f(dimen, n_mines);
  SetupPrinter printer(&f);
  printer.Print(std::cout, f);
  std::cout << std::endl;
  std::cout << std::endl;
  if (argv[argc - 1] == std::string("play")) {
    int s;
    do {
      Point p;
      std::cout << "X and Y coordinates: ";
      std::cin >> p.x >> p.y;
      if (!f.valid(p) && f.picked(p)) {
        std::cout << "Invalid coordinates, repeat" << std::endl;
        continue;
      }
      s = f.PickWithFrontier(p);
      printer.Print(std::cout, f);
      std::cout << std::endl;
      std::cout << std::endl;
    } while (s != Field::HIT_MINE && !f.all_explored());
    OmniscientPrinter().Print(std::cout, f);
    std::cout << std::endl;
    std::cout << std::endl;
    if (s == Field::HIT_MINE) {
      std::cout << "You loose :-(" << std::endl;
    } else {
      std::cout << "You win :-)" << std::endl;
    }
  }
  return 0;
}

#if 0
class MwBat : public Bat {
 public:
  virtual Maybe<Formula::ObjPtr> RegressOneStep(const Atom& a) = 0;

  void GuaranteeConsistency(split_level k) override {
    s_.GuaranteeConsistency(k);
  }

  void AddClause(const Clause& c) override {
    s_.AddClause(c);
    names_init_ = false;
  }

  bool InconsistentAt(belief_level p, split_level k) const override {
    assert(p == 0);
    return s_.Inconsistent(k);
  }

  bool EntailsClauseAt(belief_level p,
                       const SimpleClause& c,
                       split_level k) const override {
    assert(p == 0);
    return s_.Entails(c, k);
  }

  size_t n_levels() const override { return 1; }

  const StdName::SortedSet& names() const override {
    if (!names_init_) {
      names_ = s_.hplus().WithoutPlaceholders();
      names_init_ = true;
    }
    return names_;
  }

 private:
  Setup s_;
  mutable StdName::SortedSet names_;
  mutable bool names_init_ = false;
};
#endif

