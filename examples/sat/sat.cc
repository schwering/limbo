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

static void LoadCnf(std::istream& stream,
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
        //const Literal a = i < 0 ? Literal::Eq(f, F) : Literal::Eq(f, T);
        const Literal a = i < 0 ? Literal::Neq(f, T) : Literal::Eq(f, T);
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
  size_t rounds() const { return rounds_; }
  double avg_duration() const { return duration() / rounds_; }

 private:
  std::clock_t start_;
  std::clock_t end_ = 0;
  size_t rounds_ = 0;
};

int main(int argc, char *argv[]) {
  std::vector<std::vector<Literal>> cnf;
  Term extra_name;
  int k = 0;
  int l = 1;
  int w = 0;
  int loaded = 0;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: [-k=<k>] " << argv[0] << std::endl;
      return 1;
    } else if (sscanf(argv[i], "-k=%d", &k) == 1) {
    } else if (sscanf(argv[i], "-l=%d", &l) == 1) {
    } else if (sscanf(argv[i], "-w=%d", &w) == 1) {
    } else if (loaded == 0) {
      std::ifstream ifs = std::ifstream(argv[i]);
      LoadCnf(ifs, &cnf, &extra_name);
      ++loaded;
    } else {
      std::cout << "Cannot load more than one file" << std::endl;
      return 2;
    }
  }
  if (loaded == 0) {
    LoadCnf(std::cin, &cnf, &extra_name);
  }

  Timer t0;
  t0.start();
  for (int i = 1; i <= l; ++i) {
    Solver solver{};
    solver.add_extra_name(extra_name);
    for (const std::vector<Literal>& lits : cnf) {
      solver.AddClause(lits);
    }

    Timer t;
    t.start();
    const bool sat = solver.Solve();
    t.stop();

    std::cout << (sat ? "SATISFIABLE" : "UNSATISFIABLE") << " (in " << t.duration() << "s)" << std::endl;
    if (sat) {
      struct winsize win_size;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size);
      const int lit_width = 10;
      const int win_width = (w != 0 ? w : static_cast<int>(std::ceil(std::sqrt(solver.funcs().size())))) * lit_width;
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

