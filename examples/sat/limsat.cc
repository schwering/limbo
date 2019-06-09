// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
//
// A limited SAT solver using Limbo data structures.

#include <cassert>
#include <cmath>
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
#include <limbo/limsat.h>

using namespace limbo;
using namespace limbo::internal;

#ifndef LIMBO_IO_OUTPUT_H_
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
#endif

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
  for (T i = terms->size(); i < n; ++i) {
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

bool Solve(const std::vector<std::vector<Lit>>& cnf, int k_splits, const RFormula& query) {
  LimSat lim_sat;
  for (const std::vector<Lit>& lits : cnf) {
    lim_sat.AddClause(lits);
  }
  Timer t;
  t.start();
  assert(query.empty());
  const bool truth = lim_sat.Solve(k_splits, query);
  t.stop();
  printf("%d-%s (in %.5lfs)\n", k_splits, (truth ? "SATISFIABLE" : "UNSATISFIABLE"), t.duration());
  return truth;
}

int main(int argc, char *argv[]) {
  std::srand(0);
  std::vector<std::vector<Lit>> cnf;
  std::vector<Fun> funs;
  std::vector<Name> names;
  Name extra_name;
  int k_splits = 0;
  int loaded = 0;
  bool prop = false;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: " << argv[0] << " [options] [file]" << std::endl;
      std::cout << std::endl;
      std::cout << "If file is not specified, input is read from stdin." << std::endl;
      std::cout << "Input must be in DIMACS CNF format or the functional extension thereof." << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "--splits=int     -k=int  number of splits (default: " << k_splits << ")" << std::endl;
      std::cout << std::endl;
#ifndef NDEBUG
      std::cout << "Debugging is turned on (NDEBUG is not defined)." << std::endl;
#else
      std::cout << "Debugging is turned off (NDEBUG is defined)." << std::endl;
#endif
      return 1;
    } else if (sscanf(argv[i], "--splits=%d", &k_splits) == 1 || sscanf(argv[i], "-k=%d", &k_splits) == 1) {
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

  const Formula query = Formula();

  Solve(cnf, k_splits, query.readable());
  return 0;
}

