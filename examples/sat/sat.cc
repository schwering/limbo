// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2018 Christoph Schwering
//
// A SAT solver using Limbo data structures. The purpose of this file is
// to find reasons why Limbo is so slow.

#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <type_traits>

#include <limbo/clause.h>
#include <limbo/lit.h>

//#include "sat.h"
#include <limbo/sat.h>

using namespace limbo;
using namespace limbo::internal;

std::ostream& operator<<(std::ostream& os, const Fun f) {
  os << int(f);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Name n) {
  os << int(n);
  return os;
}

std::ostream& operator<<(std::ostream& os, const Lit a) {
  os << a.fun() << ' ' << (a.pos() ? "\u003D" : "\u2260") << ' ' << a.name();
  return os;
}

std::ostream& operator<<(std::ostream& os, const Clause& c) {
  os << '(';
  bool first = true;
  for (const Lit a : c) {
    if (!first) {
      os << " \u2228 ";
    }
    os << a;
    first = false;
  }
  os << ')';
  return os;
}

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
  std::size_t rounds() const { return rounds_; }
  double avg_duration() const { return duration() / rounds_; }

 private:
  std::clock_t start_;
  std::clock_t end_ = 0;
  std::size_t rounds_ = 0;
};

template<typename UnaryFunction, typename T = int>
static void CreateTerms(UnaryFunction f, int n, std::vector<typename std::result_of<UnaryFunction(T)>::type>* terms) {
  terms->clear();
  terms->reserve(n);
  for (auto i = terms->size(); i < n; ++i) {
    const auto t = f(i + 1);
    terms->push_back(t);
    std::ostringstream s;
    s << (i + 1);
  }
}

static bool LoadCnf(std::istream& stream,
                    std::vector<std::vector<Lit>>* cnf,
                    std::vector<Fun>* funs,
                    std::vector<Name>* names,
                    Name* extra_name) {
  Name T;
  Name F;
  cnf->clear();
  bool prop = false;
  for (std::string line; std::getline(stream, line); ) {
    int n_funs;
    int n_names;
    int n_clauses;
    if (line.length() == 0) {
      continue;
    } else if (line.length() >= 1 && line[0] == 'c') {
      // ignore comment
    } else if (sscanf(line.c_str(), "p cnf %d %d", &n_funs, &n_clauses) == 2) {  // propositional CNF
      funs->clear();
      names->clear();
      CreateTerms([](int i) { return Fun::FromId(i); }, n_funs, funs);
      F = Name::FromId(1);
      T = Name::FromId(2);
      names->push_back(T);
      names->push_back(F);
      *extra_name = T;
      prop = true;
    } else if (sscanf(line.c_str(), "p fcnf %d %d %d", &n_funs, &n_names, &n_clauses) == 3) {  // func CNF
      funs->clear();
      names->clear();
      CreateTerms([](int i) { return Fun::FromId(i); }, n_funs, funs);
      CreateTerms([](int i) { return Name::FromId(i); }, n_names + 1, names);
      *extra_name = names->back();
      prop = false;
    } else if (prop) {  // propositional clause
      std::vector<Lit> lits;
      int i = -1;
      for (std::istringstream iss(line); (iss >> i) && i != 0; ) {
        const Fun f = (*funs)[(i < 0 ? -i : i) - 1];
#if 0
        const Lit a = i < 0 ? Lit::Eq(f, F) : Lit::Eq(f, T);
#else
        const Lit a = i < 0 ? Lit::Eq(f, F) : Lit::Neq(f, F);
#endif
        lits.push_back(a);
      }
      if (i == 0) {
        cnf->push_back(lits);
      }
    } else {  // functional clause
      std::vector<Lit> lits;
      int i;
      int j;
      char eq = '\0';
      for (std::istringstream iss(line); (iss >> i >> eq >> j) && eq == '='; ) {
        assert(j >= 1);
        const Fun f = (*funs)[(i < 0 ? -i : i) - 1];
        const Name n = (*names)[j - 1];
        const Lit a = i < 0 ? Lit::Neq(f, n) : Lit::Eq(f, n);
        lits.push_back(a);
      }
      if (eq == '=') {
        cnf->push_back(lits);
      } else {
        std::cerr << "Parse error: '" << line << "'" << std::endl;
      }
    }
  }
  return prop;
}

