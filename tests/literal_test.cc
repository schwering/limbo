// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./literal.h>

using namespace esbl;

TEST(literal_test, less) {
  StdName n1 = Term::CreateStdName(1, 1);
  StdName n2 = Term::CreateStdName(2, 1);
  StdName n3 = Term::CreateStdName(3, 1);
  Variable x1 = Term::CreateVariable(1);
  Variable x2 = Term::CreateVariable(1);
  Variable x3 = Term::CreateVariable(1);
  Literal a({n1, n2}, true, 123, {n3, x2, x3});
  Literal b({n1, n2, x1}, false, 123, {n3, x2, x3});
  Literal c({n1, n2, x1}, false, 123, {n3, x2, n3});

  EXPECT_TRUE(a.sign() == true);
  EXPECT_TRUE(b.sign() == false);
  EXPECT_TRUE(c.sign() == false);

  EXPECT_TRUE(a.Flip().sign() == false);
  EXPECT_TRUE(b.Flip().sign() == true);
  EXPECT_TRUE(c.Flip().sign() == true);

  EXPECT_TRUE(a.Positive().sign() == true);
  EXPECT_TRUE(b.Positive().sign() == true);
  EXPECT_TRUE(c.Positive().sign() == true);

  EXPECT_TRUE(a.Negative().sign() == false);
  EXPECT_TRUE(b.Negative().sign() == false);
  EXPECT_TRUE(c.Negative().sign() == false);

  EXPECT_TRUE(a.Flip() != a);
  EXPECT_TRUE(a.Positive() == a);
  EXPECT_TRUE(a.Negative() != a);

  EXPECT_TRUE(b.Flip() != b);
  EXPECT_TRUE(b.Positive() != b);
  EXPECT_TRUE(b.Negative() == b);

  EXPECT_TRUE(c.Flip() != c);
  EXPECT_TRUE(c.Positive() != c);
  EXPECT_TRUE(c.Negative() == c);

  EXPECT_TRUE(a.Negative() < a.Positive());
  EXPECT_TRUE(b.Negative() < b.Positive());
  EXPECT_TRUE(c.Negative() < c.Positive());

  EXPECT_TRUE((static_cast<Atom>(a) < static_cast<Atom>(b)) == (a < b));

  EXPECT_TRUE((static_cast<Atom>(a) == static_cast<Atom>(a.Flip())) && (a != b));
  EXPECT_TRUE((static_cast<const Atom&>(a) == static_cast<const Atom&>(a.Flip())) && (a != b));
}

