// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./ewff.h>

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

TEST(ewff_test, conj) {
  const auto p = Ewff::Conj::Create({{x1,n1}, {x2,n2}},
                                    {{x3,x4}, {x4,x3}},
                                    {{x1,n2}, {x1,n3}, {x5,n6}},
                                    {{x1,x2}, {x2,x1}, {x5,x6}});
  EXPECT_TRUE(p.first);
  const Ewff::Conj c = p.second;

  {
    Assignment theta{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta);
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(c.CheckModel(theta));
  }
  {
    std::list<Assignment> models;
    c.Models(hplus, &models);
    EXPECT_TRUE(models.size() > 0);
    for (const Assignment& theta : models) {
      c.CheckModel(theta);
    }
  }
  {
    Assignment theta1{{x1,n1}, {x2,n2}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta1);
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(p.second.CheckModel(theta2));
  }
  {
    Assignment theta1{{x3,n3}, {x5,n5}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta1);
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(p.second.CheckModel(theta2));
  }
  {
    Assignment theta1{{x3,n3}, {x5,n6}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta1);
    EXPECT_FALSE(p.first);
  }
  {
    Assignment theta1{{x3,n3}, {x6,n5}, {x5, n6}};
    Assignment theta2{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta1);
    EXPECT_FALSE(p.first);
  }

  const auto pp = Ewff::Conj::Create({}, {}, {{x1,n0}, {x2,n0}, {x3,n0}, {x4,n0}, {x5,n0}, {x6,n0}}, {});
  EXPECT_TRUE(pp.first);
  const Ewff::Conj cc = pp.second;

  {
    std::list<Assignment> models;
    c.Models(hplus, &models);
    std::set<Assignment> models_set;
    std::copy(models.begin(), models.end(), std::inserter(models_set, models_set.end()));
    EXPECT_TRUE(!models_set.empty());

    // test completeness (and correctness) of Models()
    std::list<Assignment> all_assignments;
    cc.Models(hplus, &all_assignments);
    EXPECT_TRUE(all_assignments.size() > 0);

    for (const Assignment& theta : all_assignments) {
      EXPECT_EQ(models_set.count(theta) > 0, c.CheckModel(theta));
    }

    for (const Assignment& theta : all_assignments) {
      EXPECT_TRUE(!c.Ground(theta).first || c.CheckModel(theta));
    }
  }
}

TEST(ewff_test, ewff) {
  const auto p1 = Ewff::Conj::Create({{x2,n2}},
                {{x3,x4}},
                {{x1,n2}, {x1,n3}, {x5,n6}},
                {{x1,x2}, {x2,x1}, {x5,x6}});
  const auto p2 = Ewff::Conj::Create({{x1,n1}},
                {{x3,x4}},
                {{x1,n2}, {x1,n3}},
                {{x1,x2}});
  EXPECT_TRUE(p1.first);
  EXPECT_TRUE(p2.first);
  const Ewff::Conj c1 = p1.second;
  const Ewff::Conj c2 = p2.second;
  Ewff e({c1, c2});

  {
    std::list<Assignment> models1;
    c1.Models(hplus, &models1);
    std::set<Assignment> models_set1;
    std::copy(models1.begin(), models1.end(), std::inserter(models_set1, models_set1.end()));
    EXPECT_TRUE(!models_set1.empty());

    std::list<Assignment> models2;
    c2.Models(hplus, &models2);
    std::set<Assignment> models_set2;
    std::copy(models2.begin(), models2.end(), std::inserter(models_set2, models_set2.end()));
    EXPECT_TRUE(!models_set2.empty());

    // test completeness (and correctness) of Models()
    std::list<Assignment> all_assignments;
    c1.Models(hplus, &all_assignments);
    EXPECT_TRUE(all_assignments.size() > 0);

    for (const Assignment& theta : all_assignments) {
      EXPECT_EQ(models_set1.count(theta) + models_set2.count(theta) > 0, c1.CheckModel(theta) || c2.CheckModel(theta));
      EXPECT_EQ(models_set1.count(theta) + models_set2.count(theta) > 0, e.CheckModel(theta));
    }
  }
}

