// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./atom.h>

TEST(term_test, actions) {
  StdName n1 = Term::CreateStdName(1, 1);
  StdName n2 = Term::CreateStdName(2, 1);
  StdName n3 = Term::CreateStdName(3, 1);
  Variable x1 = Term::CreateVariable(1);
  Variable x2 = Term::CreateVariable(1);
  Variable x3 = Term::CreateVariable(1);
  Atom a({n1, n2, x1}, 1, {n3, x2, x3});

  EXPECT_TRUE(a.PrependAction(n2).PrependAction(n1).z() == (std::vector<Term>{n1,n2,n1,n2,x1}));
  EXPECT_TRUE(a.PrependAction(n2).PrependAction(n1).z() != (std::vector<Term>{n2,n1,n1,n2,x1}));
  EXPECT_TRUE(a.PrependActions({n1,n2}).z() == (std::vector<Term>{n1,n2,n1,n2,x1}));
  EXPECT_TRUE(a.AppendAction(n2).AppendAction(n1).z() == (std::vector<Term>{n1,n2,x1,n2,n1}));
  EXPECT_TRUE(a.AppendActions({n2,n1}).z() == (std::vector<Term>{n1,n2,x1,n2,n1}));
}

TEST(term_test, less) {
  StdName n1 = Term::CreateStdName(1, 1);
  StdName n2 = Term::CreateStdName(2, 1);
  StdName n3 = Term::CreateStdName(3, 1);
  Variable x1 = Term::CreateVariable(1);
  Variable x2 = Term::CreateVariable(1);
  Variable x3 = Term::CreateVariable(1);
  Atom a({n1, n2}, 1, {n3, x2, x3});
  Atom b({n1, n2, x1}, 1, {n3, x2, x3});
  Atom c({n1, n2, x1}, 1, {n3, x2, n3});

  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(b < c);
  EXPECT_FALSE(c < b);
  EXPECT_TRUE(a < c);
  EXPECT_FALSE(c < a);
}

TEST(term_test, unification) {
  StdName n1 = Term::CreateStdName(1, 1);
  StdName n2 = Term::CreateStdName(2, 1);
  StdName n3 = Term::CreateStdName(3, 1);
  Variable x1 = Term::CreateVariable(1);
  Variable x2 = Term::CreateVariable(1);
  Variable x3 = Term::CreateVariable(1);
  Atom a({x1, x2, x3}, 1, {x1, x2, x3});
  Atom b({n1, n2, n3}, 1, {n1, n2, n3});
  Atom c({n1, n2, n3}, 2, {n1, n2, n3});
  { Unifier theta;
    EXPECT_FALSE(Atom::Unify(a, c, &theta)); }
  { Unifier theta;
    EXPECT_FALSE(Atom::Unify(b, c, &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Atom::Unify(a, b, &theta));
    EXPECT_TRUE(a.Substitute(theta) == b); }
}

