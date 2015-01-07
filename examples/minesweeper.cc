// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <map>
#include <random>
#include <string>
#include <sstream>
#include <tuple>
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

  bool operator==(const Point& p) const {
    return x == p.x && y == p.y;
  }

  static bool Adjacent(Point p, Point q) {
    const size_t xd = std::max(p.x, q.x) - std::min(p.x, q.x);
    const size_t yd = std::max(p.y, q.y) - std::min(p.y, q.y);
    return xd <= 1 && yd <= 1;
  }

  size_t x;
  size_t y;
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
             std::set<Container>& subsets) {
  if (current.size() == n) {
    subsets.insert(current);
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
  Subsets(s.begin(), s.end(), n, {}, ss);
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
        distribution_(0, n_fields() - 1) {
    mines_.resize(n_fields(), false);
    picks_.resize(n_fields(), false);
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

  bool picked(const size_t index) const {
    return picks_[index];
  }

  bool picked(const Point p) const {
    assert(valid(p));
    return picked(to_index(p));
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

  int Pick(const Point p) {
    // Place mines at first pick
    if (n_picks() == 0) {
      for (size_t n = 0; n < n_mines_; ) {
        const Point q = RandomPoint();
        if (!mine(q) && !Point::Adjacent(p, q)) {
          set_mine(q, true);
          ++n;
        }
      }
    }

    assert(valid(p));
    assert(!picked(p));
    assert(!flagged(p));
    const size_t index = to_index(p);
    picks_[index] = true;
    frontier_[index] = false;
    for (const Point q : neighbors_of(p)) {
      assert(valid(q));
      if (!picked(q) && !flagged(q)) {
        frontier_[to_index(q)] = true;
      }
    }
    ++n_picks_;
    assert(picked(p));

    assert(valid(p));
    const int s = state(p);
    hit_mine_ |= s == HIT_MINE;
    return s;
  }

  int PickWithFrontier(Point p) {
    int s = Pick(p);
    if (s == 0) {
      for (const Point q : neighbors_of(p)) {
        if (!picked(q)) {
          PickWithFrontier(q);
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
    if (!picked(p)) {
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
    if (!picked(p)) {
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

  int unpicked_unflagged_neighbors(Point p) const {
    int n = 0;
    for (const Point q : neighbors_of(p)) {
      if (!picked(q) && !flagged(q)) {
        ++n;
      }
    }
    return n;
  }

  size_t n_picks() const { return n_picks_; }
  size_t n_flags() const { return n_flags_; }
  bool hit_mine() const { return hit_mine_; }
  bool all_explored() const { return n_picks() + n_mines_ == n_fields(); }

 private:
  const size_t width_;
  const size_t height_;
  const size_t n_mines_;
  size_t n_picks_ = 0;
  size_t n_flags_ = 0;
  bool hit_mine_ = false;
  std::vector<bool> mines_;
  std::vector<bool> picks_;
  std::vector<bool> flags_;
  std::vector<bool> frontier_;
  mutable std::vector<std::vector<Point>> neighbors_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<size_t> distribution_;
};

class KnowledgeBase {
 public:
  static constexpr Setup::split_level MAX_K = 2;

  explicit KnowledgeBase(const Game* g) : g_(g) {
    processed_.resize(g_->n_fields(), false);
    //cache_.resize(g_->n_fields(), Nothing);
  }

  Maybe<bool> IsMine(Point p, Setup::split_level k) {
    //const size_t index = g_->to_index(p);
    //Maybe<bool>& r = cache_[index];
    //if (!r.succ) {
      Maybe<bool> r = Nothing;
      SimpleClause yes_mine({MineLit(true, p)});
      SimpleClause no_mine({MineLit(false, p)});
      if (s_.Entails(yes_mine, k)) {
        assert(g_->mine(p));
        r = Just(true);
      } else if (s_.Entails(no_mine, k)) {
        assert(!g_->mine(p));
        r = Just(false);
      }
    //}
    return r;
  }

  void Sync() {
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!processed_[index]) {
        processed_[index] = Update(g_->to_point(index));
      }
    }
    const size_t m = g_->n_mines() - g_->n_flags();
    const size_t n = g_->n_fields() - g_->n_picks() - g_->n_flags();
    if (m < n_rem_mines_ && n < n_rem_fields_) {
      UpdateRemainingMines(m, n);
      n_rem_mines_ = m;
      n_rem_fields_ = n;
    }
  }

  const Setup& setup() const { return s_; }

 private:
  Literal MineLit(bool is, Point p) {
    return Literal({}, is, p.x * g_->width() + p.y, {});
  }

  Clause MineClause(bool sign, const std::vector<Point> ns) {
    SimpleClause c;
    for (const Point p : ns) {
      c.insert(MineLit(sign, p));
    }
    return Clause(Ewff::TRUE, c);
  }

  bool Update(Point p) {
    assert(g_->valid(p));
    const int m = g_->state(p);
    switch (m) {
      case Game::UNEXPLORED: {
        return false;
      }
      case Game::FLAGGED: {
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(true, p)}));
        return true;
      }
      case Game::HIT_MINE: {
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(true, p)}));
        return true;
      }
      default: {
        const std::vector<Point>& ns = g_->neighbors_of(p);
        const int n = ns.size();
        //std::cerr << "n = " << n << ", m = " << m << std::endl;
        for (const std::vector<Point>& ps : util::Subsets(ns, n - m + 1)) {
          s_.AddClause(MineClause(true, ps));
        }
        for (const std::vector<Point>& ps : util::Subsets(ns, m + 1)) {
          s_.AddClause(MineClause(false, ps));
        }
        s_.AddClause(Clause(Ewff::TRUE, {MineLit(false, p)}));
        return true;
      }
    }
  }

  void UpdateRemainingMines(size_t m, size_t n) {
    std::vector<Point> fields;
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!g_->picked(index) && !g_->flagged(index)) {
        fields.push_back(g_->to_point(index));
      }
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, n - m + 1)) {
      s_.AddClause(MineClause(true, ps));
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, m + 1)) {
      s_.AddClause(MineClause(false, ps));
    }
  }

  const Game* g_;
  Setup s_;
  std::vector<bool> processed_;
  size_t n_rem_mines_ = 4;
  size_t n_rem_fields_ = 10;
  //std::vector<Maybe<bool>> cache_;
};

