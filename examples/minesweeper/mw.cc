// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <cassert>
#include <ctime>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>

#include <lela/solver.h>
#include <lela/format/output.h>
#include <lela/format/syntax.h>
#include <lela/internal/maybe.h>
#include <lela/internal/iter.h>

using namespace lela;
using namespace lela::format;

class Timer {
 public:
  Timer() : start_(std::clock()) {}

  void start() {
    start_ = std::clock() - (end_ != 0 ? end_ - start_ : 0);
    ++rounds_;
  }
  void stop() { end_ = std::clock(); }
  void reset() { start_ = 0; end_ = 0; rounds_ = 0; }

  double duration() const { return (end_ - start_) / (double) CLOCKS_PER_SEC; }
  size_t rounds() const { return rounds_; }
  double avg_duration() const { return duration() / rounds_; }

 private:
  std::clock_t start_;
  std::clock_t end_ = 0;
  size_t rounds_ = 0;
};

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
  bool hit_mine() const { return hit_mine_; }
  bool all_explored() const { return n_opens() + n_mines_ == n_fields(); }

 private:
  const size_t width_;
  const size_t height_;
  const size_t n_mines_;
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

class KnowledgeBase {
 public:
  static constexpr Solver::split_level MAX_K = 2;

  explicit KnowledgeBase(const Game* g)
      : g_(g),
        ctx_(solver_.sf(), solver_.tf()),
        Bool(ctx_.NewSort()),
        XPos(ctx_.NewSort()),
        YPos(ctx_.NewSort()),
        T(ctx_.NewName(Bool)),
        F(ctx_.NewName(Bool)),
        Mine(ctx_.NewFun(Bool, 2)) {
    RegisterSort(Bool, "");
    RegisterSort(XPos, "");
    RegisterSort(YPos, "");
    RegisterSymbol(T.symbol(), "T");
    RegisterSymbol(F.symbol(), "F");
    RegisterSymbol(Mine, "Mine");
    X.resize(g_->width());
    for (size_t i = 0; i < g_->width(); ++i) {
      X[i] = ctx_.NewName(XPos);
      std::stringstream ss;
      ss << "#X" << i;
      RegisterSymbol(X[i].symbol(), ss.str());
    }
    Y.resize(g_->height());
    for (size_t i = 0; i < g_->height(); ++i) {
      Y[i] = ctx_.NewName(YPos);
      std::stringstream ss;
      ss << "#Y" << i;
      RegisterSymbol(Y[i].symbol(), ss.str());
    }
    processed_.resize(g_->n_fields(), false);
  }

  const lela::Setup& setup() { return solver_.setup(); }

  lela::internal::Maybe<bool> IsMine(Point p, Solver::split_level k) {
    t_.start();
    lela::internal::Maybe<bool> r = lela::internal::Nothing;
    Formula yes_mine = Formula::Clause(Clause{MineLit(true, p)});
    Formula no_mine = Formula::Clause(Clause{MineLit(false, p)});
    if (solver_.Entails(k, yes_mine.reader())) {
      assert(g_->mine(p));
      r = lela::internal::Just(true);
    } else if (solver_.Entails(k, no_mine.reader())) {
      assert(!g_->mine(p));
      r = lela::internal::Just(false);
    }
    t_.stop();
    return r;
  }

