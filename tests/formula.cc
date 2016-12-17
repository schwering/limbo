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

TEST(FormulaTest, substitution) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s1));
  const Symbol a = sf.CreateFunction(s1, 0);
  const Symbol f = sf.CreateFunction(s1, 1);
  const Symbol h = sf.CreateFunction(s1, 2);
  const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});
  const Term f3 = tf.CreateTerm(a, {});

  auto phi = [x1,f1,f2](Term x, Term t) { return Formula::Not(Formula::Exists(x1, Formula::Atomic(Clause{Literal::Eq(x,t), Literal::Neq(f1,f2)}))); };

  EXPECT_NE(phi(x1,n1), phi(x2,n2));
  { Formula::Ref psi = phi(x1,n2); psi->SubstituteFree(Term::SingleSubstitution(n2,n1), &tf); EXPECT_EQ(*psi, *phi(x1,n1)); }
  { Formula::Ref psi = phi(x1,f3); psi->SubstituteFree(Term::SingleSubstitution(f3,n1), &tf); EXPECT_EQ(*psi, *phi(x1,n1)); }
  { Formula::Ref psi = phi(x1,f2); psi->SubstituteFree(Term::SingleSubstitution(x1,x3), &tf); EXPECT_EQ(*psi, *phi(x1,f2)); }
  { Formula::Ref psi = phi(x1,f2); psi->SubstituteFree(Term::SingleSubstitution(x1,n1), &tf); EXPECT_EQ(*psi, *phi(x1,f2)); }
  { Formula::Ref psi = phi(x1,f2); psi->SubstituteFree(Term::SingleSubstitution(x1,n1), &tf); EXPECT_NE(*psi, *phi(n1,f2)); }
  { Formula::Ref psi = phi(x3,f2); psi->SubstituteFree(Term::SingleSubstitution(x3,n1), &tf); EXPECT_EQ(*psi, *phi(n1,f2)); }
}

TEST(Formula, NF) {
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
    Formula::Ref phi = *Ex(x, John() == x);
    EXPECT_EQ(*phi, *Formula::Exists(x, Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(John, {}), x)})));
    EXPECT_EQ(*phi->NF(ctx.sf(), ctx.tf()), *Formula::Exists(x, Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(John, {}), x)})));
  }
  {
    Formula::Ref phi = *Fa(x, John() == x);
    EXPECT_EQ(*phi, *Formula::Not(Formula::Exists(x, Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(John, {}), x)})))));
    EXPECT_EQ(*phi->NF(ctx.sf(), ctx.tf()), *Formula::Not(Formula::Exists(x, Formula::Atomic(Clause{Literal::Neq(tf.CreateTerm(John, {}), x)}))));
  }
  {
    Formula::Ref phi = *Fa(x, IsParentOf(Mother(x), x) == True && IsParentOf(Father(x), x) == True);
    EXPECT_EQ(*phi, *Formula::Not(Formula::Exists(x, Formula::Not(Formula::Not(Formula::Or(Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Mother, {x}), x}), True)})),
                                                                                           Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Father, {x}), x}), True)}))))))));
    Formula::Ref phi_nf = phi->NF(ctx.sf(), ctx.tf());
    Term x_tmp1 = (*phi_nf).as_not().arg().as_exists().arg().as_not().arg().as_exists().x();
    Term x_tmp2 = (*phi_nf).as_not().arg().as_exists().arg().as_not().arg().as_exists().arg().as_exists().x();
    auto phi_nf_exp = [&](Term x_tmp1, Term x_tmp2) {
      return Formula::Not(Formula::Exists(x, Formula::Not(Formula::Exists(x_tmp1, Formula::Exists(x_tmp2, Formula::Not(
                              Formula::Atomic(Clause{
                                              Literal::Neq(tf.CreateTerm(IsParentOf, {x_tmp2, x}), True),
                                              Literal::Neq(tf.CreateTerm(IsParentOf, {x_tmp1, x}), True),
                                              Literal::Neq(tf.CreateTerm(Father, {x}), x_tmp1),
                                              Literal::Neq(tf.CreateTerm(Mother, {x}), x_tmp2)
                                              })))))));
    };
    EXPECT_TRUE(*phi_nf == *phi_nf_exp(x_tmp1, x_tmp2) || *phi_nf == *phi_nf_exp(x_tmp2, x_tmp1));
  }
  {
    Formula::Ref phi = *Fa(x, IsParentOf(x, y) == True && IsParentOf(Father(x), x) == True);
    Formula::Ref phi_nf = phi->NF(ctx.sf(), ctx.tf());
    Term x_tmp = (*phi_nf).as_not().arg().as_exists().arg().as_not().arg().as_exists().x();
    EXPECT_EQ(*phi, *Formula::Not(Formula::Exists(x, Formula::Not(Formula::Not(Formula::Or(Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {x, y}), True)})),
                                                                                           Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(IsParentOf, {tf.CreateTerm(Father, {x}), x}), True)}))))))));
    EXPECT_EQ(*phi_nf, *Formula::Not(Formula::Exists(x, Formula::Not(Formula::Exists(x_tmp, Formula::Not(
                            Formula::Atomic(Clause{
                                            Literal::Neq(tf.CreateTerm(IsParentOf, {x, y}), True),
                                            Literal::Neq(tf.CreateTerm(IsParentOf, {x_tmp, x}), True),
                                            Literal::Neq(tf.CreateTerm(Father, {x}), x_tmp)
                                            })))))));
  }

  {
    auto P = ctx.CreateFunction(BOOL, 1);    REGISTER_SYMBOL(P);
    auto Q = ctx.CreateFunction(BOOL, 1);    REGISTER_SYMBOL(P);
    // That's the example formula from my thesis.
    Formula::Ref phi = *(Ex(x, P(x) == True) >> Fa(y, Q(y) == True));
    Formula::Ref phi_nf = phi->NF(ctx.sf(), ctx.tf());
    EXPECT_EQ(*phi, *Formula::Or(Formula::Not(Formula::Exists(x, Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(P, {x}), True)}))),
                                 Formula::Not(Formula::Exists(y, Formula::Not(Formula::Atomic(Clause{Literal::Eq(tf.CreateTerm(Q, {y}), True)}))))));
    EXPECT_EQ(*phi_nf,
              *Formula::Not(Formula::Exists(x, Formula::Not(Formula::Not(Formula::Exists(y, Formula::Not(
                                       Formula::Atomic(Clause{Literal::Neq(tf.CreateTerm(P, {x}), True),
                                                       Literal::Eq(tf.CreateTerm(Q, {y}), True)})
                                       )))))));
  }
}

}  // namespace lela

