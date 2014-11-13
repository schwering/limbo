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
  EXPECT_TRUE(Formula::Neg(close->Copy())->EntailedBy(&bat.tf(), &s, 0));

  s.AddSensingResult({}, bat.forward, true);

  // Property 2
  EXPECT_FALSE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&bat.tf(), &s, 0));

  // Property 3
  EXPECT_TRUE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&bat.tf(), &s, 1));

  s.AddSensingResult({bat.forward}, bat.sonar, true);

  // Property 4
  EXPECT_TRUE(Formula::Act({bat.forward, bat.sonar}, close->Copy())->EntailedBy(&bat.tf(), &s, 1));
}

TEST(formula, gl_regression) {
  Kr2014 bat;
  auto& s = bat.setup();
  auto close = Formula::Or(Formula::Lit(Literal({}, true, bat.d0, {})),
                           Formula::Lit(Literal({}, true, bat.d1, {})));
  auto maybe_close = Formula::Or(Formula::Lit(Literal({}, true, bat.d1, {})),
                                 Formula::Lit(Literal({}, true, bat.d2, {})));

  // Property 1
  Maybe<Formula::Ptr> reg1 = Formula::Neg(close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1);
  EXPECT_TRUE(reg1.val->EntailedBy(&bat.tf(), &s, 0));

  s.AddSensingResult({}, bat.forward, true);

  // Property 2
  Maybe<Formula::Ptr> reg2 = Formula::Act(bat.forward, maybe_close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg2);
  //EXPECT_FALSE(reg2.val->EntailedBy(&bat.tf(), &s, 0)); // here regression differs from ESL
  EXPECT_TRUE(reg2.val->EntailedBy(&bat.tf(), &s, 0));

  // Property 3
  Maybe<Formula::Ptr> reg3 = Formula::Act(bat.forward, maybe_close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3);
  EXPECT_TRUE(reg3.val->EntailedBy(&bat.tf(), &s, 1));

  s.AddSensingResult({bat.forward}, bat.sonar, true);

  // Property 4
  Maybe<Formula::Ptr> reg4 = Formula::Act({bat.forward, bat.sonar}, close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg4);
  EXPECT_TRUE(reg4.val->EntailedBy(&bat.tf(), &s, 1));
}

TEST(formula, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  EXPECT_TRUE(Formula::Lit(Literal({}, false, bat.L1, {}))->EntailedBy(&bat.tf(), &s, 2));

  // Property 2
  s.AddSensingResult({}, bat.SL, true);
  EXPECT_TRUE(Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 3
  s.AddSensingResult({bat.SL}, bat.SR1, false);
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 5
  EXPECT_FALSE(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&bat.tf(), &s, 2));
  EXPECT_FALSE(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {})))->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  s.AddSensingResult({bat.SL,bat.SR1,bat.LV}, bat.SL, true);
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&bat.tf(), &s, 2));
}

TEST(formula, morri_regression) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  Maybe<Formula::Ptr> reg1 = Formula::Lit(Literal({}, false, bat.L1, {}))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1);
  EXPECT_TRUE(reg1.val->EntailedBy(&bat.tf(), &s, 2));

  // Property 2
  s.AddSensingResult({}, bat.SL, true);
  Maybe<Formula::Ptr> reg2 = Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                               Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg2);
  EXPECT_TRUE(reg2.val->EntailedBy(&bat.tf(), &s, 2));

  // Property 3
  s.AddSensingResult({bat.SL}, bat.SR1, false);
  Maybe<Formula::Ptr> reg3 = Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3);
  EXPECT_TRUE(reg3.val->EntailedBy(&bat.tf(), &s, 2));

  // Property 5
  Maybe<Formula::Ptr> reg4 = Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg4);
  EXPECT_FALSE(reg4.val->EntailedBy(&bat.tf(), &s, 2));
  Maybe<Formula::Ptr> reg5 = Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg5);
  EXPECT_FALSE(reg5.val->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  Maybe<Formula::Ptr> reg6 = Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg6);
  EXPECT_TRUE(reg6.val->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  s.AddSensingResult({bat.SL,bat.SR1,bat.LV}, bat.SL, true);
  Maybe<Formula::Ptr> reg7 = Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg7);
  EXPECT_TRUE(reg7.val->EntailedBy(&bat.tf(), &s, 2));
}

