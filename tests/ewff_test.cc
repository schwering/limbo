// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./ewff.h>

using namespace lela;

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

static StdName::Set names{n0,n1,n2,n3,n4,n5,n6};
static StdName::SortedSet hplus{{1, names}};

TEST(ewff_test, restrict_vars) {
  const auto p = Ewff::Create(//{{x1,n1}, {x2,n2}},
                              //{{x3,x4}, {x4,x3}},
                              {{x1,n2}, {x1,n3}, {x5,n6}},
                              {{x1,x2}, {x2,x1}, {x5,x6}});
  EXPECT_TRUE(p);
  const Ewff e = p.val;
  Ewff ee = e;
  ee.RestrictVariable({x1,x2});
  EXPECT_EQ(ee, Ewff::Create({{x1,n2}, {x1,n3}}, {{x1,x2}}).val);
}

TEST(ewff_test, subsumption) {
  const auto p = Ewff::Create({{x1,n2}, {x1,n3}, {x5,n6}},
                              {{x1,x2}, {x2,x1}, {x5,x6}});
  const auto q = Ewff::Create({{x1,n2}, {x1,n3}, {x5,n6}, {x4,n4}},
                              {{x1,x2}, {x2,x1}, {x5,x6}, {x3,x4}});
  EXPECT_TRUE(bool(p));
  EXPECT_TRUE(bool(q));
  EXPECT_TRUE(q.val.Subsumes(p.val));
}

TEST(ewff_test, models) {
  const auto p = Ewff::Create(//{{x1,n1}, {x2,n2}},
                              //{{x3,x4}, {x4,x3}},
                              {{x1,n2}, {x1,n3}, {x5,n6}},
                              {{x1,x2}, {x2,x1}, {x5,x6}});
  EXPECT_TRUE(p);
  const Ewff e = p.val;

  {
    Assignment theta{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    Maybe<Ewff> p = e.Ground(theta);
    EXPECT_TRUE(p);
    EXPECT_TRUE(e.SatisfiedBy(theta));
  }
  {
    const std::list<Assignment> models = e.Models(hplus);
    EXPECT_TRUE(models.size() > 0);
    for (const Assignment& theta : models) {
      EXPECT_TRUE(e.SatisfiedBy(theta));
    }
  }
  {
    Assignment theta1{{x1,n1}, {x2,n2}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    Maybe<Ewff> p = e.Ground(theta1);
    EXPECT_TRUE(p);
    EXPECT_TRUE(p.val.SatisfiedBy(theta2));
  }
  {
    Assignment theta1{{x3,n3}, {x5,n5}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    Maybe<Ewff> p = e.Ground(theta1);
    EXPECT_TRUE(p);
    EXPECT_TRUE(p.val.SatisfiedBy(theta2));
  }
  {
    Assignment theta1{{x3,n3}, {x5,n6}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    Maybe<Ewff> p = e.Ground(theta1);
    EXPECT_FALSE(p);
  }
  {
    Assignment theta1{{x3,n3}, {x6,n5}, {x5, n6}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    Maybe<Ewff> p = e.Ground(theta1);
    EXPECT_FALSE(p);
  }
}

TEST(ewff_test, models_completeness) {
  const auto p = Ewff::Create(//{{x1,n1}, {x2,n2}},
                              //{{x3,x4}, {x4,x3}},
                              {{x1,n2}, {x1,n3}, {x5,n6}},
                              {{x1,x2}, {x2,x1}, {x5,x6}});
  EXPECT_TRUE(p);
  const Ewff e = p.val;
  //const auto pp = Ewff::Create({{x1,n0}, {x2,n0}, {x3,n0}, {x4,n0}, {x5,n0}, {x6,n0}}, {});
  const auto pp = Ewff::Create({{x1,n0}, {x2,n0}, {x5,n0}, {x6,n0}}, {});
  EXPECT_TRUE(pp);
  const Ewff ee = pp.val;

  {
    const std::list<Assignment> models = e.Models(hplus);
    std::set<Assignment> models_set;
    std::copy(models.begin(), models.end(), std::inserter(models_set, models_set.end()));
    EXPECT_TRUE(!models_set.empty());

    // test completeness (and correctness) of Models()
    const std::list<Assignment> all_assignments = ee.Models(hplus);
    EXPECT_TRUE(all_assignments.size() > 0);

    for (const Assignment& theta : all_assignments) {
      EXPECT_EQ(models_set.count(theta) > 0, e.SatisfiedBy(theta));
    }

    for (const Assignment& theta : all_assignments) {
      EXPECT_TRUE(!e.Ground(theta) || e.SatisfiedBy(theta));
    }
  }
}

TEST(ewff_test, conj_normalization) {
  const Maybe<Ewff> e1 = Ewff::Create({}, {{x1,x4}, {x5,x2}, {x4,x2}});
  EXPECT_TRUE(e1);
  const Maybe<Ewff> e2 = Ewff::Create({}, {{x1,x4}, {x5,x2}, {x4,x2}, {x1,x1}, {x4,x2}, {x6,x6}});
  EXPECT_FALSE(e2);
  const Maybe<Ewff> e3 = Ewff::Create({}, {{x4,x2}, {x4,x2}, {x1,x4}, {x5,x2}});
  EXPECT_TRUE(e3);
  const Maybe<Ewff> e4 = Ewff::Create({}, {{x1,x4}, {x2,x4}, {x2,x5}});
  EXPECT_TRUE(e4);
  EXPECT_TRUE(e1.val == e3.val);
  EXPECT_TRUE(e3.val == e4.val);
  EXPECT_TRUE(e4.val == e1.val);
}

