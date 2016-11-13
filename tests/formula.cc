// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/formula.h>
#include <lela/pretty.h>

namespace lela {

using namespace output;

TEST(FormulaTest, reader) {
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

  EXPECT_EQ(c1.reader().head().type(), Formula::Element::kClause);
  EXPECT_EQ(c1.reader().head().clause(), internal::Just(cl1));
  EXPECT_EQ(c1.reader().Build(), c1);
  EXPECT_EQ(Formula::Not(c1).reader().head().type(), Formula::Element::kNot);
  EXPECT_EQ(Formula::Not(c1).reader().arg().head().type(), Formula::Element::kClause);
  EXPECT_EQ(Formula::Not(c1).reader().arg().head().clause(), internal::Just(cl1));
  EXPECT_EQ(Formula::Not(c1).reader().arg().Build(), c1);

  EXPECT_EQ(c2.reader().head().type(), Formula::Element::kClause);
  EXPECT_EQ(c2.reader().head().clause(), internal::Just(cl2));

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

TEST(FormulaTest, substitution) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s1));
  const Symbol a = sf.CreateFunction(s1, 0);
  const Symbol f = sf.CreateFunction(s1, 1);
  const Symbol h = sf.CreateFunction(s2, 2);
  const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});
  const Term f3 = tf.CreateTerm(a, {});

  auto phi = [f1,f2](Term x, Term t) { return Formula::Not(Formula::Exists(x, Formula::Clause(Clause{Literal::Eq(x,t), Literal::Neq(f1,f2)}))); };

  EXPECT_EQ(phi(x1,n2).reader().Substitute(Term::SingleSubstitution(n2,n1), &tf).Build(), phi(x1,n1));
  EXPECT_EQ(phi(x1,f3).reader().Substitute(Term::SingleSubstitution(f3,n1), &tf).Build(), phi(x1,n1));
  EXPECT_EQ(phi(x1,f2).reader().Substitute(Term::SingleSubstitution(x1,x3), &tf).Build(), phi(x3,f2));
}

}  // namespace lela

