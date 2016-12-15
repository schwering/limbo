// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include <lela/setup.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format::output;

template<typename T>
size_t dist(T r) { return std::distance(r.begin(), r.end()); }

TEST(SetupTest, Subsumes_Consistent_clauses) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort(); RegisterSort(s1, "");
  //const Symbol::Sort s2 = sf.CreateSort();
  const Term n = tf.CreateTerm(Symbol::Factory::CreateName(1, s1));
  const Term m = tf.CreateTerm(Symbol::Factory::CreateName(2, s1));
  const Term a = tf.CreateTerm(Symbol::Factory::CreateFunction(1, s1, 0), {});
  //const Term b = tf.CreateTerm(Symbol::Factory::CreateFunction(2, s1, 0), {});
  const Term fn = tf.CreateTerm(Symbol::Factory::CreateFunction(3, s1, 1), {n});
  const Term fm = tf.CreateTerm(Symbol::Factory::CreateFunction(3, s1, 1), {m});
  const Term gn = tf.CreateTerm(Symbol::Factory::CreateFunction(4, s1, 1), {n});
  const Term gm = tf.CreateTerm(Symbol::Factory::CreateFunction(4, s1, 1), {m});

  {
    lela::Setup s0;
    EXPECT_EQ(std::distance(s0.clauses().begin(), s0.clauses().end()), 0);
    s0.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)}));
    s0.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)}));
#if 0
    s0.Init();
    EXPECT_EQ(dist(s0.clauses()), 2);
    auto pts = s0.primitive_terms();
    EXPECT_EQ(std::set<Term>(pts.begin(), pts.end()).size(), 4);
    EXPECT_EQ(dist(s0.clauses_with(a)), 0);
    EXPECT_EQ(dist(s0.clauses_with(fn)), 1);
    EXPECT_EQ(dist(s0.clauses_with(fm)), 1);
#endif
    EXPECT_TRUE(s0.Consistent());
    EXPECT_TRUE(s0.LocallyConsistent(Clause({Literal::Eq(fm,m)})));
    //EXPECT_TRUE(s0.LocallyConsistent(Clause({Literal::Neq(fm,m)})));
    EXPECT_TRUE(!s0.LocallyConsistent(Clause({Literal::Eq(fn,n)})));
    for (auto i : s0.clauses()) {
      EXPECT_TRUE(s0.Subsumes(s0.clause(i)));
    }
    EXPECT_FALSE(s0.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

    {
      lela::Setup s1(&s0);
      s1.AddClause(Clause({Literal::Neq(fn,n), Literal::Eq(fm,m)}));
      s1.AddClause(Clause({Literal::Neq(gn,n), Literal::Eq(gm,m)}));
      s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(fn,n)}));
      s1.AddClause(Clause({Literal::Neq(a,n), Literal::Eq(gn,n)}));
#if 0
      s1.Init();
#endif
      EXPECT_EQ(dist(s1.clauses()), 4);
      //std::cout << s0 << std::endl;
      //std::cout << s1 << std::endl;
      //print_range(std::cout, s0.primitive_terms()) << std::endl;
      //print_range(std::cout, s1.primitive_terms()) << std::endl;
#if 0
      auto pts = s1.primitive_terms();
      EXPECT_EQ(std::set<Term>(pts.begin(), pts.end()).size(), 4+1);
      EXPECT_EQ(dist(s1.clauses()), 4);
      EXPECT_EQ(dist(s1.clauses_with(a)), 2);
      EXPECT_EQ(dist(s1.clauses_with(fn)), 2);
      EXPECT_EQ(dist(s1.clauses_with(fm)), 1);
#endif
      EXPECT_TRUE(!s1.Consistent());
      for (auto i : s1.clauses()) {
        EXPECT_TRUE(s1.Subsumes(s1.clause(i)));
      }
      EXPECT_FALSE(s1.Subsumes(Clause({Literal::Eq(a,m), Literal::Eq(a,n)})));

      {
        lela::Setup s2(&s1);
        s2.AddClause(Clause({Literal::Eq(a,m), Literal::Eq(a,n)}));
#if 0
        s2.Init();
#endif
        //std::cout << s0 << std::endl;
        //std::cout << s1 << std::endl;
        //std::cout << s2 << std::endl;
        //print_range(std::cout, s0.primitive_terms()) << std::endl;
        //print_range(std::cout, s1.primitive_terms()) << std::endl;
        //print_range(std::cout, s2.primitive_terms()) << std::endl;
        EXPECT_EQ(dist(s2.clauses()), 5);
#if 0
        auto pts = s2.primitive_terms();
        EXPECT_EQ(std::set<Term>(pts.begin(), pts.end()).size(), 4+1+0);
        EXPECT_EQ(dist(s2.clauses_with(a)), 3);
        EXPECT_EQ(dist(s2.clauses_with(fn)), 2);
        EXPECT_EQ(dist(s2.clauses_with(fm)), 1);
#endif
        EXPECT_TRUE(!s2.Consistent());
        for (auto i : s2.clauses()) {
          EXPECT_TRUE(s2.Subsumes(s2.clause(i)));
        }

        {
          lela::Setup s3(&s2);
          s3.AddClause(Clause({Literal::Neq(a,m)}));
#if 0
          s3.Init();
#endif
          EXPECT_EQ(dist(s3.clauses()), 5); // again, the unique-filter catches 'a' by chance
#if 0
          auto pts = s3.primitive_terms();
          EXPECT_EQ(std::set<Term>(pts.begin(), pts.end()).size(), 4+1+0);
          EXPECT_EQ(dist(s3.clauses_with(a)), 1);
          EXPECT_EQ(dist(s3.clauses_with(fn)), 1);
          EXPECT_EQ(dist(s3.clauses_with(fm)), 1);
#endif
          EXPECT_TRUE(s3.Consistent());
        }
      }
    }
  }
}

}  // namespace lela