class Color {
 public:
  static const Color RESET;
  static const Color BRIGHT;
  static const Color DIM;
  static const Color UNDERSCORE;
  static const Color BLINK;
  static const Color REVERSE;
  static const Color BLACK;
  static const Color RED;
  static const Color GREEN;

  Color() : code("") {}
  explicit Color(const std::string& c) : code(c) {}

  Color operator|(const Color& c) const {
    return Color(code +";"+ c.code);
  };

  std::string str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

 private:
  friend std::ostream& operator<<(std::ostream& os, const Color& c);

  std::string code;
};

const Color Color::RESET("0");
const Color Color::BRIGHT("1");
const Color Color::DIM("2");
const Color Color::UNDERSCORE("4");
const Color Color::BLINK("5");
const Color Color::REVERSE("7");
const Color Color::BLACK("30");
const Color Color::RED("31");
const Color Color::GREEN("32");

std::ostream& operator<<(std::ostream& os, const Color& c) {
  return os << "\033[" << (c.code.size() > 0 ? c.code : "0") << "m";
}

class Printer {
 public:
  struct Label {
    Label(const std::string& s) : Label(Color::RESET, s) {}
    Label(const Color& c, const std::string& s) : color(c), text(s) {}

    Color color;
    std::string text;
  };

  void Print(std::ostream& os, const Game& g) {
    const int width = 3;
    os << std::setw(width) << "";
    for (size_t x = 0; x < g.width(); ++x) {
      os << Color::DIM << std::setw(width) << x << Color::RESET;
    }
    os << std::endl;
    for (size_t y = 0; y < g.height(); ++y) {
      os << Color::DIM << std::setw(width) << y << Color::RESET;
      for (size_t x = 0; x < g.width(); ++x) {
        Label l = label(g, Point(x, y));
        os << l.color << std::setw(width) << l.text << Color::RESET;
      }
      os << std::endl;
    }
  }

  virtual Label label(const Game&, Point) = 0;
};

class OmniscientPrinter : public Printer {
 public:
  Label label(const Game& g, const Point p) {
    return g.mine(p) ? Label(Color::RED, "X") : Label("");
  }
};

class SimplePrinter : public Printer {
 public:
  Label label(const Game& g, const Point p) override {
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: return Label("");
      case Game::FLAGGED:    return Label(Color::GREEN, "X");
      case Game::HIT_MINE:   return Label(Color::RED, "X");
      case 0:                return Label(".");
      default: {
        std::stringstream ss;
        ss << g.state(p);
        return Label(ss.str());
      }
    }
  }
};

