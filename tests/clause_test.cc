// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include <./clause.h>
#include <./maybe.h>
#include <./print.h>

using namespace lela;

struct EqSubstitute {
  EqSubstitute(Term pre, Term post) : pre_(pre), post_(post) {}
  Maybe<Term> operator()(Term t) const { if (t == pre_) return Just(post_); else return Nothing; }

 private:
  const Term pre_;
  const Term post_;
};

TEST(clause_test, symbol) {
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

  EXPECT_TRUE(Clause({Literal::Eq(n1,n1)}).valid());
  EXPECT_TRUE(!Clause({Literal::Neq(n1,n1)}).valid());
  EXPECT_TRUE(Clause({Literal::Eq(f1,f1)}).valid());
  EXPECT_TRUE(!Clause({Literal::Neq(f1,f1)}).valid());
  EXPECT_TRUE(!Clause({Literal::Neq(f1,n1)}).valid());
  EXPECT_TRUE(!Clause({Literal::Neq(f1,f2)}).valid());
  EXPECT_TRUE(Clause({Literal::Eq(n1,n1), Literal::Eq(n2,n2)}).valid());
  EXPECT_TRUE(Clause({Literal::Eq(n1,n1), Literal::Neq(n2,n2)}).valid());
  EXPECT_TRUE(!Clause({Literal::Neq(n1,n1), Literal::Neq(n2,n2)}).valid());

  EXPECT_TRUE(!Clause({Literal::Eq(n1,n1)}).invalid());
  EXPECT_TRUE(Clause({Literal::Neq(n1,n1)}).invalid());
  EXPECT_TRUE(!Clause({Literal::Eq(f1,f1)}).invalid());
  EXPECT_TRUE(Clause({Literal::Neq(f1,f1)}).invalid());
  EXPECT_TRUE(!Clause({Literal::Neq(f1,n1)}).invalid());
  EXPECT_TRUE(!Clause({Literal::Neq(f1,f2)}).invalid());
  EXPECT_TRUE(!Clause({Literal::Eq(n1,n1), Literal::Eq(n2,n2)}).invalid());
  EXPECT_TRUE(!Clause({Literal::Eq(n1,n1), Literal::Neq(n2,n2)}).invalid());
  EXPECT_TRUE(Clause({Literal::Neq(n1,n1), Literal::Neq(n2,n2)}).invalid());

  {
    Clause c1({Literal::Eq(f1,n1)});
    Clause c2({Literal::Neq(f1,n2)});
    EXPECT_TRUE(c1.Subsumes(c2));
    EXPECT_FALSE(c2.Subsumes(c1));
  }

  {
    Clause c1({Literal::Eq(f1,n1)});
    Clause c2({Literal::Eq(f1,n2)});
    EXPECT_FALSE(c1.Subsumes(c2));
    EXPECT_FALSE(c2.Subsumes(c1));
  }
  {
    Clause c1({Literal::Eq(f1,n1)});
    Clause c2({Literal::Eq(f1,n1)});
    EXPECT_TRUE(c1.Subsumes(c2));
    EXPECT_TRUE(c2.Subsumes(c1));
  }

  {
    Clause c1({Literal::Eq(f1,n1), Literal::Neq(n1,n1)});
    Clause c2({Literal::Eq(f1,n1)});
    EXPECT_TRUE(c1.Subsumes(c2));
    EXPECT_TRUE(c2.Subsumes(c1));
    EXPECT_TRUE(c1 == c2); // because of minimization, n1 != n1 is removed
  }

  {
    Clause c1({Literal::Eq(f1,n1), Literal::Neq(n1,n1)});
    Maybe<Clause> c2;
    c2 = c1.PropagateUnit(Literal::Neq(f1,n1));
    EXPECT_TRUE(c2 && c2.val.empty());
    EXPECT_TRUE(c2.val.Subsumes(c1));
    c2 = c1.PropagateUnit(Literal::Eq(f1,n2));
    EXPECT_TRUE(c2 && c2.val.empty());
    EXPECT_TRUE(c2.val.Subsumes(c1));
    c2 = c1.PropagateUnit(Literal::Eq(f1,n1));
    EXPECT_FALSE(c2);
  }

  {
    Clause c1({Literal::Eq(f4,n1), Literal::Eq(f2,n1)});
    EXPECT_TRUE(c1.size() == 2);
    c1 = c1.Substitute(EqSubstitute(f1,n2));
    EXPECT_TRUE(c1.size() == 2);
    EXPECT_TRUE(!c1.ground());
    c1 = c1.Substitute(EqSubstitute(x2,n2));
    EXPECT_TRUE(c1.size() == 1);
    EXPECT_TRUE(c1.unit());
  }
}

TEST(clause_test2, symbol) {
  {
    const Symbol::Sort s1 = 1;
    //const Symbol::Sort s2 = 2;
    const Term n = Term::Create(Symbol::CreateName(1, s1));
    const Term m = Term::Create(Symbol::CreateName(2, s1));
    const Term a = Term::Create(Symbol::CreateFunction(1, s1, 0), {});
    //const Term b = Term::Create(Symbol::CreateFunction(2, s1, 0), {});
    //const Term fn = Term::Create(Symbol::CreateFunction(3, s1, 1), {n});
    //const Term fm = Term::Create(Symbol::CreateFunction(3, s1, 1), {m});
    //const Term gn = Term::Create(Symbol::CreateFunction(4, s1, 1), {n});
    //const Term gm = Term::Create(Symbol::CreateFunction(4, s1, 1), {m});

    Clause c1({Literal::Eq(a,m), Literal::Eq(a,n)});
    Clause c2({Literal::Neq(a,m)});
    EXPECT_FALSE(c1.Subsumes(c2));
    EXPECT_FALSE(c2.Subsumes(c1));
  }
}
 