TEST(formula, fol_incompleteness_positive1) {
  // The tautology (A x . E y . ~P(x) v P(y)) is provable in our variant of ESL.
  // Is it provable in the paper? Don't remember, check. TODO(chs)
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q = Formula::Forall(x, Formula::Exists(y,
              Formula::Or(Formula::Lit(Literal({}, true, 0, {x})),
                          Formula::Lit(Literal({}, false, 0, {y})))));
  esbl::Setup s;
  for (Setup::split_level k = 1; k < 2; ++k) {
    EXPECT_EQ(q->EntailedBy(&tf, &s, k), k > 0);
  }
}

TEST(formula, fol_incompleteness_positive2) {
  // The tautology (A x . P(x)) v (E y . ~P(y)) is provable in our variant of
  // ESL, because the formula is implicitly brought to prenex form, starting
  // with the quantifiers from left to right, and hence it is equivalent to the
  // formula from test fol_incompleteness_positive1.
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q1 = Formula::Forall(x, Formula::Lit(Literal({}, true, 0, {x})));
  auto q2 = Formula::Exists(y, Formula::Neg(Formula::Lit(Literal({}, true, 0, {y}))));
  auto q = Formula::Or(std::move(q1), std::move(q2));
  esbl::Setup s;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_EQ(q->EntailedBy(&tf, &s, k), k > 0);
  }
}

TEST(formula, fol_incompleteness_negative1) {
  // The tautology (E x . A y . ~P(x) v P(y)) is not provable in our variant of
  // ESL (and neither it is in the paper version).
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q = Formula::Exists(x, Formula::Forall(y,
              Formula::Or(Formula::Lit(Literal({}, true, 0, {x})),
                          Formula::Lit(Literal({}, false, 0, {y})))));
  esbl::Setup s;
  for (Setup::split_level k = 1; k < 2; ++k) {
    EXPECT_FALSE(q->EntailedBy(&tf, &s, k));
  }
}

TEST(formula, fol_incompleteness_negative2) {
  // The tautology (E y . ~P(y)) v (A x . P(x)) is not provable in our variant
  // of ESL, because the formula is implicitly brought to prenex form, starting
  // with the quantifiers from left to right, and hence it is equivalent to the
  // formula from test fol_incompleteness_negative1.
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q1 = Formula::Forall(x, Formula::Lit(Literal({}, true, 0, {x})));
  auto q2 = Formula::Exists(y, Formula::Neg(Formula::Lit(Literal({}, true, 0, {y}))));
  auto q = Formula::Or(std::move(q2), std::move(q1));
  esbl::Setup s;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_FALSE(q->EntailedBy(&tf, &s, k));
  }
}

TEST(formula, fol_incompleteness_reverse) {
  // The sentence (A x . ~P(x)) v (A x . P(x)) is not a tautology and hence
  // should come out false.
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q1 = Formula::Forall(x, Formula::Lit(Literal({}, true, 0, {x})));
  auto q2 = Formula::Forall(y, Formula::Neg(Formula::Lit(Literal({}, true, 0, {y}))));
  auto q = Formula::Or(std::move(q1), std::move(q2));
  esbl::Setup s;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_FALSE(q->EntailedBy(&tf, &s, k));
  }
}

TEST(formula, fol_setup_universal) {
  // The setup { P(x) } should entail (A x . P(x)).
  esbl::Setup s;
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  s.AddClause(Clause(Ewff::TRUE, SimpleClause({Literal({}, true, 0, {x})})));
  auto q = Formula::Forall(y, Formula::Lit(Literal({}, true, 0, {x})));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(q->EntailedBy(&tf, &s, k));
  }
}

