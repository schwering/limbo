// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./grounder.h"
#include "./print.h"

using namespace lela;

template<typename T>
size_t dist(T r) { return std::distance(r.begin(), r.end()); }

TEST(grounder, grounder) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  //const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s2));
  const Symbol a = sf.CreateFunction(s1, 0);
  //const Symbol f = sf.CreateFunction(s1, 1);
  //const Symbol g = sf.CreateFunction(s2, 1);
  const Symbol h = sf.CreateFunction(s2, 2);
  const Symbol i = sf.CreateFunction(s2, 2);
  const Term c1 = tf.CreateTerm(a, {});
  //const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});
  //const Term f3 = tf.CreateTerm(g, {f1});
  //const Term f4 = tf.CreateTerm(h, {n1,f1});
  const Term f5 = tf.CreateTerm(i, {x1,x3});

  std::map<Symbol::Sort, size_t> plus{{s1,2},{s2,2}};
  std::vector<Clause> kb;
  kb.push_back(Clause({Literal::Eq(c1,x1)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(dist(s.clauses()), 2);
  }
  kb.push_back(Clause({Literal::Eq(f2,x2)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(dist(s.clauses()), 3 + 3);
  }
  kb.push_back(Clause({Literal::Eq(f5,x2)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(dist(s.clauses()), 3 + 3 + 3*3*2);
  }
}

