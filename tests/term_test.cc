// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./term.h>

using namespace esbl;

TEST(term_test, dummy) {
  Term t;
  EXPECT_TRUE(!t.is_variable());
  EXPECT_TRUE(!t.is_name());
}

TEST(term_test, variable_stdname) {
  Term::Factory f;
  Variable x = f.CreateVariable(1);
  Term xt = x;
  Term d;
  StdName n = f.CreateStdName(1, 1);
  Term nt = n;
  EXPECT_TRUE(x == xt);
  EXPECT_TRUE(xt == x);
  EXPECT_TRUE(x != d);
  EXPECT_TRUE(n != d);
  EXPECT_TRUE(x != n);
  EXPECT_TRUE(n != x);
  EXPECT_TRUE(n == nt);
  EXPECT_TRUE(nt == n);
  EXPECT_TRUE(xt != nt);
  EXPECT_TRUE(xt < nt);
  EXPECT_TRUE(x < n);
  EXPECT_FALSE(x.is_ground());
  EXPECT_FALSE(xt.is_ground());
  EXPECT_TRUE(x.is_variable());
  EXPECT_TRUE(xt.is_variable());
  EXPECT_FALSE(x.is_name());
  EXPECT_FALSE(xt.is_name());
  EXPECT_TRUE(n.is_ground());
  EXPECT_TRUE(nt.is_ground());
  EXPECT_FALSE(n.is_variable());
  EXPECT_FALSE(nt.is_variable());
  EXPECT_TRUE(n.is_name());
  EXPECT_TRUE(nt.is_name());
  EXPECT_FALSE(nt < xt);
  EXPECT_FALSE(n < x);
  EXPECT_EQ((nt < xt), !(xt < nt));
  EXPECT_EQ((n < x), !(x < n));
  EXPECT_EQ((nt < xt), (n < x));
  EXPECT_EQ((x < nt), (x < n));
  EXPECT_EQ(sizeof(x), sizeof(xt));
  EXPECT_EQ(sizeof(n), sizeof(nt));
  EXPECT_EQ(sizeof(x), sizeof(n));
}

TEST(term_test, substitution) {
  Term::Factory f;
  const Variable x = f.CreateVariable(1);
  const Variable y = f.CreateVariable(1);
  const StdName m = f.CreateStdName(1, 1);
  const StdName n = f.CreateStdName(2, 1);
  const Unifier theta{ { x, m }, { y, n } };
  EXPECT_FALSE(x == y);
  EXPECT_FALSE(n == m);
  EXPECT_FALSE(x == m);
  EXPECT_FALSE(y == n);
  EXPECT_FALSE(x == n);
  EXPECT_FALSE(y == m);
  EXPECT_TRUE(x.Substitute(theta) == m);
  EXPECT_FALSE(x.Substitute(theta) == n);
  EXPECT_FALSE(x == m);
  EXPECT_TRUE(y.Substitute(theta) == n);
  EXPECT_FALSE(y.Substitute(theta) == m);
  EXPECT_FALSE(y == n);
  EXPECT_TRUE(m.Substitute(theta) == m);
  EXPECT_FALSE(m.Substitute(theta) == n);
  EXPECT_FALSE(m == n);
  EXPECT_TRUE(n.Substitute(theta) == n);
  EXPECT_FALSE(n.Substitute(theta) == m);
  EXPECT_FALSE(m == n);
}