bool Solve(Sat* solver, int n_conflicts_init, int conflicts_increase) {
  struct Stats {
    int conflicts = 0;
    int conflicts_level_sum = 0;
    int conflicts_btlevel_sum = 0;
    int decisions = 0;
    int decisions_level_sum = 0;
  } stats;
  bool restarts = n_conflicts_init >= 0;
  int n_conflicts = n_conflicts_init;
  int sat = 0;

  auto conflict_predicate = [&](auto level, auto conflict, const std::vector<Lit>& learnt, auto btlevel) {
    ++stats.conflicts;
    stats.conflicts_level_sum += int(level);
    stats.conflicts_btlevel_sum += int(btlevel);
    //std::cout << "Learnt from " << solver.clause(conflict) << " at level " << level << " that " << learnt << ", backtracking to " << btlevel << std::endl;
    return !restarts || stats.conflicts < n_conflicts;
  };
  auto decision_predicate = [&](auto level, Lit a) {
    ++stats.decisions;
    stats.decisions_level_sum += int(level);
    //std::cout << "Decided " << a << " at level " << level << std::endl;
    return true;
  };

  Timer t;
  t.start();
  for (int i = 0; sat == 0; ++i) {
    n_conflicts = static_cast<int>(std::pow(conflicts_increase, i) * n_conflicts_init);
    sat = solver->Solve(conflict_predicate, decision_predicate);
  }
  t.stop();
  printf("%s (in %.5lfs)\n", (sat > 0 ? "SATISFIABLE" : "UNSATISFIABLE"), t.duration());
  printf("Conflicts: %d (at average level %lf to average level %lf) | Decisions: %d (at average level %lf)\n",
         stats.conflicts,
         (static_cast<double>(stats.conflicts_level_sum)/static_cast<double>(stats.conflicts)),
         (static_cast<double>(stats.conflicts_btlevel_sum)/static_cast<double>(stats.conflicts)),
         stats.decisions,
         (static_cast<double>(stats.decisions_level_sum)/static_cast<double>(stats.decisions)));
  return sat > 0;
}

void PrintSolution(const Sat& solver, const bool prop, const int n_columns,
                   const std::vector<Fun>& funs, const std::vector<Name>& names,
                   const bool extra, const Name extra_name) {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  const int lit_width = 10;
  const int win_width = (n_columns != 0 ? n_columns : static_cast<int>(std::ceil(std::sqrt(funs.size())))) * lit_width;
  int i = 0;
  if (!prop) {
    for (const Fun f : funs) {
      const Name n = solver.model()[f];
      if (!extra && n == extra_name) {
        continue;
      }
      std::stringstream fss; fss << f;
      std::stringstream ess; ess << " = ";
      std::stringstream nss; nss << n;
      const std::string fs = fss.str();
      const std::string es = ess.str();
      const std::string ns = nss.str();
      const int ews = es.length();
      const int fws = std::max(1, (lit_width - ews - 1)/2 - static_cast<int>(fs.length()));
      const int nws = std::max(1, (lit_width - ews - 1)/2 + (lit_width - ews - 1)%2 - static_cast<int>(ns.length()));
      std::cout << std::string(fws, ' ') << fs << es << ns << std::string(nws, ' ');
      if (++i % (win_width / lit_width) == 0) {
        std::cout << std::endl;
      }
    }
  } else {
    for (const Fun f : funs) {
      const Name n = solver.model()[f];
      if (!extra && n == extra_name) {
        continue;
      }
      std::cout << (n != extra_name ? "-" : "") << int(f) << ' ';
    }
    std::cout << '0';
  }
  std::cout << std::endl;
}

