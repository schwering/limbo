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

class Game {
 public:
  static constexpr int UNEXPLORED = -2;
  static constexpr int HIT_MINE = -1;

  Game(const size_t dimen, const size_t n_mines)
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
    const Game* self = this;
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
  static constexpr int RESET = 0;
  static constexpr int BRIGHT = 1;
  static constexpr int DIM = 2;
  static constexpr int BLACK = 30;
  static constexpr int RED = 31;
  static constexpr int GREEN = 32;

  void Print(std::ostream& os, const Game& g) {
    const int width = 3;
    os << std::setw(width) << "";
    for (size_t x = 0; x < g.dimension(); ++x) {
      os << std::setw(width) << x;
    }
    os << std::endl;
    for (size_t y = 0; y < g.dimension(); ++y) {
      os << std::setw(width) << y;
      for (size_t x = 0; x < g.dimension(); ++x) {
        std::pair<int, std::string> l = label(g, Point(x, y));
        os << "\033[" << l.first << "m" << std::setw(width) << l.second << "\033[0m";
      }
      os << std::endl;
    }
  }

  virtual std::pair<int, std::string> label(const Game&, Point) = 0;
};

class KnowledgeBase {
 public:
  explicit KnowledgeBase(const Game* g) : g_(g) {
    processed_.resize(g_->dimension() * g_->dimension(), false);
  }

  Maybe<bool> IsMine(Point p, Setup::split_level k) {
    SimpleClause yes_mine({MineLit(true, p)});
    SimpleClause no_mine({MineLit(false, p)});
    if (s_.Entails(yes_mine, k)) {
      assert(g_->mine(p));
      return Just(true);
    }
    if (s_.Entails(no_mine, k)) {
      assert(!g_->mine(p));
      return Just(false);
    }
    return Nothing;
  }

  void Sync() {
    for (size_t index = 0; index < g_->dimension() * g_->dimension(); ++index) {
      if (!processed_[index]) {
        processed_[index] = Update(g_->toPoint(index));
      }
    }
  }

  const Setup& setup() const { return s_; }

 private:
  Literal MineLit(bool is, Point p) {
    return Literal({}, is, p.x * g_->dimension() + p.y, {});
  }

  Clause MineClause(bool sign, const std::set<Point> ns) {
    SimpleClause c;
    for (const Point p : ns) {
      c.insert(MineLit(sign, p));
    }
    return Clause(Ewff::TRUE, c);
  }

  bool Update(Point p) {
    const int m = g_->state(p);
    switch (m) {
      case Game::UNEXPLORED: {
        return false;
      }
      case Game::HIT_MINE: {
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(true, p)}));
        return true;
      }
      default: {
        std::set<Point> ns = g_->NeighborsOf(p);
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

  const Game* g_;
  Setup s_;
  std::vector<bool> processed_;
};

class OmniscientPrinter : public Printer {
 public:
  std::pair<int, std::string> label(const Game& g, Point p) {
    return g.mine(p) ? std::make_pair(RED, "X") : std::make_pair(RESET, "");
  }
};

class SimplePrinter : public Printer {
 public:
  std::pair<int, std::string> label(const Game& g, Point p) override {
    switch (g.state(p)) {
      case Game::UNEXPLORED: return std::make_pair(RESET, "");
      case Game::HIT_MINE:   return std::make_pair(RED, "X");
      case 0:                 return std::make_pair(DIM, ".");
      default: {
        std::stringstream ss;
        ss << g.state(p);
        return std::make_pair(RESET, ss.str());
      }
    }
  }
};

class KnowledgeBasePrinter : public Printer {
 public:
  explicit KnowledgeBasePrinter(KnowledgeBase* kb) : kb_(kb) {}

  std::pair<int, std::string> label(const Game& g, Point p) override {
    kb_->Sync();
    switch (g.state(p)) {
      case Game::UNEXPLORED: {
        const Maybe<bool> r = kb_->IsMine(p, 2);
        if (r.succ) {
          assert(g.mine(p) == r.val);
          if (r.val) {
            return std::make_pair(RED, "X");
          } else if (!r.val) {
            return std::make_pair(GREEN, "$");
          }
        }
        return std::make_pair(RESET, "");
      }
      case Game::HIT_MINE: {
        return std::make_pair(RED, "X");
      }
      default: {
        const int m = g.state(p);
        if (m == 0) {
          return std::make_pair(DIM, ".");
        }
        std::stringstream ss;
        ss << m;
        return std::make_pair(RESET, ss.str());
      }
    }
  }

 private:
  KnowledgeBase* kb_;
};

class Agent {
 public:
  virtual Point Explore() = 0;
};

class HumanAgent : public Agent {
 public:
  Point Explore() override {
    Point p;
    std::cout << "X and Y coordinates: ";
    std::cin >> p.x >> p.y;
    return p;
  }
};

class KnowledgeBaseAgent : public Agent {
 public:
  explicit KnowledgeBaseAgent(Game* g, KnowledgeBase* kb) : g_(g), kb_(kb) {}

  Point Explore() override {
    const Setup::split_level MAX_K = 4;
    for (Setup::split_level k = 0; k < MAX_K; ++k) {
      for (size_t x = 0; x < g_->dimension(); ++x) {
        for (size_t y = 0; y < g_->dimension(); ++y) {
          const Point p(x, y);
          if (!g_->picked(p)) {
            const Maybe<bool> r = kb_->IsMine(p, k);
            if (r.succ && !r.val) {
              std::cout << "X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
              return p;
            }
          }
        }
      }
    }
    for (size_t x = 0; x < g_->dimension(); ++x) {
      for (size_t y = 0; y < g_->dimension(); ++y) {
        const Point p(x, y);
        if (!g_->picked(p)) {
          const Maybe<bool> r = kb_->IsMine(p, MAX_K);
          if (!r.succ) {
            std::cout << "X and Y coordinates: " << p.x << " " << p.y << ", which is just a guess. I should have done some more reasoning." << std::endl;
            return p;
          }
        }
      }
    }
    for (size_t x = 0; x < g_->dimension(); ++x) {
      for (size_t y = 0; y < g_->dimension(); ++y) {
        const Point p(x, y);
        if (!g_->picked(p)) {
          std::cout << "X and Y coordinates: " << p.x << " " << p.y << ", which is just a wild guess. I should have done some more reasoning." << std::endl;
          return p;
        }
      }
    }
    return Point(0, 0);
  }

 private:
  Game* g_;
  KnowledgeBase* kb_;
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
  Game g(dimen, n_mines);
  KnowledgeBase kb(&g);
  KnowledgeBaseAgent agent(&g, &kb);
  KnowledgeBasePrinter printer(&kb);
  printer.Print(std::cout, g);
  std::cout << std::endl;
  std::cout << std::endl;
  if (argv[argc - 1] == std::string("play")) {
    int s;
    do {
      Point p = agent.Explore();
      if (!g.valid(p) && g.picked(p)) {
        std::cout << "Invalid coordinates, repeat" << std::endl;
        continue;
      }
      s = g.PickWithFrontier(p);
      printer.Print(std::cout, g);
      std::cout << std::endl;
      std::cout << std::endl;
    } while (s != Game::HIT_MINE && !g.all_explored());
    OmniscientPrinter().Print(std::cout, g);
    std::cout << std::endl;
    std::cout << std::endl;
    if (s == Game::HIT_MINE) {
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

