// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./atom.h>

using namespace lela;

inline bool operator<(const Atom& a, const Atom& b) {
  return Atom::Comparator()(a, b);
}

TEST(atom_test, less) {
  Term::Factory f;
  StdName n1 = f.CreateStdName(1, 1);
  StdName n2 = f.CreateStdName(2, 1);
  StdName n3 = f.CreateStdName(3, 1);
  Variable x1 = f.CreateVariable(1);
  Variable x2 = f.CreateVariable(1);
  Variable x3 = f.CreateVariable(1);
  Atom a(12, {n1, n2, n3, x2, x3});
  Atom b(123, {n1, n2, x1, n3, x2, x3});
  Atom c(123, {n1, n2, x1, n3, x2, n3});

  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(b < c);
  EXPECT_FALSE(c < b);
  EXPECT_TRUE(a < c);
  EXPECT_FALSE(c < a);
}

TEST(atom_test, unification) {
  Term::Factory f;
  StdName n1 = f.CreateStdName(1, 1);
  StdName n2 = f.CreateStdName(2, 1);
  StdName n3 = f.CreateStdName(3, 1);
  Variable x1 = f.CreateVariable(1);
  Variable x2 = f.CreateVariable(1);
  Variable x3 = f.CreateVariable(1);
  Atom a(123, {x1, x2, x3, x1, x2, x3});
  Atom b(123, {n1, n2, n3, n1, n2, n3});
  Atom c(41,  {n1, n2, n3, n1, n2, n3});
  Atom d(123, {x2, x1, x2, x1, x2, x3});
  Atom e(123, {x2, x1, n3, n1, n2, n3});
  { Unifier theta;
    EXPECT_FALSE(Atom::Unify(a, c, &theta)); }
  { Unifier theta;
    EXPECT_FALSE(Atom::Unify(b, c, &theta)); }
  { Unifier theta;
    EXPECT_TRUE(Atom::Unify(a, b, &theta));
    EXPECT_TRUE(a.Substitute(theta) == b); }
  { Unifier theta;
    EXPECT_TRUE(Atom::Unify(a, d, &theta));
    EXPECT_FALSE(a.Substitute(theta) == d);
    EXPECT_FALSE(a == d.Substitute(theta));
    std::cout << a << std::endl;
    std::cout << d << std::endl;
    std::cout << theta << std::endl;
    EXPECT_TRUE(a.Substitute(theta) == d.Substitute(theta)); }
  { Unifier theta;
    EXPECT_FALSE(Atom::Unify(e, a, &theta));
    EXPECT_FALSE(Atom::Unify(a, e, &theta)); }
}

