// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./clause.h>
#include <./setups.h>

using namespace lela;

#if 0
using namespace bats;

TEST(setup, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto s1 = bat.setups();
  EXPECT_EQ(s1.setups().size(), 3);

  // Property 1
  EXPECT_TRUE(s1.Entails(SimpleClause({Literal({}, false, bat.L1, {})}), k));

  // Property 2
  auto s2 = s1;
  s2.AddClause(Clause(Ewff::TRUE, {SfLiteral({}, bat.SL, true)}));
  EXPECT_TRUE(s2.Entails(SimpleClause({Literal({bat.SL}, true, bat.R1, {})}), k));
  EXPECT_TRUE(s2.Entails(SimpleClause({Literal({bat.SL}, true, bat.L1, {})}), k) &&
              s2.Entails(SimpleClause({Literal({bat.SL}, true, bat.R1, {})}), k));
  EXPECT_FALSE(s1.Entails(SimpleClause({Literal({bat.SL}, true, bat.L1, {})}), k) &&
               s1.Entails(SimpleClause({Literal({bat.SL}, true, bat.R1, {})}), k));  // sensing is really required
  EXPECT_TRUE(s2.Entails(SimpleClause({Literal({}, true, bat.R1, {})}), k));  // the sensing action in the situation is redundant

  // Property 3
  auto s3 = s2;
  s3.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL}, bat.SR1, false)}));
  EXPECT_TRUE(s3.Entails(SimpleClause({Literal({bat.SL,bat.SR1}, false, bat.R1, {})}), k));
  EXPECT_FALSE(s2.Entails(SimpleClause({Literal({bat.SL,bat.SR1}, false, bat.R1, {})}), k));  // sensing is really required
  EXPECT_TRUE(s3.Entails(SimpleClause({Literal({}, false, bat.R1, {})}), k));  // the sensing actions in the situation are redundant

  // Property 5
  EXPECT_FALSE(s3.Entails(SimpleClause({Literal({bat.SL,bat.SR1}, true, bat.L1, {})}), k));
  EXPECT_FALSE(s3.Entails(SimpleClause({Literal({bat.SL,bat.SR1}, false, bat.L1, {})}), k));
  EXPECT_FALSE(s3.Entails(SimpleClause({Literal({}, true, bat.L1, {})}), k));  // the sensing actions in the situation are redundant

  // Property 6
  auto s4 = s3;
  s4.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL,bat.SR1}, bat.LV, true)}));
  EXPECT_TRUE(s4.Entails(SimpleClause({Literal({bat.SL,bat.SR1,bat.LV}, true, bat.R1, {})}), k));
  EXPECT_FALSE(s4.Entails(SimpleClause({Literal({bat.SL,bat.SR1}, true, bat.R1, {})}), k));  // LV had an effect

  // Property 7
  auto s5 = s4;
  s5.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL,bat.SR1,bat.LV}, bat.SL, true)}));
  EXPECT_TRUE(s5.Entails(SimpleClause({Literal({bat.SL,bat.SR1,bat.LV,bat.SL}, true, bat.L1, {})}), k));
  EXPECT_TRUE(s5.Entails(SimpleClause({Literal({bat.LV}, true, bat.L1, {})}), k));  // the sensing actions in the situation are redundant
}
#endif

TEST(setup, example_12) {
  constexpr Setups::split_level k = 1;
  Setups s;
  const Literal a(true, 1, {});
  const Literal b(true, 2, {});
  const Literal c(true, 3, {});
  s.AddBeliefConditional(Clause(Ewff::TRUE, {a.Negative()}), Clause(Ewff::TRUE, {b}), k);
  s.AddBeliefConditional(Clause(Ewff::TRUE, {c.Negative()}), Clause(Ewff::TRUE, {a}), k);
  s.AddBeliefConditional(Clause(Ewff::TRUE, {c.Negative()}), Clause(Ewff::TRUE, {b.Negative()}), k);
  EXPECT_EQ(s.setups().size(), 3);

  EXPECT_TRUE(s.setup(0).Entails({a.Negative(), b}, k));
  EXPECT_TRUE(s.setup(0).Entails({c.Negative()}, k));
  EXPECT_TRUE(s.setup(1).Entails({c.Negative(), a}, k));
  EXPECT_TRUE(s.setup(1).Entails({c.Negative(), b.Negative()}, k));
  EXPECT_FALSE(s.setup(1).Entails({a}, k));
  EXPECT_FALSE(s.setup(1).Entails({b.Negative()}, k));
  EXPECT_TRUE(s.setup(2).clauses().empty());
}

TEST(setup, test_inconsistency) {
  Setups s;
  const Literal a(true, 1, {});
  const Literal b(true, 2, {});
  s.AddBeliefConditional(Clause(Ewff::TRUE, {}), Clause(Ewff::TRUE, {a, b}), 0);
  s.AddBeliefConditional(Clause(Ewff::TRUE, {}), Clause(Ewff::TRUE, {a, b.Flip()}), 0);
  s.AddBeliefConditional(Clause(Ewff::TRUE, {}), Clause(Ewff::TRUE, {a.Flip(), b}), 0);
  s.AddBeliefConditional(Clause(Ewff::TRUE, {}), Clause(Ewff::TRUE, {a.Flip(), b.Flip()}), 0);
  EXPECT_EQ(s.setups().size(), 2);

  EXPECT_TRUE(s.setup(0).Entails({a, b}, 0));
  EXPECT_FALSE(s.setup(1).Entails({a, b}, 0));

  EXPECT_TRUE(s.Entails({a, b}, 0));
  EXPECT_FALSE(s.Entails({a, b}, 1));
  EXPECT_TRUE(s.Entails({a, b}, 0));
}

