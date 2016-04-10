// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include <./grounder.h>
#include <./print.h>

using namespace lela;

TEST(grounder_test, grounder) {
  const Symbol::Sort s1 = 1;
  const Symbol::Sort s2 = 2;
  const Term n1 = Term::Create(Symbol::CreateName(1, s1));
  const Term n2 = Term::Create(Symbol::CreateName(2, s1));
  const Term x1 = Term::Create(Symbol::CreateVariable(1, s1));
  const Term x2 = Term::Create(Symbol::CreateVariable(2, s1));
  const Term c1 = Term::Create(Symbol::CreateFunction(1, s1, 0), {});
  //const Term c2 = Term::Create(Symbol::CreateFunction(2, s1, 0), {});
  const Term f1 = Term::Create(Symbol::CreateFunction(1, s1, 1), {n1});
  const Term f2 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,x2});
  const Term f3 = Term::Create(Symbol::CreateFunction(1, s2, 1), {f1});
  const Term f4 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,f1});

  std::vector<Clause> kb;
  kb.push_back(Clause({Literal::Eq(c1,x1)}));
  kb.push_back(Clause({Literal::Eq(f2,x2)}));
  std::map<Symbol::Sort, size_t> plus{{s1,2},{s2,1}};
  Grounder::Ground(kb, plus);
}

