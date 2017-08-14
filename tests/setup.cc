// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include <limbo/setup.h>
#include <limbo/format/output.h>

namespace limbo {

using namespace limbo::format;

typedef std::vector<Clause> ClauseVector;
typedef std::unordered_set<Clause> ClauseSet;

ClauseVector V(const Setup& s) {
  ClauseVector vec;
  for (auto i : s.clauses()) {
    const internal::Maybe<Clause> c = s.clause(i);
    if (c) {
      vec.push_back(c.val);
    }
  }
  return vec;
}

ClauseSet S(const Setup& s) {
  ClauseSet set;
  for (auto i : s.clauses()) {
    const internal::Maybe<Clause> c = s.clause(i);
    if (c) {
      set.insert(c.val);
    }
  }
  return set;
}

size_t length(const Setup& s) { return V(s).size(); }
size_t unique_length(const Setup& s) { return S(s).size(); }

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
    limbo::Setup s0;
    EXPECT_EQ(s0.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)})), limbo::Setup::kOk);
    EXPECT_EQ(s0.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)})), limbo::Setup::kOk);
    EXPECT_TRUE(s0.Consistent());
    for (auto i : s0.clauses()) {
      EXPECT_TRUE(s0.clause(i));
      EXPECT_TRUE(s0.Subsumes(s0.clause(i).val));
    }
    EXPECT_FALSE(s0.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

    {
      limbo::Setup& s1 = s0;
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)})), limbo::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)})), limbo::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(fn,n)})), limbo::Setup::kOk);
      EXPECT_EQ(s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(gn,n)})), limbo::Setup::kOk);
      EXPECT_EQ(length(s1), 6);
      EXPECT_EQ(unique_length(s1), 4);
      s1.Minimize();
      EXPECT_EQ(length(s1), 6);
      EXPECT_EQ(unique_length(s1), 4);
      EXPECT_TRUE(!s1.Consistent());
      for (const auto i : s1.clauses()) {
        EXPECT_TRUE(s1.clause(i));
        EXPECT_TRUE(s1.Subsumes(s1.clause(i).val));
      }
      EXPECT_FALSE(s1.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

      {
        limbo::Setup& s2 = s1;
        EXPECT_EQ(s2.AddClause(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})), limbo::Setup::kOk);
        EXPECT_EQ(length(s2), 7);
        EXPECT_EQ(unique_length(s2), 5);
        EXPECT_TRUE(!s2.Consistent());
        for (const auto i : s2.clauses()) {
          EXPECT_TRUE(s2.clause(i));
          EXPECT_TRUE(s2.Subsumes(s2.clause(i).val));
        }

        {
          limbo::Setup& s3 = s2;
          EXPECT_EQ(s3.AddClause(Clause({Literal::Neq(a,m)})), limbo::Setup::kOk);
          EXPECT_EQ(length(s3), 5);
          EXPECT_EQ(unique_length(s3), 5);
          s3.Minimize();
          EXPECT_EQ(length(s3), 5);
          EXPECT_EQ(unique_length(s3), 5);
          //EXPECT_TRUE(s3.Consistent());
        }
      }
    }
  }
}

}  // namespace limbo

