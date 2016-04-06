// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include <./literal.h>
#include <./print.h>

using namespace lela;

TEST(literal_test, symbol) {
  const Symbol::Sort s1 = 1;
  const Symbol::Sort s2 = 2;
  const Term n1 = Term::Create(Symbol::CreateName(1, s1));
  const Term n2 = Term::Create(Symbol::CreateName(2, s1));
  const Term x1 = Term::Create(Symbol::CreateVariable(1, s1));
  const Term x2 = Term::Create(Symbol::CreateVariable(2, s1));
  const Term f1 = Term::Create(Symbol::CreateFunction(1, s1, 1), {n1});
  const Term f2 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,x2});
  const Term f3 = Term::Create(Symbol::CreateFunction(1, s2, 1), {f1});
  const Term f4 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,f1});

  EXPECT_TRUE(Literal::Eq(x1,n1).dual() == Literal::Eq(n1,x1));
  EXPECT_TRUE(Literal::Eq(x1,n1).flip() == Literal::Neq(x1,n1));
  EXPECT_TRUE(Literal::Eq(x1,n1).flip() == Literal::Neq(x1,n1).flip().flip());
  EXPECT_TRUE(Literal::Eq(x1,n1) == Literal::Eq(x1,n1).flip().flip());

  EXPECT_TRUE(!Literal::Eq(x1,n1).ground());
  EXPECT_TRUE(!Literal::Eq(x1,n1).primitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).flip().quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).dual().quasiprimitive());

  EXPECT_TRUE(Literal::Eq(f1,n1).ground());
  EXPECT_TRUE(Literal::Eq(f1,n1).primitive());
  EXPECT_TRUE(Literal::Eq(f1,n1).quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f1,n1).flip().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f1,n1).dual().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f1,n1).dual() == Literal::Eq(f1,n1).dual());

  EXPECT_TRUE(!Literal::Eq(f2,n1).ground());
  EXPECT_TRUE(!Literal::Eq(f2,n1).primitive());
  EXPECT_TRUE(Literal::Eq(f2,n1).quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f2,n1).flip().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f2,n1).dual().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f2,n1) == Literal::Eq(f2,n1).dual());

  EXPECT_TRUE(Literal::Eq(f3,n1).ground());
  EXPECT_TRUE(!Literal::Eq(f3,n1).primitive());
  EXPECT_TRUE(!Literal::Eq(f3,n1).quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(f3,n1).flip().quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(f3,n1).dual().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f3,n1).dual() == Literal::Eq(f3,n1).dual());

  EXPECT_TRUE(Literal::Eq(f4,n1).ground());
  EXPECT_TRUE(!Literal::Eq(f4,n1).primitive());
  EXPECT_TRUE(!Literal::Eq(f4,n1).quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(f4,n1).flip().quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(f4,n1).dual().quasiprimitive());
  EXPECT_TRUE(Literal::Eq(f4,n1) == Literal::Eq(f4,n1).dual());

  EXPECT_TRUE(Literal::Eq(n1,n1).valid());
  EXPECT_TRUE(!Literal::Neq(n1,n1).valid());
  EXPECT_TRUE(Literal::Eq(f1,f1).valid());
  EXPECT_TRUE(!Literal::Neq(f1,f1).valid());
  EXPECT_TRUE(!Literal::Neq(f1,n1).valid());
  EXPECT_TRUE(!Literal::Neq(f1,f2).valid());

  EXPECT_TRUE(!Literal::Eq(n1,n1).invalid());
  EXPECT_TRUE(Literal::Neq(n1,n1).invalid());
  EXPECT_TRUE(!Literal::Eq(f1,f1).invalid());
  EXPECT_TRUE(Literal::Neq(f1,f1).invalid());
  EXPECT_TRUE(!Literal::Neq(f1,n1).invalid());
  EXPECT_TRUE(!Literal::Neq(f1,f2).invalid());
}

