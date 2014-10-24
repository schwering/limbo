// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./ewff.h>

static StdName n0 = Term::CreateStdName(0, 1);
static StdName n1 = Term::CreateStdName(1, 1);
static StdName n2 = Term::CreateStdName(2, 1);
static StdName n3 = Term::CreateStdName(3, 1);
static StdName n4 = Term::CreateStdName(4, 1);
static StdName n5 = Term::CreateStdName(5, 1);
static StdName n6 = Term::CreateStdName(6, 1);
static Variable x0 = Term::CreateVariable(1);
static Variable x1 = Term::CreateVariable(1);
static Variable x2 = Term::CreateVariable(1);
static Variable x3 = Term::CreateVariable(1);
static Variable x4 = Term::CreateVariable(1);
static Variable x5 = Term::CreateVariable(1);
static Variable x6 = Term::CreateVariable(1);

TEST(ewff_test, conj) {
  Ewff::Conj c({{x1,n1}, {x2,n2}},
               {{x3,x4}, {x4,x3}},
               {{x1,n2}, {x1,n3}, {x5,n6}},
               {{x1,x2}, {x2,x1}, {x5,x6}});

  {
    Assignment theta{{x1,n1}, {x2,n2}, {x3,n3}, {x4,n3}, {x5,n5}, {x6,n6}};
    std::pair<bool, Ewff::Conj> p = c.Ground(theta);
    EXPECT_TRUE(p.first);
    EXPECT_TRUE(c.CheckModel(theta));
  }
  {
    std::list<Assignment> models;
    c.FindModels({n0,n1,n2,n3,n4,n5,n6}, &models);
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

  Ewff::Conj cc({}, {}, {{x1,n0}, {x2,n0}, {x3,n0}, {x4,n0}, {x5,n0}, {x6,n0}}, {});

  {
    std::list<Assignment> models;
    c.FindModels({n1,n2,n3,n4,n5,n6}, &models);
    std::set<Assignment> models_set;
    std::copy(models.begin(), models.end(), std::inserter(models_set, models_set.end()));
    EXPECT_TRUE(!models_set.empty());

    // test completeness (and correctness) of FindModels()
    std::list<Assignment> all_assignments;
    cc.FindModels({n1,n2,n3,n4,n5,n6}, &all_assignments);
    EXPECT_TRUE(all_assignments.size() > 0);

    for (const Assignment& theta : all_assignments) {
      EXPECT_EQ(models_set.count(theta) > 0, c.CheckModel(theta));
    }
  }
}

TEST(ewff_test, ewff) {
  Ewff::Conj c1({{x2,n2}},
                {{x3,x4}},
                {{x1,n2}, {x1,n3}, {x5,n6}},
                {{x1,x2}, {x2,x1}, {x5,x6}});
  Ewff::Conj c2({{x1,n1}},
                {{x3,x4}},
                {{x1,n2}, {x1,n3}},
                {{x1,x2}});
  Ewff e({c1, c2});

  {
    std::list<Assignment> models1;
    c1.FindModels({n1,n2,n3,n4,n5,n6}, &models1);
    std::set<Assignment> models_set1;
    std::copy(models1.begin(), models1.end(), std::inserter(models_set1, models_set1.end()));
    EXPECT_TRUE(!models_set1.empty());

    std::list<Assignment> models2;
    c2.FindModels({n1,n2,n3,n4,n5,n6}, &models2);
    std::set<Assignment> models_set2;
    std::copy(models2.begin(), models2.end(), std::inserter(models_set2, models_set2.end()));
    EXPECT_TRUE(!models_set2.empty());

    // test completeness (and correctness) of FindModels()
    std::list<Assignment> all_assignments;
    c1.FindModels({n1,n2,n3,n4,n5,n6}, &all_assignments);
    EXPECT_TRUE(all_assignments.size() > 0);

    for (const Assignment& theta : all_assignments) {
      EXPECT_EQ(models_set1.count(theta) + models_set2.count(theta) > 0, c1.CheckModel(theta) || c2.CheckModel(theta));
      EXPECT_EQ(models_set1.count(theta) + models_set2.count(theta) > 0, e.CheckModel(theta));
    }
  }
}

