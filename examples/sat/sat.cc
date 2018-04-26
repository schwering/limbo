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

#include <limbo/literal.h>
#include <limbo/term.h>

#include <limbo/internal/intmap.h>

#include <limbo/format/output.h>

#include "clause.h"
#include "solver.h"

using namespace limbo;
using namespace limbo::internal;

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

template<typename NullaryPredicate>
static void CreateTerms(NullaryPredicate p, size_t n, std::vector<Term>* terms) {
  terms->clear();
  terms->reserve(n);
  for (auto i = terms->size(); i < n; ++i) {
    const Term t = p();
    terms->push_back(t);
    std::ostringstream s;
    s << (i + 1);
    limbo::format::RegisterSymbol(t.symbol(), s.str());
  }
}

static bool LoadCnf(std::istream& stream,
                    std::vector<std::vector<Literal>>* cnf,
                    Term* extra_name) {
  Symbol::Factory* sf = Symbol::Factory::Instance();
  Term::Factory* tf = Term::Factory::Instance();
  const Symbol::Sort sort = sf->CreateNonrigidSort();  // for now, we use a single sort for everything
  limbo::format::RegisterSort(sort, "");
  std::vector<Term> funcs;
  std::vector<Term> names;
  Term T;
  Term F;
  cnf->clear();
  bool prop = false;
  for (std::string line; std::getline(stream, line); ) {
    int n_funcs;
    int n_names;
    int n_clauses;
    if (line.length() == 0) {
      continue;
    } else if (line.length() >= 1 && line[0] == 'c') {
      // ignore comment
    } else if (sscanf(line.c_str(), "p cnf %d %d", &n_funcs, &n_clauses) == 2) {  // propositional CNF
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateFunction(sort, 0)); }, n_funcs, &funcs);
      names.clear();
      T = tf->CreateTerm(sf->CreateName(sort));
      F = tf->CreateTerm(sf->CreateName(sort));
      names.push_back(T);
      names.push_back(F);
      limbo::format::RegisterSymbol(T.symbol(), "T");
      limbo::format::RegisterSymbol(F.symbol(), "F");
      *extra_name = F;
      prop = true;
    } else if (sscanf(line.c_str(), "p fcnf %d %d %d", &n_funcs, &n_names, &n_clauses) == 3) {  // func CNF
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateFunction(sort, 0)); }, n_funcs, &funcs);
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateName(sort)); }, n_names + 1, &names);
      *extra_name = names.back();
      prop = false;
    } else if (prop) {  // propositional clause
      std::vector<Literal> lits;
      int i = -1;
      for (std::istringstream iss(line); (iss >> i) && i != 0; ) {
        const Term f = funcs[(i < 0 ? -i : i) - 1];
#if 0
        const Literal a = i < 0 ? Literal::Eq(f, F) : Literal::Eq(f, T);
#else
        const Literal a = i < 0 ? Literal::Neq(f, T) : Literal::Eq(f, T);
#endif
        lits.push_back(a);
      }
      if (i == 0) {
        cnf->push_back(lits);
      }
    } else {  // functional clause
      std::vector<Literal> lits;
      int i;
      int j;
      char eq = '\0';
      for (std::istringstream iss(line); (iss >> i >> eq >> j) && eq == '='; ) {
        assert(j >= 1);
        const Term f = funcs[(i < 0 ? -i : i) - 1];
        const Term n = names[j - 1];
        const Literal a = i < 0 ? Literal::Neq(f, n) : Literal::Eq(f, n);
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

bool Solve(Solver& solver, int n_conflicts_init, int conflicts_increase) {
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

  auto conflict_predicate = [&](Solver::level_t level, Solver::cref_t conflict, const std::vector<Literal>& learnt, Solver::level_t btlevel) {
    ++stats.conflicts;
    stats.conflicts_level_sum += level;
    stats.conflicts_btlevel_sum += btlevel;
    //std::cout << "Learnt from " << solver.clause(conflict) << " at level " << level << " that " << learnt << ", backtracking to " << btlevel << std::endl;
    return !restarts || stats.conflicts < n_conflicts;
  };
  auto decision_predicate = [&](Solver::level_t level, Literal a) {
    ++stats.decisions;
    stats.decisions_level_sum += level;
    //std::cout << "Decided " << a << " at level " << level << std::endl;
    return true;
  };

  Timer t;
  t.start();
  for (int i = 0; sat == 0; ++i) {
    n_conflicts = static_cast<int>(std::pow(conflicts_increase, i) * n_conflicts_init);
    sat = solver.Solve(conflict_predicate, decision_predicate);
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

int main(int argc, char *argv[]) {
  std::srand(0);
  std::vector<std::vector<Literal>> cnf;
  Term extra_name;
  int iterations = 1;
  int n_columns = 0;
  int restarts = -1;
  int loaded = 0;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: [-k=<k>] " << argv[0] << std::endl;
      return 1;
    } else if (sscanf(argv[i], "--iterations=%d", &iterations) == 1 || sscanf(argv[i], "-i=%d", &iterations) == 1) {
    } else if (sscanf(argv[i], "--columns=%d", &n_columns) == 1 || sscanf(argv[i], "-c=%d", &n_columns) == 1) {
    } else if (sscanf(argv[i], "--restart=%d", &restarts) == 1 || sscanf(argv[i], "-r=%d", &restarts) == 1) {
    } else if (loaded == 0 && argv[i][0] != '-') {
      std::ifstream ifs = std::ifstream(argv[i]);
      LoadCnf(ifs, &cnf, &extra_name);
      ++loaded;
    } else {
      std::cout << "Cannot load '"<< argv[i] << "'" << std::endl;
      return 2;
    }
  }
  if (loaded == 0) {
    LoadCnf(std::cin, &cnf, &extra_name);
    ++loaded;
  }
  assert(loaded > 0);
  assert(!extra_name.null());

  Timer t0;
  t0.start();
  for (int i = 1; i <= iterations; ++i) {
    Solver solver{};
    auto extra_name_factory = [extra_name](const Symbol::Sort s) { assert(s == extra_name.sort()); return extra_name; };
    for (const std::vector<Literal>& lits : cnf) {
      solver.AddClause(lits, extra_name_factory);
    }
    solver.Init();

    const bool sat = Solve(solver, restarts, 2);

    if (sat) {
      struct winsize win_size;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size);
      const int lit_width = 10;
      const int win_width = (n_columns != 0 ? n_columns : static_cast<int>(std::ceil(std::sqrt(solver.funcs().upper_bound())))) * lit_width;
      int i = 0;
      for (const Term f : solver.funcs()) {
        if (f.null()) {
          continue;
        }
        std::stringstream fss; fss << f;
        std::stringstream ess; ess << " = ";
        std::stringstream nss; nss << solver.model()[f];
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
      std::cout << std::endl;
    }
  }
  t0.stop();
  std::cout << "Total took " << t0.duration() << " seconds" << std::endl;

  return 0;
}

