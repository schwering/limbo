// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering
//
// A SAT solver using Limbo data structures. The purpose of this file is
// to find reasons why Limbo is so slow.

#include <cassert>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>

#include <limbo/clause.h>
#include <limbo/literal.h>
#include <limbo/setup.h>
#include <limbo/term.h>

#include <limbo/format/output.h>

using namespace limbo;

static void LoadDnf(std::istream& stream, Setup* setup, std::vector<Term>* funcs, std::vector<Term>* names) {
  Symbol::Factory* sf = Symbol::Factory::Instance();
  Term::Factory* tf = Term::Factory::Instance();
  const Symbol::Sort sort = sf->CreateNonrigidSort();  // for now, we use a single sort for everything

  for (std::string line; std::getline(stream, line); ) {
    int n_funcs;
    int n_names;
    int n_clauses;
    if (sscanf(line.c_str(), "c") == 1) {
      // ignore comment
    } else if (sscanf(line.c_str(), "p cnf %d %d", &n_funcs, &n_clauses) == 2) {  // propositional CNF
      *funcs = std::vector<Term>();
      for (int i = 0; i < n_funcs; ++i) {
        const Term f = tf->CreateTerm(sf->CreateFunction(sort, 0));
        funcs->push_back(f);
        std::ostringstream name;
        name << (i + 1);
        limbo::format::RegisterSymbol(f.symbol(), name.str());
      }
      *names = std::vector<Term>();
      const Term T = tf->CreateTerm(sf->CreateName(sort));
      names->push_back(T);
      limbo::format::RegisterSort(sort, "");
      limbo::format::RegisterSymbol(T.symbol(), "T");
      n_names = -1;
    } else if (sscanf(line.c_str(), "p fcnf %d %d %d", &n_funcs, &n_names, &n_clauses) == 2) {  // functional CNF
      *funcs = std::vector<Term>();
      for (int i = 0; i < n_funcs; ++i) {
        const Term f = tf->CreateTerm(sf->CreateFunction(sort, 0));
        funcs->push_back(f);
        std::ostringstream name;
        name << (i + 1);
        limbo::format::RegisterSymbol(f.symbol(), name.str());
      }
      *names = std::vector<Term>();
      for (int i = 0; i < n_names; ++i) {
        const Term n = tf->CreateTerm(sf->CreateFunction(sort, 0));
        names->push_back(n);
        std::ostringstream name;
        name << (i + 1);
        limbo::format::RegisterSymbol(n.symbol(), name.str());
      }
    } else if (n_names < 0) {  // propositional clause
      std::vector<Literal> lits;
      int i = -1;
      for (std::istringstream iss(line); (iss >> i) && i != 0; ) {
        const Term f = (*funcs)[(i < 0 ? -i : i) - 1];
        const Term n = (*names)[0];
        const Literal a = i < 0 ? Literal::Neq(f, n) : Literal::Eq(f, n);
        lits.push_back(a);
      }
      if (i == 0) {
        const Clause c(lits.begin(), lits.end());
        setup->AddClause(c);
      }
    } else {  // functional clause
      std::vector<Literal> lits;
      int i;
      int j;
      char eq = '\0';
      for (std::istringstream iss(line); (iss >> i >> eq >> j) && eq == '='; ) {
        const Term f = (*funcs)[(i < 0 ? -i : i) - 1];
        const Term n = (*names)[(j < 0 ? -j : j) - 1];
        const Literal a = i < 0 ? Literal::Neq(f, n) : Literal::Eq(f, n);
        lits.push_back(a);
      }
      if (eq == '=') {
        const Clause c(lits.begin(), lits.end());
        setup->AddClause(c);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  Setup setup;
  std::vector<Term> funcs;
  std::vector<Term> names;
  int k = 0;
  int loaded = 0;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-h") || argv[i] == std::string("--help")) {
      std::cout << "Usage: [-k=<k>] " << argv[0] << std::endl;
      return 1;
    } else if (sscanf(argv[i], "-k=%d", &k) == 1) {
    } else if (loaded == 0) {
      std::ifstream ifs = std::ifstream(argv[1]);
      LoadDnf(ifs, &setup, &funcs, &names);
      ++loaded;
    } else {
      std::cout << "Cannot load more than one file" << std::endl;
      return 2;
    }
  }
  if (loaded == 0) {
    LoadDnf(std::cin, &setup, &funcs, &names);
  }
  std::cout << "k="<<k << std::endl;
  std::cout << setup << std::endl;
}

