// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include <./term.h>
#include <./print.h>

using namespace lela;

TEST(term_test, symbol) {
  constexpr Symbol::Sort s1 = 1;
  constexpr Symbol::Sort s2 = 2;
  EXPECT_EQ(s1, s1);
  EXPECT_EQ(s2, s2);
  EXPECT_TRUE(s1 != s2);

  const Term n1 = Term::Create(Symbol::CreateName(1, s1));
  const Term n2 = Term::Create(Symbol::CreateName(2, s1));
  EXPECT_TRUE(n1 == Term::Create(Symbol::CreateName(1, s1)) && n2 != Term::Create(Symbol::CreateName(1, s1)));
  EXPECT_TRUE(n1 != Term::Create(Symbol::CreateName(2, s1)) && n2 == Term::Create(Symbol::CreateName(2, s1)));
  EXPECT_TRUE(!n1.null() && n1.name() && !n1.variable() && !n1.function());
  EXPECT_TRUE(!n2.null() && n2.name() && !n2.variable() && !n2.function());

  const Term x1 = Term::Create(Symbol::CreateVariable(1, s1));
  const Term x2 = Term::Create(Symbol::CreateVariable(2, s1));
  EXPECT_TRUE(!x1.null() && !x1.name() && x1.variable() && !x1.function());
  EXPECT_TRUE(!x2.null() && !x2.name() && x2.variable() && !x2.function());
  EXPECT_TRUE(n1 != x1 && n1 != x2 && n2 != x1 && n2 != x2);
  EXPECT_TRUE(x1 == Term::Create(Symbol::CreateVariable(1, s1)) && x2 != Term::Create(Symbol::CreateVariable(1, s1)));
  EXPECT_TRUE(x1 != Term::Create(Symbol::CreateVariable(2, s1)) && x2 == Term::Create(Symbol::CreateVariable(2, s1)));

  const Term f1 = Term::Create(Symbol::CreateFunction(1, s1, 1), {n1});
  const Term f2 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,x2});
  const Term f3 = Term::Create(Symbol::CreateFunction(1, s2, 1), {f1});
  const Term f4 = Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,f1});
  EXPECT_TRUE(!f1.null() && !f1.name() && !f1.variable() && f1.function() && f1.ground() && f1.primitive() && f1.quasiprimitive());
  EXPECT_TRUE(!f2.null() && !f2.name() && !f2.variable() && f2.function() && !f2.ground() && !f2.primitive() && f2.quasiprimitive());
  EXPECT_TRUE(!f3.null() && !f3.name() && !f3.variable() && f3.function() && f3.ground() && !f3.primitive() && !f3.quasiprimitive());
  EXPECT_TRUE(!f4.null() && !f4.name() && !f4.variable() && f4.function() && f4.ground() && !f4.primitive() && !f4.quasiprimitive());
  const Term f5 = f2.Substitute(x2, f1);
  EXPECT_TRUE(f2 != f4);
  EXPECT_TRUE(!f5.name() && !f5.variable() && f5.function() && f5.ground() && !f4.primitive() && !f4.quasiprimitive());
  EXPECT_TRUE(f5 != f2);
  EXPECT_TRUE(f5 == f4);
  EXPECT_TRUE(f5 == Term::Create(Symbol::CreateFunction(2, s2, 2), {n1,f1}));

  Term::Set terms;
  struct IsTrue { bool operator()(Term t) const { return true; } };
  struct IsNameOfSort { IsNameOfSort(Symbol::Sort sort) : sort_(sort) {} bool operator()(Term t) const { return t.name() && t.symbol().sort() == sort_; } Symbol::Sort sort_; };
  struct TermIdentity { Term operator()(Term t) const { return t; } };
  struct TermSort { Symbol::Sort operator()(Term t) const { return t.symbol().sort(); } };

  terms.clear();
  f4.Collect(IsNameOfSort(s1), TermIdentity(), &terms);
  EXPECT_TRUE(terms == Term::Set({n1}));

  terms.clear();
  f4.Collect(IsTrue(), TermIdentity(), &terms);
  EXPECT_TRUE(terms == Term::Set({n1,f1,f4}));

  std::set<Symbol::Sort> sorts;
  f4.Collect(IsTrue(), TermSort(), &sorts);
  EXPECT_TRUE(sorts == std::set<Symbol::Sort>({s1,s2}));
}

