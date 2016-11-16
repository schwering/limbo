// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/literal.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format;

TEST(LiteralTest, general) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(Symbol::Factory::CreateName(1, s1));
  //const Term n2 = tf.CreateTerm(Symbol::Factory::CreateName(2, s1));
  const Term x1 = tf.CreateTerm(Symbol::Factory::CreateVariable(1, s1));
  const Term x2 = tf.CreateTerm(Symbol::Factory::CreateVariable(2, s1));
  const Term f1 = tf.CreateTerm(Symbol::Factory::CreateFunction(1, s1, 1), {n1});
  const Term f2 = tf.CreateTerm(Symbol::Factory::CreateFunction(2, s2, 2), {n1,x2});
  const Term f3 = tf.CreateTerm(Symbol::Factory::CreateFunction(1, s2, 1), {f1});
  const Term f4 = tf.CreateTerm(Symbol::Factory::CreateFunction(2, s2, 2), {n1,f1});

  EXPECT_TRUE(Literal::Eq(x1,n1).dual() == Literal::Eq(n1,x1));
  EXPECT_TRUE(Literal::Eq(x1,n1).flip() == Literal::Neq(x1,n1));
  EXPECT_TRUE(Literal::Eq(x1,n1).flip() == Literal::Neq(x1,n1).flip().flip());
  EXPECT_TRUE(Literal::Eq(x1,n1) == Literal::Eq(x1,n1).flip().flip());

  EXPECT_TRUE(!Literal::Eq(x1,n1).ground());
  EXPECT_TRUE(!Literal::Eq(x1,n1).primitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).flip().quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,n1).dual().quasiprimitive());

  EXPECT_TRUE(!Literal::Eq(x1,x1).ground());
  EXPECT_TRUE(!Literal::Eq(x1,x1).primitive());
  EXPECT_TRUE(!Literal::Eq(x1,x1).quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,x1).flip().quasiprimitive());
  EXPECT_TRUE(!Literal::Eq(x1,x1).dual().quasiprimitive());

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

}  // namespace lela

