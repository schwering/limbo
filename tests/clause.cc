// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <limbo/clause.h>

namespace limbo {

TEST(ClauseTest, Normalize) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);

  {
    Lit as[] = {Lit::Eq(f, n), Lit::Eq(f, n), Lit::Neq(f, m), Lit::Neq(g, n), Lit::Neq(g, n), Lit::Eq(g, m), Lit::Eq(g, m)};
    const size_t len = Clause::Normalize(sizeof(as) / sizeof(as[0]), as, Clause::InvalidityPromise{false});
    EXPECT_EQ(len, 2);
    EXPECT_EQ(as[0], Lit::Neq(f, m));
    EXPECT_EQ(as[1], Lit::Neq(g, n));
  }

  {
    Lit as[] = {Lit::Eq(f, n), Lit::Eq(f, n), Lit::Neq(f, m), Lit::Neq(g, n), Lit::Neq(g, n), Lit::Eq(g, m), Lit::Eq(g, m), Lit::Neq(g, m)};
    const size_t len = Clause::Normalize(sizeof(as) / sizeof(as[0]), as, Clause::InvalidityPromise{true});
    EXPECT_EQ(len, 3);
    EXPECT_EQ(as[0], Lit::Neq(f, m));
    EXPECT_EQ(as[1], Lit::Neq(g, n));
    EXPECT_EQ(as[2], Lit::Neq(g, m));
  }

  {
    Lit as[] = {Lit::Eq(f, n), Lit::Eq(f, n), Lit::Neq(f, m), Lit::Neq(g, n), Lit::Neq(g, n), Lit::Eq(g, m), Lit::Eq(g, m), Lit::Neq(g, m)};
    const size_t len = Clause::Normalize(sizeof(as) / sizeof(as[0]), as, Clause::InvalidityPromise{false});
    EXPECT_EQ(len, -1);
  }
}

TEST(ClauseTest, ClauseFactory) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);
  Clause::Factory factory;

  {
    Clause::Factory::CRef cr = factory.New({Lit::Eq(f, n), Lit::Eq(f, n), Lit::Neq(f, m), Lit::Neq(g, n), Lit::Neq(g, n), Lit::Eq(g, m), Lit::Eq(g, m)});
    EXPECT_NE(cr, Clause::Factory::CRef::kNull);
    EXPECT_NE(cr, Clause::Factory::CRef::kDomain);
    Clause& c = factory[cr];
    EXPECT_FALSE(c.valid());
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c[0], Lit::Neq(f, m));
    EXPECT_EQ(c[1], Lit::Neq(g, n));
    EXPECT_THAT(c, ::testing::ElementsAre(Lit::Neq(f, m), Lit::Neq(g, n)));
    EXPECT_FALSE(c.unit());
    EXPECT_FALSE(c.learnt());

    Clause::Factory::CRef cr2 = factory.New({Lit::Neq(f, m), Lit::Neq(g, n)});
    EXPECT_NE(cr, cr2);
    const Clause& d = factory[cr2];
    EXPECT_EQ(c, d);
    EXPECT_FALSE(d.valid());
    EXPECT_FALSE(d.unit());
    EXPECT_FALSE(d.learnt());
    EXPECT_TRUE(c.Subsumes(d));
    EXPECT_TRUE(d.Subsumes(c));

    Clause::Factory::CRef cr3 = factory.New({Lit::Eq(f, n)});
    const Clause& u = factory[cr3];
    EXPECT_NE(c, u);
    EXPECT_TRUE(u.unit());
    EXPECT_EQ(u[0], Lit::Eq(f, n));
    EXPECT_THAT(u, ::testing::ElementsAre(Lit::Eq(f, n)));
    EXPECT_FALSE(u.valid());
    EXPECT_FALSE(c.Subsumes(u));
    EXPECT_TRUE(u.Subsumes(c));

    const int removed = c.RemoveIf([g](const Lit a) { return a.fun() == g; });
    EXPECT_EQ(removed, 1);
    EXPECT_NE(c, d);
    EXPECT_FALSE(c.valid());
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c[0], Lit::Neq(f, m));
    EXPECT_THAT(c, ::testing::ElementsAre(Lit::Neq(f, m)));
    EXPECT_TRUE(c.unit());
    EXPECT_FALSE(c.learnt());
  }

  {
    Clause::Factory::CRef cr = factory.New({Lit::Eq(f, n), Lit::Eq(f, n), Lit::Neq(f, m), Lit::Neq(g, n), Lit::Neq(g, n), Lit::Eq(g, m), Lit::Eq(g, m), Lit::Neq(g, m)});
    EXPECT_NE(cr, Clause::Factory::CRef::kNull);
    EXPECT_NE(cr, Clause::Factory::CRef::kDomain);
    const Clause& c = factory[cr];
    EXPECT_TRUE(c.valid());
    EXPECT_EQ(c.size(), 1);
    EXPECT_TRUE(c.unit());
    EXPECT_FALSE(c.learnt());
    EXPECT_TRUE(c[0].null());
  }
}

}  // namespace limbo
 