class KnowledgeBasePrinter : public Printer {
 public:
  explicit KnowledgeBasePrinter(KnowledgeBase* kb) : kb_(kb) {}

  Label label(const Game& g, const Point p) override {
    kb_->Sync();
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: {
        if (g.frontier(p)) {
          const Maybe<bool> r = kb_->IsMine(p, KnowledgeBase::MAX_K);
          if (r.succ) {
            assert(g.mine(p) == r.val);
            if (r.val) {
              return Label(Color::RED | Color::BLINK, "X");
            } else if (!r.val) {
              return Label(Color::GREEN | Color::BLINK, "O");
            }
          }
        }
        return Label("");
      }
      case Game::FLAGGED: {
        return Label(Color::GREEN, "X");
      }
      case Game::HIT_MINE: {
        return Label(Color::RED, "X");
      }
      default: {
        const int m = g.state(p);
        if (m == 0) {
          return Label(".");
        }
        std::stringstream ss;
        ss << m;
        return Label(ss.str());
      }
    }
  }

 private:
  KnowledgeBase* kb_;
};

class Agent {
 public:
  virtual void Explore() = 0;
};

class HumanAgent : public Agent {
 public:
  explicit HumanAgent(Game* g) : g_(g) {}

  void Explore() override {
    for (;;) {
      Point p;
      std::cout << "Exploring X and Y coordinates: ";
      std::cin >> p.x >> p.y;
      if (!g_->valid(p) || g_->picked(p)) {
        std::cout << "Invalid coordinates, repeat" << std::endl;
        continue;
      }
      g_->PickWithFrontier(p);
      return;
    }
  }

 private:
  Game* g_;
};

class KnowledgeBaseAgent : public Agent {
 public:
  explicit KnowledgeBaseAgent(Game* g, KnowledgeBase* kb) : g_(g), kb_(kb) {}

  void Explore() override {
    kb_->Sync();

    // First pick a random point which is not at the edge of the field.
    if (g_->n_picks() == 0) {
      Point p;
      do {
        p = g_->RandomPoint();
      } while (g_->neighbors_of(p).size() < 8);
      std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << " chosen at random" << std::endl;
      g_->PickWithFrontier(p);
      return;
    }

#if 1
    std::vector<std::tuple<int, int, Point>> ps;
    ps.reserve(g_->n_fields() - g_->n_picks() - g_->n_flags());
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      const Point p(g_->to_point(index));
      if (!g_->picked(p) && !g_->flagged(p)) {
        int min_nb_possible_mines = 8;
        int min_nb_unknowns = 8;
        for (const Point q : g_->neighbors_of(p)) {
          if (g_->picked(q) && !g_->flagged(q)) {
            const int nb_possible_mines = g_->state_minus_flags(q);
            const int nb_unknowns = g_->unpicked_unflagged_neighbors(q);
            if (std::tie(nb_possible_mines, nb_unknowns) <
                std::tie(min_nb_possible_mines, min_nb_unknowns)) {
              min_nb_possible_mines = nb_possible_mines;
              min_nb_unknowns = nb_unknowns;
            }
          }
        }
        ps.push_back(std::make_tuple(min_nb_possible_mines, min_nb_unknowns, p));
      }
    }
    std::sort(ps.begin(), ps.end());

    // First look for a field which is known not to be a mine.
    for (Setup::split_level k = 0; k <= KnowledgeBase::MAX_K; ++k) {
      for (const auto tuple : MakeRange(ps.begin(), ps.end())) {
        const Point p = std::get<2>(tuple);
        const Maybe<bool> r = kb_->IsMine(p, k);
        if (r.succ) {
          if (r.val) {
            std::cout << "Flagging X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << " (" << std::get<0>(tuple) << ", " << std::get<1>(tuple) << ")" << std::endl;
            g_->Flag(p);
          } else {
            std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << " (" << std::get<0>(tuple) << ", " << std::get<1>(tuple) << ")" << std::endl;
            g_->PickWithFrontier(p);
          }
          return;
        }
      }
    }