TEST(term_test, unification) {
  Term::Factory f;
  const Variable x = f.CreateVariable(1);
  const Variable y = f.CreateVariable(1);
  const StdName m = f.CreateStdName(1, 1);
  const StdName n = f.CreateStdName(2, 1);
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(m, m, &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(m, m, &theta)); }
  { Unifier theta;
    EXPECT_FALSE(Term::Unify(m, n, &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(m, m, &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(x, y, &theta));
    EXPECT_TRUE(x != y);
    EXPECT_TRUE(x.Substitute(theta) == y.Substitute(theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(m, x, &theta));
    EXPECT_TRUE(m != x);
    EXPECT_TRUE(m == x.Substitute(theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(m, x, &theta));
    EXPECT_TRUE(m != x);
    EXPECT_TRUE(m == x.Substitute(theta));
    EXPECT_FALSE(Term::Unify(n, x.Substitute(theta), &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Term::Unify(x, y, &theta));
    EXPECT_TRUE(Term::Unify(y, m, &theta));
    EXPECT_TRUE(x != y);
    EXPECT_TRUE(y != m);
    EXPECT_TRUE(x != m);
    EXPECT_TRUE(x.Substitute(theta) == y.Substitute(theta));
    EXPECT_TRUE(y.Substitute(theta) == m.Substitute(theta));
    EXPECT_TRUE(y.Substitute(theta) == m);
    EXPECT_TRUE(x.Substitute(theta) == m); }
}

TEST(term_test, sequence) {
  Term::Factory f;
  Variable x1 = f.CreateVariable(1);
  Variable x2 = f.CreateVariable(2);
  Variable x3 = f.CreateVariable(1);
  StdName n1 = f.CreateStdName(1, 1);
  StdName n2 = f.CreateStdName(2, 2);
  StdName n3 = f.CreateStdName(3, 1);
  TermSeq z1({x1,x2,x3});
  TermSeq z2({n1,n2,n3});
  TermSeq z3({x1,x2,x1});
  TermSeq z4({n1,n2,n1});
  TermSeq z5({x1,x2,x1,x2});
  TermSeq z6({n1,n2,n1,n2});

  { Unifier t; EXPECT_FALSE(z1.Matches(z2, &t)); }
  { Unifier t; EXPECT_TRUE(z1.Matches(z1, &t)); EXPECT_TRUE(z1.Matches(z1, &t)); }

  { Unifier t; EXPECT_TRUE(z2.Matches(z2, &t)); }
  { Unifier t; EXPECT_TRUE(z2.Matches(z1, &t)); }
  { Unifier t; EXPECT_TRUE(z2.Matches(z1, &t)); EXPECT_TRUE(z2.Matches(z2, &t)); }
  { Unifier t; EXPECT_TRUE(z2.Matches(z2, &t)); EXPECT_TRUE(z2.Matches(z1, &t)); }
  { Unifier t; EXPECT_FALSE(z2.Matches(z3, &t)); }
  { Unifier t; EXPECT_FALSE(z2.Matches(z4, &t)); }
  { Unifier t; EXPECT_FALSE(z2.Matches(z5, &t)); }
  { Unifier t; EXPECT_FALSE(z2.Matches(z6, &t)); }

  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z1, z2, &t)); }
  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z1, z1, &t)); EXPECT_TRUE(TermSeq::Unify(z1, z1, &t)); }

  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z2, z2, &t)); }
  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z2, z1, &t)); }
  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z2, z1, &t)); EXPECT_TRUE(TermSeq::Unify(z2, z2, &t)); }
  { Unifier t; EXPECT_TRUE(TermSeq::Unify(z2, z2, &t)); EXPECT_TRUE(TermSeq::Unify(z2, z1, &t)); }
  { Unifier t; EXPECT_FALSE(TermSeq::Unify(z2, z3, &t)); }
  { Unifier t; EXPECT_FALSE(TermSeq::Unify(z2, z4, &t)); }
  { Unifier t; EXPECT_FALSE(TermSeq::Unify(z2, z5, &t)); }
  { Unifier t; EXPECT_FALSE(TermSeq::Unify(z2, z6, &t)); }

  {
    Unifier theta;
    EXPECT_FALSE(z1 == z3);
    EXPECT_TRUE(z1.Matches(z3, &theta));
    TermSeq zz3 = z3;
    for (Term& t : zz3) {
      t = t.Substitute(theta);
    }
    EXPECT_FALSE(z1 == zz3);
  }

  {
    Unifier theta;
    EXPECT_FALSE(z1 == z3);
    EXPECT_TRUE(z3.Matches(z1, &theta));
    TermSeq zz1 = z1;
    for (Term& t : zz1) {
      t = t.Substitute(theta);
    }
    EXPECT_TRUE(zz1 == z3);
  }

  {
    Unifier theta;
    EXPECT_FALSE(z1 == z3);
    EXPECT_TRUE(TermSeq::Unify(z1, z3, &theta));
    TermSeq zz1 = z1;
    for (Term& t : zz1) {
      t = t.Substitute(theta);
    }
    TermSeq zz3 = z3;
    for (Term& t : zz3) {
      t = t.Substitute(theta);
    }
    EXPECT_TRUE(zz1 == zz3);
  }

  {
    Unifier theta;
    EXPECT_FALSE(z1 == z3);
    EXPECT_TRUE(TermSeq::Unify(z3, z1, &theta));
    TermSeq zz1 = z1;
    for (Term& t : zz1) {
      t = t.Substitute(theta);
    }
    TermSeq zz3 = z3;
    for (Term& t : zz3) {
      t = t.Substitute(theta);
    }
    EXPECT_TRUE(zz1 == zz3);
  }

  for (const auto& z : {z1,z2,z3,z4,z5,z6}) {
    for (const auto& zz : {z1,z2,z3,z4,z5,z6}) {
      Unifier t;
      Unifier t1;
      Unifier t2;
      EXPECT_EQ(TermSeq::Unify(z, zz, &t1), TermSeq::Unify(z, zz, &t2));
      EXPECT_EQ(TermSeq::Unify(z, zz, &t), TermSeq::Unify(z, zz, &t));
    }
  }

  EXPECT_TRUE(z5.WithoutLast(2).first);
  EXPECT_EQ(z5.WithoutLast(2).second, TermSeq({x1,x2}));
  EXPECT_TRUE(z6.WithoutLast(z5.size()).first);
  EXPECT_EQ(z6.WithoutLast(z5.size()).second, TermSeq({}));
  EXPECT_FALSE(z4.WithoutLast(z5.size()).first);

  EXPECT_TRUE(z6.WithoutLast(1).first);
  EXPECT_EQ(z6.WithoutLast(1).second, TermSeq({n1,n2,n1}));
}

