// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/formula.h>
#include <lela/format/output.h>
#include <lela/format/cpp/syntax.h>

namespace lela {

using namespace lela::format::cpp;
using namespace lela::format::output;

#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

TEST(Syntax, general) {
  // An extended version of this test is TEST(Formula, NF), which also tests the normal form.
  Context ctx;
  Term::Factory& tf = *ctx.tf();
  auto BOOL = ctx.CreateSort();
  auto True = ctx.CreateName(BOOL);                 REGISTER_SYMBOL(True);
  auto HUMAN = ctx.CreateSort();
  auto Father = ctx.CreateFunction(HUMAN, 1);       REGISTER_SYMBOL(Father);
  auto Mother = ctx.CreateFunction(HUMAN, 1);       REGISTER_SYMBOL(Mother);
  auto IsParentOf = ctx.CreateFunction(BOOL, 2);    REGISTER_SYMBOL(IsParentOf);
  auto John = ctx.CreateFunction(HUMAN, 0);         REGISTER_SYMBOL(John);
  auto x = ctx.CreateVariable(HUMAN);               REGISTER_SYMBOL(x);
  auto y = ctx.CreateVariable(HUMAN);               REGISTER_SYMBOL(y);
  {
    auto phi = *Ex(x, John() == x);
    EXPECT_EQ(*phi, *Formula::Factory::Exists(x, Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(John, {}), x)})));
  }
  {
    auto phi = *Fa(x, John() == x);
    EXPECT_EQ(*phi, *Formula::Factory::Not(Formula::Factory::Exists(x, Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(John, {}), x)})))));
  }
  {
    auto phi = *Fa(x, IsParentOf(Mother(x), x) == True && IsParentOf(Father(x), x) == True);
    EXPECT_EQ(*phi, *Formula::Factory::Not(Formula::Factory::Exists(x, Formula::Factory::Not(Formula::Factory::Not(Formula::Factory::Or(
                            Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Mother, {x}), x}), True)})),
                            Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Father, {x}), x}), True)}))))))));
  }
  {
    auto phi = *Fa(x, IsParentOf(x, y) == True && IsParentOf(Father(x), x) == True);
    EXPECT_EQ(*phi, *Formula::Factory::Not(Formula::Factory::Exists(x, Formula::Factory::Not(Formula::Factory::Factory::Not(Formula::Factory::Or(
                            Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {x, y}), True)})),
                            Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Father, {x}), x}), True)}))))))));
  }

  {
    auto P = ctx.CreateFunction(BOOL, 1);    REGISTER_SYMBOL(P);
    auto Q = ctx.CreateFunction(BOOL, 1);    REGISTER_SYMBOL(P);
    auto phi = *(Ex(x, P(x) == True) >> Fa(y, Q(y) == True));
    EXPECT_EQ(*phi, *Formula::Factory::Or(
            Formula::Factory::Not(Formula::Factory::Factory::Factory::Exists(x, Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(P, {x}), True)}))),
            Formula::Factory::Not(Formula::Factory::Exists(y, Formula::Factory::Not(Formula::Factory::Atomic(Clause{Literal::Eq(tf.CreateTerm(Q, {y}), True)}))))));
  }
}

}  // namespace lela

