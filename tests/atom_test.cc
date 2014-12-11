// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./atom.h>

using namespace lela;

inline bool operator<(const Atom& a, const Atom& b) {
  return Atom::Comparator()(a, b);
}

TEST(atom_test, actions) {
  Term::Factory f;
  StdName n1 = f.CreateStdName(1, 1);
  StdName n2 = f.CreateStdName(2, 1);
  StdName n3 = f.CreateStdName(3, 1);
  Variable x1 = f.CreateVariable(1);
  Variable x2 = f.CreateVariable(1);
  Variable x3 = f.CreateVariable(1);
  Atom a({n1, n2, x1}, 123, {n3, x2, x3});

  EXPECT_TRUE(a.pred() == 123);
  EXPECT_TRUE(a.PrependActions({n1,n2}).z() == (TermSeq{n1,n2,n1,n2,x1}));
  EXPECT_TRUE(a.PrependActions({n1,n2}).z() != (TermSeq{n2,n1,n1,n2,x1}));
}

TEST(atom_test, less) {
  Term::Factory f;
  StdName n1 = f.CreateStdName(1, 1);
  StdName n2 = f.CreateStdName(2, 1);
  StdName n3 = f.CreateStdName(3, 1);
  Variable x1 = f.CreateVariable(1);
  Variable x2 = f.CreateVariable(1);
  Variable x3 = f.CreateVariable(1);
  Atom a({n1, n2}, 123, {n3, x2, x3});
  Atom b({n1, n2, x1}, 123, {n3, x2, x3});
  Atom c({n1, n2, x1}, 123, {n3, x2, n3});

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
  Atom a({x1, x2, x3}, 123, {x1, x2, x3});
  Atom b({n1, n2, n3}, 123, {n1, n2, n3});
  Atom c({n1, n2, n3}, 41, {n1, n2, n3});
  Atom d({x2, x1, x2}, 123, {x1, x2, x3});
  Atom e({x2, x1, n3}, 123, {n1, n2, n3});
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

