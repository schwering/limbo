// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./ecai2014.h>
#include <./kr2014.h>
#include <./formula.h>

using namespace esbl;
using namespace bats;

TEST(formula, gl) {
  Kr2014 bat;
  auto& s = bat.setup();
  auto close = Formula::Or(Formula::Lit(Literal({}, true, bat.d0, {})),
                           Formula::Lit(Literal({}, true, bat.d1, {})));
  auto maybe_close = Formula::Or(Formula::Lit(Literal({}, true, bat.d1, {})),
                                 Formula::Lit(Literal({}, true, bat.d2, {})));

  // Property 1
  EXPECT_TRUE(Formula::Neg(close->Copy())->EntailedBy(&s, 0));

  s.AddSensingResult({}, bat.forward, true);

  // Property 2
  EXPECT_FALSE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&s, 0));

  // Property 3
  EXPECT_TRUE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&s, 1));

  s.AddSensingResult({bat.forward}, bat.sonar, true);

  // Property 4
  EXPECT_TRUE(Formula::Act({bat.forward, bat.sonar}, close->Copy())->EntailedBy(&s, 1));
}

TEST(formula, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  EXPECT_TRUE(Formula::Lit(Literal({}, false, bat.L1, {}))->EntailedBy(&s, 2));

  // Property 2
  s.AddSensingResult({}, bat.SL, true);
  EXPECT_TRUE(Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&s, 2));

  // Property 3
  s.AddSensingResult({bat.SL}, bat.SR1, false);
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&s, 2));

  // Property 5
  EXPECT_FALSE(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&s, 2));
  EXPECT_FALSE(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))->EntailedBy(&s, 2));

  // Property 6
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {})))->EntailedBy(&s, 2));

  // Property 6
  s.AddSensingResult({bat.SL,bat.SR1,bat.LV}, bat.SL, true);
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&s, 2));
}

TEST(formula, fol_incompleteness) {
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q1 = Formula::Forall(x, Formula::Lit(Literal({}, true, 0, {x})));
  auto q2 = Formula::Exists(y, Formula::Neg(Formula::Lit(Literal({}, true, 0, {y}))));
  auto q = Formula::Or(std::move(q1), std::move(q2));
  esbl::Setup s;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_FALSE(q->EntailedBy(&s, k));
  }
}

