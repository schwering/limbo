// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include <./setup.h>
#include <./print.h>

using namespace lela;

template<typename Iter>
size_t dist(Range<Iter> r) { return std::distance(r.begin(), r.end()); }

TEST(setup_test, symbol) {
  const Symbol::Sort s1 = 1;
  //const Symbol::Sort s2 = 2;
  const Term n = Term::Create(Symbol::CreateName(1, s1));
  const Term m = Term::Create(Symbol::CreateName(2, s1));
  const Term a = Term::Create(Symbol::CreateFunction(1, s1, 0), {});
  //const Term b = Term::Create(Symbol::CreateFunction(2, s1, 0), {});
  const Term fn = Term::Create(Symbol::CreateFunction(3, s1, 1), {n});
  const Term fm = Term::Create(Symbol::CreateFunction(3, s1, 1), {m});
  const Term gn = Term::Create(Symbol::CreateFunction(4, s1, 1), {n});
  const Term gm = Term::Create(Symbol::CreateFunction(4, s1, 1), {m});

  {
    lela::Setup s0;
    EXPECT_EQ(std::distance(s0.clauses().begin(), s0.clauses().end()), 0);
    s0.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)}));
    s0.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)}));
    s0.Init();
    EXPECT_EQ(dist(s0.clauses()), 2);
    EXPECT_EQ(dist(s0.primitive_terms()), 4);
    EXPECT_EQ(dist(s0.clauses_with(a)), 0);
    EXPECT_EQ(dist(s0.clauses_with(fn)), 1);
    EXPECT_EQ(dist(s0.clauses_with(fm)), 1);
    EXPECT_TRUE(!s0.PossiblyInconsistent());
    for (auto i : s0.clauses()) {
      EXPECT_TRUE(s0.Implies(s0.clause(i)));
    }
    EXPECT_FALSE(s0.Implies(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

    lela::Setup s1(&s0);
    s1.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)}));
    s1.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)}));
    s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(fn,n)}));
    s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(gn,n)}));
    s1.Init();
    EXPECT_EQ(dist(s1.clauses()), 4);
    EXPECT_EQ(dist(s1.primitive_terms()), 4+3);
    EXPECT_EQ(dist(s1.clauses()), 4);
    EXPECT_EQ(dist(s1.clauses_with(a)), 2);
    EXPECT_EQ(dist(s1.clauses_with(fn)), 2);
    EXPECT_EQ(dist(s1.clauses_with(fm)), 1);
    EXPECT_TRUE(s1.PossiblyInconsistent());
    for (auto i : s1.clauses()) {
      EXPECT_TRUE(s1.Implies(s1.clause(i)));
    }
    EXPECT_FALSE(s1.Implies(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

    lela::Setup s2(&s1);
    s2.AddClause(Clause({Literal::Eq(a,m), Literal::Eq(a,n)}));
    s2.Init();
    EXPECT_EQ(dist(s2.clauses()), 5);
    EXPECT_EQ(dist(s2.primitive_terms()), 4+3); // the constant 'a' isn't counted in this setup because it's the unique-filter catches it (by chance)
    EXPECT_EQ(dist(s2.clauses_with(a)), 3);
    EXPECT_EQ(dist(s2.clauses_with(fn)), 2);
    EXPECT_EQ(dist(s2.clauses_with(fm)), 1);
    EXPECT_TRUE(s2.PossiblyInconsistent());
    for (auto i : s2.clauses()) {
      EXPECT_TRUE(s2.Implies(s2.clause(i)));
    }

    lela::Setup s3(&s2);
    s3.AddClause(Clause({Literal::Neq(a,m)}));
    for (auto i : s3.clauses()) {
      std::cout << s3.clause(i) << std::endl;
    }
    std::cout << std::endl;
    s3.Init();
    for (auto i : s3.clauses()) {
      std::cout << s3.clause(i) << std::endl;
    }
    EXPECT_EQ(dist(s3.clauses()), 5); // again, the unique-filter catches 'a' by chance
    EXPECT_EQ(dist(s3.primitive_terms()), 4+3);
    EXPECT_EQ(dist(s3.clauses_with(a)), 3);
    EXPECT_EQ(dist(s3.clauses_with(fn)), 2);
    EXPECT_EQ(dist(s3.clauses_with(fm)), 1);
  }
}

