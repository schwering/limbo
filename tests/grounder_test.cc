// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./formula.h"
#include "./grounder.h"
#include "./print.h"

using namespace lela;

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

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
  const Term x4 = tf.CreateTerm(sf.CreateVariable(s2));
  const Symbol a = sf.CreateFunction(s1, 0);
  //const Symbol f = sf.CreateFunction(s1, 1);
  //const Symbol g = sf.CreateFunction(s2, 1);
  //const Symbol h = sf.CreateFunction(s2, 2);
  //const Symbol i = sf.CreateFunction(s2, 2);
  const Term c1 = tf.CreateTerm(a, {});
  //const Term f1 = tf.CreateTerm(f, {n1});
  //const Term f2 = tf.CreateTerm(h, {n1,x2});
  //const Term f3 = tf.CreateTerm(g, {f1});
  //const Term f4 = tf.CreateTerm(h, {n1,f1});
  //const Term f5 = tf.CreateTerm(i, {x1,x3});

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(n1,n1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(length(s.clauses()), 0);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1,n1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is invalid and hence boiled down
    // to [].
    std::cout << s << std::endl;
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(length(s.clauses()), 0);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is invalid and hence boiled down
    // to [].
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(n1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1], [n2=n1]. The first clause is valid and
    // hence skipped. The second is invalid and hence boiled down to [].
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1/=n1], [n2/=n1]. The second clause is valid and
    // hence skipped. The first is invalid and hence boiled down to [].
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(c1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [c1=n1].
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(c1,x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [c1/=n1].
    EXPECT_EQ(length(s.clauses()), 1);
  }

  /*
  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1,n1)}));
    lela::Setup s = g.Ground();
    EXPECT_EQ(length(s.clauses()), 1); // because the grounding contains only the clauses [n1/=n1], [n2/=n2] which is skipped because it's valid
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1,x1)}));
    lela::Setup s = g.Ground();
    EXPECT_EQ(length(s.clauses()), 0); // because the grounding contains only the clauses [n1=n1], which is skipped because it's valid
  }

  std::cout << "OK" << std::endl;

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1,x1)}));
    lela::Setup s = g.Ground();
    EXPECT_EQ(length(s.clauses()), 1);
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(c1,x1)}));
    lela::Setup s = g.Ground();
    EXPECT_EQ(length(s.clauses()), 1);
  }
  */

//  {
//    Grounder g(&sf, &tf);
//    g.AddClause(Clause({Literal::Eq(c1,x1)}));
//    g.PrepareFor(Formula::Exists(x1, Formula::Clause({Literal::Eq(x1,x1)})).reader());
//    g.PrepareFor(Formula::Exists(x2, Formula::Clause({Literal::Eq(x2,x2)})).reader());
//    g.PrepareFor(Formula::Exists(x3, Formula::Clause({Literal::Eq(x3,x3)})).reader());
//    g.PrepareFor(Formula::Exists(x4, Formula::Clause({Literal::Eq(x4,x4)})).reader());
//    lela::Setup s = g.Ground();
//    EXPECT_EQ(length(s.clauses()), 2);
//  }


#if 0
  Grounder::PlusMap plus;
  plus[s1] = 2;
  plus[s2] = 2;
  std::vector<Clause> kb;
  kb.push_back(Clause({Literal::Eq(c1,x1)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(length(s.clauses()), 2);
  }
  kb.push_back(Clause({Literal::Eq(f2,x2)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(length(s.clauses()), 3 + 3);
  }
  kb.push_back(Clause({Literal::Eq(f5,x2)}));
  {
    lela::Setup s = Grounder::Ground(kb, plus, &tf);
    EXPECT_EQ(length(s.clauses()), 3 + 3 + 3*3*2);
  }
#endif
}