    // Otherwise look for a non-frontier field which is we don't know anything about.
    for (const auto tuple : MakeRange(ps.rbegin(), ps.rend())) {
      const Point p = std::get<2>(tuple);
      const Maybe<bool> r = kb_->IsMine(p, KnowledgeBase::MAX_K);
      if (r.succ) {
        if (r.val) {
          std::cout << "Flagging X and Y coordinates: " << p.x << " " << p.y << ", which is a not at the frontier, found at split level " << KnowledgeBase::MAX_K << std::endl;
          g_->Flag(p);
        } else {
          std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is a not at the frontier, found at split level " << KnowledgeBase::MAX_K << std::endl;
          g_->PickWithFrontier(p);
        }
        return;
      } else {
        std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is just a (non-frontier) guess. I should have done some more reasoning." << std::endl;
        g_->PickWithFrontier(p);
        return;
      }
    }

#else

    // First look for a field which is known not to be a mine.
    for (Setup::split_level k = 0; k <= KnowledgeBase::MAX_K; ++k) {
      for (size_t index = 0; index < g_->n_fields(); ++index) {
        if (g_->frontier(index)) {
          const Point p = g_->to_point(index);
          const Maybe<bool> r = kb_->IsMine(p, k);
          if (r.succ) {
            if (r.val) {
              std::cout << "Flagging X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
              g_->Flag(p);
            } else {
              std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
              g_->PickWithFrontier(p);
            }
            return;
          }
        }
      }
    }

    // Otherwise look for a non-frontier field which is we don't know anything about.
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!g_->frontier(index) && !g_->picked(index) && !g_->flagged(index)) {
        const Point p = g_->to_point(index);
        const Maybe<bool> r = kb_->IsMine(p, KnowledgeBase::MAX_K);
        if (r.succ) {
          if (r.val) {
            std::cout << "Flagging X and Y coordinates: " << p.x << " " << p.y << ", which is a not at the frontier, found at split level " << KnowledgeBase::MAX_K << std::endl;
            g_->Flag(p);
          } else {
            std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is a not at the frontier, found at split level " << KnowledgeBase::MAX_K << std::endl;
            g_->PickWithFrontier(p);
          }
          return;
        } else {
          std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is just a (non-frontier) guess. I should have done some more reasoning." << std::endl;
          g_->PickWithFrontier(p);
          return;
        }
      }
    }

    // Otherwise look for a frontier field which is we don't know anything about.
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (g_->frontier(index)) {
        const Point p = g_->to_point(index);
        const Maybe<bool> r = kb_->IsMine(p, KnowledgeBase::MAX_K);
        if (!r.succ) {
          std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is just a (frontier) guess. I should have done some more reasoning." << std::endl;
          g_->PickWithFrontier(p);
          return;
        }
      }
    }
#endif


    // That's weird, this case should never occur, because our reasoning should
    // be correct.
    std::cout << __FILE__ << ":" << __LINE__ << ": Why didn't we choose a point?" << std::endl;
    assert(false);
  }

 private:
  Game* g_;
  KnowledgeBase* kb_;
};

int main(int argc, char *argv[]) {

  size_t width = 9;
  size_t height = 9;
  size_t n_mines = 10;
  size_t seed = 0;
  bool print_knowledge = false;
  if (argc >= 2) {
    width = atoi(argv[1]);
  }
  if (argc >= 3) {
    height = atoi(argv[2]);
  }
  n_mines = (width + 1) * (height + 1) / 10;
  if (argc >= 4) {
    n_mines = atoi(argv[3]);
  }
  if (argc >= 5) {
    seed = atoi(argv[4]);
  }
  if (argc >= 6) {
    print_knowledge = argv[5] == std::string("know");
  }
  const std::clock_t start = std::clock();
  Game g(width, height, n_mines, seed);
  KnowledgeBase kb(&g);
  KnowledgeBaseAgent agent(&g, &kb);
  Printer* printer;
  if (print_knowledge) {
    printer = new KnowledgeBasePrinter(&kb);
  } else {
    printer = new SimplePrinter();
  }
  do {
    agent.Explore();
    std::cout << std::endl;
    printer->Print(std::cout, g);
    std::cout << std::endl;
  } while (!g.hit_mine() && !g.all_explored());
  OmniscientPrinter().Print(std::cout, g);
  std::cout << std::endl;
  if (g.hit_mine()) {
    std::cout << Color::RED << "You loose :-(";
  } else {
    std::cout << Color::GREEN << "You win :-)";
  }
  const double duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
  std::cout << "  [width: " << width << ", height: " << height << ", height: " << n_mines << ", seed: " << seed << ", runtime: " << duration << " seconds]" << Color::RESET << std::endl;
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

