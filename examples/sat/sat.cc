// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2018 Christoph Schwering
//
// A SAT solver using Limbo data structures. The purpose of this file is
// to find reasons why Limbo is so slow.

#include <cassert>
#include <cstdio>
#include <cstring>

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
                    std::vector<Term>* funcs,
                    std::vector<Term>* names,
                    Term* extra_name) {
  Symbol::Factory* sf = Symbol::Factory::Instance();
  Term::Factory* tf = Term::Factory::Instance();
  const Symbol::Sort sort = sf->CreateNonrigidSort();  // for now, we use a single sort for everything
  Term T;
  Term F;
  cnf->clear();
  for (std::string line; std::getline(stream, line); ) {
    int n_funcs;
    int n_names;
    int n_clauses;
    if (line.length() >= 1 && line[0] == 'c') {
      // ignore comment
    } else if (sscanf(line.c_str(), "p cnf %d %d", &n_funcs, &n_clauses) == 2) {  // propositional CNF
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateFunction(sort, 0)); }, n_funcs, funcs);
      names->clear();
      T = tf->CreateTerm(sf->CreateName(sort));
      F = tf->CreateTerm(sf->CreateName(sort));
      names->push_back(T);
      names->push_back(F);
      limbo::format::RegisterSort(sort, "");
      limbo::format::RegisterSymbol(T.symbol(), "T");
      limbo::format::RegisterSymbol(F.symbol(), "F");
      n_names = -1;
      *extra_name = F;
    } else if (sscanf(line.c_str(), "p fcnf %d %d %d", &n_funcs, &n_names, &n_clauses) == 2) {  // functional CNF
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateFunction(sort, 0)); }, n_funcs, funcs);
      CreateTerms([tf, sf, sort]() { return tf->CreateTerm(sf->CreateName(sort)); }, n_names + 1, names);
      *extra_name = names->back();
    } else if (n_names < 0) {  // propositional clause
      std::vector<Literal> lits;
      int i = -1;
      for (std::istringstream iss(line); (iss >> i) && i != 0; ) {
        const Term f = (*funcs)[(i < 0 ? -i : i) - 1];
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
        const Term f = (*funcs)[(i < 0 ? -i : i) - 1];
        const Term n = (*names)[j - 1];
        const Literal a = i < 0 ? Literal::Neq(f, n) : Literal::Eq(f, n);
        lits.push_back(a);
      }
      if (eq == '=') {
        cnf->push_back(lits);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  std::vector<std::vector<Literal>> cnf;
  std::vector<Term> funcs;
  std::vector<Term> names;
  Term extra_name;
  int k = 0;
  int loaded = 0;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: [-k=<k>] " << argv[0] << std::endl;
      return 1;
    } else if (sscanf(argv[i], "-k=%d", &k) == 1) {
    } else if (loaded == 0) {
      std::ifstream ifs = std::ifstream(argv[i]);
      LoadCnf(ifs, &cnf, &funcs, &names, &extra_name);
      ++loaded;
    } else {
      std::cout << "Cannot load more than one file" << std::endl;
      return 2;
    }
  }
  if (loaded == 0) {
    LoadCnf(std::cin, &cnf, &funcs, &names, &extra_name);
  }
  std::cout << "k="<<k << std::endl;
  //std::cout << solver << std::endl;

  Solver solver;
  solver.add_extra_name(extra_name);
  for (const std::vector<Literal>& lits : cnf) {
    std::cout << lits << std::endl;
    solver.AddClause(lits);
  }

  bool sat = solver.Solve();
  std::cout << (sat ? "SATISFIABLE" : "UNSATISFIABLE") << std::endl;
  if (sat) {
    for (Term f : funcs) {
      std::cout << f << " = " << solver.model()[f] << std::endl;
    }
  }

  return 0;
}

