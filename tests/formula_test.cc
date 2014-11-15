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

  s.AddClause(Clause(Ewff::TRUE, {Literal({}, true, Atom::SF, {bat.forward})}));

  // Property 2
  EXPECT_FALSE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&bat.tf(), &s, 0));

  // Property 3
  EXPECT_TRUE(Formula::Act(bat.forward, maybe_close->Copy())->EntailedBy(&bat.tf(), &s, 1));

  s.AddClause(Clause(Ewff::TRUE, {Literal({bat.forward}, true, Atom::SF, {bat.sonar})}));

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
  Formula::Ptr reg1 = Formula::Neg(close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1->EntailedBy(&bat.tf(), &s, 0));

  Formula::Lit(Literal({}, true, Atom::SF, {bat.forward}))->Regress(&bat.tf(), bat)->AddToSetup(&bat.tf(), &s);

  // Property 2
  Formula::Ptr reg2 = Formula::Act(bat.forward, maybe_close->Copy())->Regress(&bat.tf(), bat);
  //EXPECT_FALSE(reg2->EntailedBy(&bat.tf(), &s, 0)); // here regression differs from ESL
  EXPECT_TRUE(reg2->EntailedBy(&bat.tf(), &s, 0));

  // Property 3
  Formula::Ptr reg3 = Formula::Act(bat.forward, maybe_close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3->EntailedBy(&bat.tf(), &s, 1));

  Formula::Lit(Literal({bat.forward}, true, Atom::SF, {bat.sonar}))->Regress(&bat.tf(), bat)->AddToSetup(&bat.tf(), &s);

  // Property 4
  Formula::Ptr reg4 = Formula::Act({bat.forward, bat.sonar}, close->Copy())->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg4->EntailedBy(&bat.tf(), &s, 1));
}

TEST(formula, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  EXPECT_TRUE(Formula::Lit(Literal({}, false, bat.L1, {}))->EntailedBy(&bat.tf(), &s, 2));

  // Property 2
  s.AddClause(Clause(Ewff::TRUE, {Literal({}, true, Atom::SF, {bat.SL})}));
  EXPECT_TRUE(Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 3
  s.AddClause(Clause(Ewff::TRUE, {Literal({bat.SL}, false, Atom::SF, {bat.SR1})}));
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 5
  EXPECT_FALSE(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&bat.tf(), &s, 2));
  EXPECT_FALSE(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {})))->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  s.AddClause(Clause(Ewff::TRUE, {Literal({bat.SL,bat.SR1,bat.LV}, true, Atom::SF, {bat.SL})}));
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {})))->EntailedBy(&bat.tf(), &s, 2));
}

TEST(formula, morri_regression) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  Formula::Ptr reg1 = Formula::Lit(Literal({}, false, bat.L1, {}))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1->EntailedBy(&bat.tf(), &s, 2));

  // Property 2
  Formula::Lit(Literal({}, true, Atom::SF, {bat.SL}))->Regress(&bat.tf(), bat)->AddToSetups(&bat.tf(), &s);
  Formula::Ptr reg2 = Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                               Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg2->EntailedBy(&bat.tf(), &s, 2));

  // Property 3
  Formula::Lit(Literal({bat.SL}, false, Atom::SF, {bat.SR1}))->Regress(&bat.tf(), bat)->AddToSetups(&bat.tf(), &s);
  Formula::Ptr reg3 = Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3->EntailedBy(&bat.tf(), &s, 2));

  // Property 5
  Formula::Ptr reg4 = Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(reg4->EntailedBy(&bat.tf(), &s, 2));
  Formula::Ptr reg5 = Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(reg5->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  Formula::Ptr reg6 = Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg6->EntailedBy(&bat.tf(), &s, 2));

  // Property 6
  Formula::Lit(Literal({bat.SL,bat.SR1,bat.LV}, true, Atom::SF, {bat.SL}))->Regress(&bat.tf(), bat)->AddToSetups(&bat.tf(), &s);
  Formula::Ptr reg7 = Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg7->EntailedBy(&bat.tf(), &s, 2));
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
    //EXPECT_EQ(q->EntailedBy(&tf, &s, k), k > 0);
    // It holds even for k = 0 because our CNF we drop tautologous clauses from
    // the CNF.
    EXPECT_TRUE(q->EntailedBy(&tf, &s, k));
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
  // The setup { P(x) } should entail (A y . P(y)).
  esbl::Setup s;
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  s.AddClause(Clause(Ewff::TRUE, SimpleClause({Literal({}, true, 0, {x})})));
  auto q = Formula::Forall(y, Formula::Lit(Literal({}, true, 0, {y})));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(q->EntailedBy(&tf, &s, k));
  }
}

TEST(formula, query_resolution) {
  // The query (p v q) ^ (~p v q) is subsumed by setup {q} for split k > 0.
  // And since we minimize the CNF, we obtain the query {q} and thus the query
  // should hold for k = 0 as well.
  esbl::Setup s;
  Term::Factory tf;
  const Literal p = Literal({}, true, 0, {});
  const Literal q = Literal({}, true, 1, {});
  s.AddClause(Clause(Ewff::TRUE, SimpleClause({q})));
  auto phi = Formula::And(Formula::Or(Formula::Lit(q), Formula::Lit(p)),
                          Formula::Or(Formula::Lit(q), Formula::Lit(p.Flip())));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(phi->EntailedBy(&tf, &s, k));
  }
}

