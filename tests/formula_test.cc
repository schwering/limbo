// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./formula.h"
#include "./print.h"

using namespace lela;

TEST(formula, formula) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Symbol f = sf.CreateFunction(s1, 1);
  const Symbol h = sf.CreateFunction(s2, 2);
  const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});

  Clause cl1({Literal::Eq(f1,n1)});
  Clause cl2({Literal::Neq(f2,n2)});

  Formula c1 = Formula::Clause(cl1);
  Formula c2 = Formula::Clause(cl2);

  EXPECT_EQ(c1.reader().head().type(), Formula::kClause);
  EXPECT_EQ(c1.reader().head().clause(), cl1);
  EXPECT_EQ(c1.reader().Build(), c1);
  EXPECT_EQ(Formula::Not(c1).reader().head().type(), Formula::kNot);
  EXPECT_EQ(Formula::Not(c1).reader().arg().head().type(), Formula::kClause);
  EXPECT_EQ(Formula::Not(c1).reader().arg().head().clause(), cl1);
  EXPECT_EQ(Formula::Not(c1).reader().arg().Build(), c1);

  EXPECT_EQ(c2.reader().head().type(), Formula::kClause);
  EXPECT_EQ(c2.reader().head().clause(), cl2);

  EXPECT_EQ(Formula::Not(Formula::Not(c1)).reader().arg().arg().Build(), c1);
  EXPECT_EQ(Formula::Exists(x2, Formula::Not(c1)).reader().arg().arg().Build(), c1);
  EXPECT_EQ(Formula::Or(Formula::Not(c1), Formula::Not(c2)).reader().left().arg().Build(), c1);
  EXPECT_EQ(Formula::Or(Formula::Not(c1), Formula::Not(c2)).reader().right().arg().Build(), c2);
  EXPECT_EQ(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), c2).reader().left().left().arg().Build(), c1);
  EXPECT_EQ(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), c2).reader().Build(), Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), c2));
  EXPECT_EQ(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), c2).reader().left().right().arg().Build(), c2);
  EXPECT_EQ(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2)).reader().right().arg().Build(), c2);
  EXPECT_EQ(Formula::Exists(x2, c2).reader().Build(), Formula::Exists(x2, c2));
  EXPECT_EQ(Formula::Exists(x2, Formula::Exists(x2, c2)).reader().Build(), Formula::Exists(x2, Formula::Exists(x2, c2)));
  EXPECT_EQ(Formula::Exists(x2, Formula::Exists(x2, c2)).reader().arg().Build(), Formula::Exists(x2, c2));
  EXPECT_EQ(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2)).reader().right().Build(), Formula::Exists(x2, c2));
  EXPECT_EQ(Formula::Not(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2))).reader().arg().right().Build(), Formula::Exists(x2, c2));
  EXPECT_EQ(Formula::Not(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2))).reader().arg().Build(), Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2)));
  EXPECT_EQ(Formula::Not(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2))).reader().Build(), Formula::Not(Formula::Or(Formula::Or(Formula::Not(c1), Formula::Not(c2)), Formula::Exists(x2, c2))));
}