int main(int argc, char *argv[]) {
  std::srand(0);
  std::vector<std::vector<Lit>> cnf;
  std::vector<Fun> funs;
  std::vector<Name> names;
  Name extra_name;
  int n_models = 1;
  int n_iterations = 1;
  int n_columns = 0;
  int n_conflicts_before_restart = -1;
  int loaded = 0;
  int extra = true;
  bool prop = false;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: " << argv[0] << " [options] [file]" << std::endl;
      std::cout << std::endl;
      std::cout << "If file is not specified, input is read from stdin." << std::endl;
      std::cout << "Input must be in DIMACS CNF format or the functional extension thereof." << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "--columns=int    -c=int  columns in output, e.g, 9 for sudoku (default: " << n_columns << ")" << std::endl;
      std::cout << "--extra=bool     -e=bool whether extra name is added (default: " << extra << ")" << std::endl;
      std::cout << "--iterations=int -i=int  repretitions with clauses learnt so far (default: " << n_iterations << ")" << std::endl;
      std::cout << "--models=int     -n=int  how many models to find (default: " << n_models << ", infinity: -1)" << std::endl;
      std::cout << "--restart=int    -r=int  conflicts before restart, (default: " << n_conflicts_before_restart << ", infinity: -1)" << std::endl;
      std::cout << std::endl;
#ifndef NDEBUG
      std::cout << "Debugging is turned on (NDEBUG is not defined)." << std::endl;
#else
      std::cout << "Debugging is turned off (NDEBUG is defined)." << std::endl;
#endif
      return 1;
    } else if (sscanf(argv[i], "--columns=%d", &n_columns) == 1 || sscanf(argv[i], "-c=%d", &n_columns) == 1) {
    } else if (sscanf(argv[i], "--extra=%d", &extra) == 1 || sscanf(argv[i], "-e=%d", &extra) == 1) {
    } else if (sscanf(argv[i], "--iterations=%d", &n_iterations) == 1 || sscanf(argv[i], "-i=%d", &n_iterations) == 1) {
    } else if (sscanf(argv[i], "--models=%d", &n_models) == 1 || sscanf(argv[i], "-n=%d", &n_models) == 1) {
    } else if (sscanf(argv[i], "--restart=%d", &n_conflicts_before_restart) == 1 || sscanf(argv[i], "-r=%d", &n_conflicts_before_restart) == 1) {
    } else if (loaded == 0 && argv[i][0] != '-') {
      std::ifstream ifs = std::ifstream(argv[i]);
      prop = LoadCnf(ifs, &cnf, &funs, &names, &extra_name);
      ++loaded;
    } else {
      std::cout << "Cannot load '"<< argv[i] << "'" << std::endl;
      return 2;
    }
  }
  if (loaded == 0) {
    prop = LoadCnf(std::cin, &cnf, &funs, &names, &extra_name);
    ++loaded;
  }
  assert(loaded > 0);
  assert(!extra_name.null());

  Timer timer_total;
  timer_total.start();
  Sat solver;
  auto extra_name_factory = [extra_name](const Fun) { return extra_name; };
  for (const std::vector<Lit>& lits : cnf) {
    solver.AddClause(lits, extra_name_factory);
  }
  solver.Init();
  for (int i_iterations = 1; i_iterations <= n_iterations; ++i_iterations) {
    solver.Simplify();
    int i_models;
    for (i_models = 0; i_models < n_models || n_models < 0; ++i_models) {
      const bool sat = Solve(&solver, n_conflicts_before_restart, 2);
      if (!sat) {
        break;
      }
      if (n_columns >= 0) {
        PrintSolution(solver, prop, n_columns, funs, names, extra, extra_name);
      }
      std::vector<Lit> lits;
      for (const Fun f : funs) {
        const Name n = solver.model()[f];
        lits.push_back(Lit::Neq(f, n));
      }
      solver.AddClause(lits, extra_name_factory);
    }
    if (n_models != 1) {
      std::cout << "Found " << i_models << " models" << std::endl;
    }
  }
  timer_total.stop();
  if (timer_total.rounds() > 1) {
    std::cout << "Total took " << timer_total.duration() << " seconds" << std::endl;
  }
  return 0;
}

