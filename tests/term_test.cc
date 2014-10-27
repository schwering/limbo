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

