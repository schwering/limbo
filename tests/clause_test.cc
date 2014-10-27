// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./clause.h>

using namespace esbl;

static Term::Factory f;
static StdName n0 = f.CreateStdName(0, 1);
static StdName n1 = f.CreateStdName(1, 1);
static StdName n2 = f.CreateStdName(2, 1);
static StdName n3 = f.CreateStdName(3, 1);
static StdName n4 = f.CreateStdName(4, 1);
static StdName n5 = f.CreateStdName(5, 1);
static StdName n6 = f.CreateStdName(6, 1);
static Variable x0 = f.CreateVariable(1);
static Variable x1 = f.CreateVariable(1);
static Variable x2 = f.CreateVariable(1);
static Variable x3 = f.CreateVariable(1);
static Variable x4 = f.CreateVariable(1);
static Variable x5 = f.CreateVariable(1);
static Variable x6 = f.CreateVariable(1);

static std::set<StdName> names{n0,n1,n2,n3,n4,n5,n6};
static StdName::SortedSet hplus{{1, names}};

static Atom::PredId P = 1;
static Atom::PredId Q = 2;

TEST(clause, rel)
{
  Clause empty(true, Ewff::TRUE, GroundClause());
  Clause c1(true,
            Ewff({ Ewff::Conj({{x1,n1}},
                              {{x2,x3}},
                              {{x2,n2}, {x3,n3}},
                              {{x1,x2}})}),
            GroundClause({ Literal({x3}, true, P, {x1,x2}),
                           Literal({x3}, false, P, {x1,x2}) }));
  Clause c2(false,
            Ewff({ Ewff::Conj({},
                              {{x4,x5}},
                              {},
                              {{x5,x6}})}),
            GroundClause({ Literal({x4}, true, P, {x5,x6}),
                           Literal({x6}, false, Q, {x4,x5}) }));
  Clause c3(false,
            Ewff({ Ewff::Conj({},
                              {{x1,x4}, {x2,x5}, {x3,x3}, {x6,x6}},  // all vars occur in ewff
                              {},
                              {})}),
            GroundClause({ Literal({x1}, true, P, {x2}),
                           Literal({x4}, false, Q, {x5,x6}) }));
  Clause c4(false,
            Ewff({ Ewff::Conj({},
                              {{x1,x4}, {x2,x5}},  // same as c3, but x3, x6 do not occur
                              {},
                              {})}),
            GroundClause({ Literal({x1}, true, P, {x2}),
                           Literal({x4}, false, Q, {x5,x6}) }));

  EXPECT_EQ(empty.Rel(hplus, Literal({n2,n4}, false, P, {n1,n4})).size(), 0);
  EXPECT_EQ(empty.Rel(hplus, Literal({n2,n4}, true, P, {n1,n4})).size(), 0);
  EXPECT_EQ(c1.Rel(hplus, Literal({n2,n4}, false, P, {n1,n4})).size(), 1);
  EXPECT_EQ(c1.Rel(hplus, Literal({n2,n4}, true, P, {n1,n4})).size(), 1);
  EXPECT_EQ(c2.Rel(hplus, Literal({n2,n4}, false, P, {n1,n4})).size(), 0);
  EXPECT_EQ(c2.Rel(hplus, Literal({n2,n4}, true, P, {n1,n4})).size(), 0);
  EXPECT_EQ(c2.Rel(hplus, Literal({n2}, false, P, {n2,n4})).size(), 0);
  EXPECT_EQ(c2.Rel(hplus, Literal({n2}, true, P, {n2,n4})).size(), 1);
  EXPECT_EQ(c3.Rel(hplus, Literal({n1}, false, P, {n2})).size(), 0);
  EXPECT_EQ(c3.Rel(hplus, Literal({n1}, true, P, {n2})).size(), 7);
  for (const StdName& n : names) {
    EXPECT_EQ(c3.Rel(hplus, Literal({n1}, true, P, {n2})).count(Literal({n1}, true, Q, {n2,n})), 1);
  }
  EXPECT_EQ(c4.Rel(hplus, Literal({n1}, false, P, {n2})).size(), 0);
  EXPECT_EQ(c4.Rel(hplus, Literal({n1}, true, P, {n2})).size(), 1);
  EXPECT_EQ(c4.Rel(hplus, Literal({n1}, false, Q, {n2,x3})).size(), 1);
  EXPECT_EQ(c4.Rel(hplus, Literal({n1}, false, Q, {n2,x6})).size(), 1);
}

TEST(clause, subsumption)
{
  Clause empty(true, Ewff::TRUE, GroundClause());
  Clause c1(true,
            Ewff({ Ewff::Conj({{x1,n1}},
                              {{x2,x3}},
                              {{x2,n2}, {x3,n3}},
                              {{x1,x2}})}),
            GroundClause({ Literal({x3}, true, P, {x1,x2}),
                           Literal({x3}, false, P, {x1,x2}) }));
  Clause c2(false,
            Ewff({ Ewff::Conj({},
                              {{x4,x5}},
                              {},
                              {{x5,x6}})}),
            GroundClause({ Literal({x4}, true, P, {x5,x6}),
                           Literal({x6}, false, Q, {x4,x5}) }));
  GroundClause d1({Literal({n2,n4}, true, P, {n1,n4}),
                   Literal({n2,n4}, false, P, {n1,n4})});
  GroundClause d2({Literal({n4}, true, P, {n4,n6}),
                   Literal({n6}, false, Q, {n4,n4})});

  EXPECT_TRUE(empty.Subsumes(d1));
  EXPECT_TRUE(empty.Subsumes(d2));
  EXPECT_TRUE(c1.Subsumes(d1));
  EXPECT_TRUE(c2.Subsumes(d2));
  EXPECT_FALSE(c1.Subsumes(d2));
  EXPECT_FALSE(c2.Subsumes(d1));
}

