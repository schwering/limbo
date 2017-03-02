// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include <lela/setup.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format;

template<typename T>
size_t dist(T r) { return std::distance(r.begin(), r.end()); }

TEST(SetupTest, Subsumes_Consistent_clauses) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort s1 = sf.CreateSort(); RegisterSort(s1, "");
  const Term n = tf.CreateTerm(Symbol::Factory::CreateName(1, s1));
  const Term m = tf.CreateTerm(Symbol::Factory::CreateName(2, s1));
  const Term a = tf.CreateTerm(Symbol::Factory::CreateFunction(1, s1, 0), {});
  const Term fn = tf.CreateTerm(Symbol::Factory::CreateFunction(3, s1, 1), {n});
  const Term fm = tf.CreateTerm(Symbol::Factory::CreateFunction(3, s1, 1), {m});
  const Term gn = tf.CreateTerm(Symbol::Factory::CreateFunction(4, s1, 1), {n});
  const Term gm = tf.CreateTerm(Symbol::Factory::CreateFunction(4, s1, 1), {m});

  {
    lela::Setup s0;
    EXPECT_EQ(s0.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)})), lela::Setup::kOk);
    EXPECT_EQ(s0.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)})), lela::Setup::kOk);
    EXPECT_TRUE(s0.Consistent());
    EXPECT_TRUE(s0.LocallyConsistent({fm,fn}));
    for (size_t i : s0.clauses()) {
      EXPECT_TRUE(s0.Subsumes(s0.clause(i)));
    }
    EXPECT_FALSE(s0.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

    {
      lela::Setup& s1 = s0;
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)})), lela::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)})), lela::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(fn,n)})), lela::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(gn,n)})), lela::Setup::kOk);
      EXPECT_EQ(dist(s1.clauses()), 6);
      s1.Minimize();
      EXPECT_EQ(dist(s1.clauses()), 4);
      EXPECT_TRUE(!s1.Consistent());
      for (const size_t i : s1.clauses()) {
        EXPECT_TRUE(s1.Subsumes(s1.clause(i)));
      }
      EXPECT_FALSE(s1.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

      {
        lela::Setup& s2 = s1;
        EXPECT_EQ(s2.AddClause(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})), lela::Setup::kOk);
        EXPECT_EQ(dist(s2.clauses()), 5);
        EXPECT_TRUE(!s2.Consistent());
        for (const size_t i : s2.clauses()) {
          EXPECT_TRUE(s2.Subsumes(s2.clause(i)));
        }

        {
          lela::Setup& s3 = s2;
          EXPECT_EQ(s3.AddClause(Clause({Literal::Neq(a,m)})), lela::Setup::kOk);
          EXPECT_EQ(dist(s3.clauses()), 5+1+1+2+2);
          s3.Minimize();
          EXPECT_EQ(dist(s3.clauses()), 5);
          //EXPECT_TRUE(s3.Consistent());
        }
      }
    }
  }
}

}  // namespace lela

