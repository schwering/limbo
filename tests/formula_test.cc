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
  EXPECT_TRUE(Formula::Know(0, Formula::Neg(close->Copy()))->Eval(&s));

  s.AddClause(Clause(Ewff::TRUE, {SfLiteral({}, bat.forward, true)}));

  // Property 2
  EXPECT_FALSE(Formula::Know(0, Formula::Act(bat.forward, maybe_close->Copy()))->Eval(&s));

  // Property 3
  EXPECT_TRUE(Formula::Know(1, Formula::Act(bat.forward, maybe_close->Copy()))->Eval(&s));

  s.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.forward}, bat.sonar, true)}));

  // Property 4
  EXPECT_TRUE(Formula::Know(1, Formula::Act({bat.forward, bat.sonar}, close->Copy()))->Eval(&s));
}

TEST(formula, gl_regression) {
  Kr2014 bat;
  auto& s = bat.setup();
  auto close = Formula::Or(Formula::Lit(Literal({}, true, bat.d0, {})),
                           Formula::Lit(Literal({}, true, bat.d1, {})));
  auto maybe_close = Formula::Or(Formula::Lit(Literal({}, true, bat.d1, {})),
                                 Formula::Lit(Literal({}, true, bat.d2, {})));

  // Property 1
  Formula::Ptr reg1 = Formula::Know(0, Formula::Neg(close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1->Eval(&s));

  Formula::Lit(SfLiteral({}, bat.forward, true))->Regress(&bat.tf(), bat)->AddToSetup(&s);

  // Property 2
  Formula::Ptr reg2 = Formula::Act(bat.forward, Formula::Know(0, maybe_close->Copy()))->Regress(&bat.tf(), bat);
  //EXPECT_FALSE(reg2->Eval(&s)); // here regression differs from ESL
  EXPECT_TRUE(reg2->Eval(&s));

  // Property 3
  Formula::Ptr reg3 = Formula::Act(bat.forward, Formula::Know(1, maybe_close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3->Eval(&s));

  Formula::Lit(SfLiteral({bat.forward}, bat.sonar, true))->Regress(&bat.tf(), bat)->AddToSetup(&s);

  // Property 4
  Formula::Ptr reg4 = Formula::Act({bat.forward, bat.sonar}, Formula::Know(1, close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg4->Eval(&s));
}

TEST(formula, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  Formula::Ptr q1 = Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {})));
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  std::cout << __FILE__ << ":" << *q1 << std::endl;
  EXPECT_TRUE(q1->Eval(&s));
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;

  // Property 2
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  s.AddClause(Clause(Ewff::TRUE, {SfLiteral({}, bat.SL, true)}));
  EXPECT_TRUE(Formula::Act(bat.SL, Formula::Know(2, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                                 Formula::Lit(Literal({}, true, bat.R1, {})))))->Eval(&s));

  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  // Property 3
  s.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL}, bat.SR1, false)}));
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1}, Formula::Know(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {})))))->Eval(&s));

  // Property 5
  EXPECT_FALSE(Formula::Act({bat.SL, bat.SR1}, Formula::Know(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Eval(&s));
  EXPECT_FALSE(Formula::Act({bat.SL, bat.SR1}, Formula::Know(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.L1, {})))))->Eval(&s));
  EXPECT_TRUE(Formula::And(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Know(2, Formula::Lit(Literal({}, true, bat.L1, {}))))),
                           Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Know(2, Formula::Lit(Literal({}, false, bat.L1, {}))))))->Eval(&s));

  // Property 6
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Know(2, Formula::Lit(Literal({}, true, bat.R1, {}))))->Eval(&s));

  // Property 6
  s.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL,bat.SR1,bat.LV}, bat.SL, true)}));
  EXPECT_TRUE(Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Know(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Eval(&s));
}

TEST(formula, morri_regression) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);
  auto& s = bat.setups();

  // Property 1
  Formula::Ptr reg1 = Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg1->Eval(&s));

  // Property 2
  Formula::Lit(SfLiteral({}, bat.SL, true))->Regress(&bat.tf(), bat)->AddToSetups(&s);
  Formula::Ptr reg2 = Formula::Act(bat.SL, Formula::Believe(2, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                                            Formula::Lit(Literal({}, true, bat.R1, {})))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg2->Eval(&s));

  // Property 3
  Formula::Lit(SfLiteral({bat.SL}, bat.SR1, false))->Regress(&bat.tf(), bat)->AddToSetups(&s);
  Formula::Ptr reg3 = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {})))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg3->Eval(&s));

  // Property 5
  Formula::Ptr reg5a = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(reg5a->Eval(&s));
  Formula::Ptr reg5b = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.L1, {})))))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(reg5b->Eval(&s));
  Formula::Ptr reg5 = Formula::And(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))),
                                   Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {}))))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg5->Eval(&s));

  // Property 6
  Formula::Ptr reg6 = Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg6->Eval(&s));

  // Property 6
  Formula::Lit(SfLiteral({bat.SL,bat.SR1,bat.LV}, bat.SL, true))->Regress(&bat.tf(), bat)->AddToSetups(&s);
  Formula::Ptr reg7 = Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(reg7->Eval(&s));
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
    EXPECT_EQ(Formula::Know(k, q->Copy())->Eval(&s), k > 0);
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
    //EXPECT_EQ(Formula::Know(k, q->Copy())->Eval(&s), k > 0);
    // It holds even for k = 0 because our CNF we drop tautologous clauses from
    // the CNF.
    EXPECT_TRUE(Formula::Know(k, q->Copy())->Eval(&s));
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
    EXPECT_FALSE(Formula::Know(k, q->Copy())->Eval(&s));
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
    EXPECT_FALSE(Formula::Know(k, q->Copy())->Eval(&s));
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
    EXPECT_FALSE(Formula::Know(k, q->Copy())->Eval(&s));
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
    EXPECT_TRUE(Formula::Know(k, q->Copy())->Eval(&s));
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
    EXPECT_TRUE(Formula::Know(k, phi->Copy())->Eval(&s));
  }
}

