// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/formula.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format::output;

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

}  // namespace lela