  void Sync() {
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!processed_[index]) {
        processed_[index] = Update(g_->to_point(index));
      }
    }
    const size_t m = g_->n_mines() - g_->n_flags();
    const size_t n = g_->n_fields() - g_->n_opens() - g_->n_flags();
    if (m < n_rem_mines_ && n < n_rem_fields_) {
      UpdateRemainingMines(m, n);
      n_rem_mines_ = m;
      n_rem_fields_ = n;
    }
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

 private:
  Literal MineLit(bool is, Point p) const {
    Term t = Mine(X[p.x], Y[p.y]);
    return is ? Literal::Eq(t, T) : Literal::Eq(t, F);
  }

  Clause MineClause(bool sign, const std::vector<Point> ns) const {
    auto r = lela::internal::transform_range(ns.begin(), ns.end(), [this, sign](Point p) { return MineLit(sign, p); });
    return Clause(r.begin(), r.end());
  }

  bool Update(Point p) {
    assert(g_->valid(p));
    const int m = g_->state(p);
    switch (m) {
      case Game::UNEXPLORED: {
        return false;
      }
      case Game::FLAGGED: {
        solver_.AddClause(Clause{MineLit(true, p)});
        return true;
      }
      case Game::HIT_MINE: {
        solver_.AddClause(Clause{MineLit(true, p)});
        return true;
      }
      default: {
        const std::vector<Point>& ns = g_->neighbors_of(p);
        const int n = ns.size();
        for (const std::vector<Point>& ps : util::Subsets(ns, n - m + 1)) {
          solver_.AddClause(MineClause(true, ps));
        }
        for (const std::vector<Point>& ps : util::Subsets(ns, m + 1)) {
          solver_.AddClause(MineClause(false, ps));
        }
        solver_.AddClause(Clause{MineLit(false, p)});
        return true;
      }
    }
  }

  void UpdateRemainingMines(size_t m, size_t n) {
    std::vector<Point> fields;
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      if (!g_->opened(index) && !g_->flagged(index)) {
        fields.push_back(g_->to_point(index));
      }
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, n - m + 1)) {
      solver_.AddClause(MineClause(true, ps));
    }
    for (const std::vector<Point>& ps : util::Subsets(fields, m + 1)) {
      solver_.AddClause(MineClause(false, ps));
    }
  }

  const Game* g_;
  Solver solver_;
  Context ctx_;

  Symbol::Sort Bool;
  Symbol::Sort XPos;
  Symbol::Sort YPos;
  HiTerm T;               // name for positive truth value
  HiTerm F;               // name for positive truth value
  std::vector<HiTerm> X;  // names for X positions
  std::vector<HiTerm> Y;  // names for Y positions
  HiSymbol Mine;

  std::vector<bool> processed_;
  size_t n_rem_mines_ = 11;
  size_t n_rem_fields_ = 11;
  Timer t_;
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

  virtual ~Printer() = default;

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
    assert(g.valid(p));
    switch (g.state(p)) {
      case Game::UNEXPLORED: return Label(g.mine(p) ? "X" : "");
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
          const lela::internal::Maybe<bool> r = kb_->IsMine(p, KnowledgeBase::MAX_K);
          if (r.yes) {
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
      if (!g_->valid(p) || g_->opened(p)) {
        std::cout << "Invalid coordinates, repeat" << std::endl;
        continue;
      }
      g_->OpenWithFrontier(p);
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

    // First open a random point which is not at the edge of the field.
    if (g_->n_opens() == 0) {
      Point p;
      do {
        p = g_->RandomPoint();
      } while (g_->neighbors_of(p).size() < 8);
      std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << " chosen at random" << std::endl;
      g_->OpenWithFrontier(p);
      return;
    }

    // First look for a field which is known not to be a mine.
    for (Solver::split_level k = 0; k <= KnowledgeBase::MAX_K; ++k) {
      for (size_t index = 0; index < g_->n_fields(); ++index) {
        const Point p = g_->to_point(index);
        if (g_->opened(p) || g_->flagged(p)) {
          continue;
        }
        const lela::internal::Maybe<bool> r = kb_->IsMine(p, k);
        if (r.yes) {
          if (r.val) {
            std::cout << "Flagging X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
            g_->Flag(p);
          } else {
            std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << " found at split level " << k << std::endl;
            g_->OpenWithFrontier(p);
          }
          return;
        }
      }
    }

    // Didn't find any reliable action, so we need to guess.
    for (size_t index = 0; index < g_->n_fields(); ++index) {
      const Point p = g_->to_point(index);
      if (g_->opened(p) || g_->flagged(p)) {
        continue;
      }
      std::cout << "Exploring X and Y coordinates: " << p.x << " " << p.y << ", which is just a guess." << std::endl;
      g_->OpenWithFrontier(p);
      return;
    }

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
  Timer t;
  Game g(width, height, n_mines, seed);
  KnowledgeBase kb(&g);
  KnowledgeBaseAgent agent(&g, &kb);
  Printer* printer;
  if (print_knowledge) {
    printer = new KnowledgeBasePrinter(&kb);
  } else {
    printer = new SimplePrinter();
  }
  t.start();
  do {
    Timer t;
    t.start();
    agent.Explore();
    t.stop();
    std::cout << std::endl;
    printer->Print(std::cout, g);
    std::cout << std::endl;
    std::cout << "Last move took " << std::fixed << t.duration() << ", queries took " << std::fixed << kb.timer().duration() << " / " << std::setw(4) << kb.timer().rounds() << " = " << std::fixed << kb.timer().avg_duration() << std::endl;
    kb.ResetTimer();
  } while (!g.hit_mine() && !g.all_explored());
  t.stop();
  std::cout << "Final board:" << std::endl;
  std::cout << std::endl;
  OmniscientPrinter().Print(std::cout, g);
  std::cout << std::endl;
  if (g.hit_mine()) {
    std::cout << Color::RED << "You loose :-(";
  } else {
    std::cout << Color::GREEN << "You win :-)";
  }
  std::cout << "  [width: " << width << ", height: " << height << ", height: " << n_mines << ", seed: " << seed << ", runtime: " << t.duration() << " seconds]" << Color::RESET << std::endl;
  delete printer;

  //std::cout << kb.setup() << std::endl;
  //int m = 0;
  //int n = 0;
  //for (Setup::Index j : kb.setup().clauses()) {
  //  for (Setup::Index i : kb.setup().clauses()) {
  //    if (i != j && kb.setup().clause(i).Subsumes(kb.setup().clause(j))) {
  //      std::cout << kb.setup().clause(j) << " subsumed by " << kb.setup().clause(i) << std::endl;
  //      ++n;
  //      break;
  //    }
  //  }
  //  ++m;
  //}
  //std::cout << n << "/" << m << " clauses in setup are subsumed" << std::endl;
  //std::cout << std::endl;
  return 0;
}

