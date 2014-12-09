// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./ecai2014.h>
#include <./kr2014.h>
#include <./testbat.h>
#include <./formula.h>

using namespace esbl;
using namespace bats;

TEST(formula, gl) {
  Kr2014 bat;
  auto close = Formula::Or(Formula::Lit(Literal({}, true, bat.d0, {})),
                           Formula::Lit(Literal({}, true, bat.d1, {})));
  auto maybe_close = Formula::Or(Formula::Lit(Literal({}, true, bat.d1, {})),
                                 Formula::Lit(Literal({}, true, bat.d2, {})));

  // Property 1
  EXPECT_TRUE(bat.Entails(Formula::Know(0, Formula::Neg(close->Copy()))));

  bat.AddClause(Clause(Ewff::TRUE, {SfLiteral({}, bat.forward, true)}));

  // Property 2
  EXPECT_FALSE(bat.Entails(Formula::Know(0, Formula::Act(bat.forward, maybe_close->Copy()))));

  // Property 3
  EXPECT_TRUE(bat.Entails(Formula::Know(1, Formula::Act(bat.forward, maybe_close->Copy()))));

  bat.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.forward}, bat.sonar, true)}));

  // Property 4
  EXPECT_TRUE(bat.Entails(Formula::Know(1, Formula::Act({bat.forward, bat.sonar}, close->Copy()))));
}

TEST(formula, gl_regression) {
  Kr2014 bat;
  auto close = Formula::Or(Formula::Lit(Literal({}, true, bat.d0, {})),
                           Formula::Lit(Literal({}, true, bat.d1, {})));
  auto maybe_close = Formula::Or(Formula::Lit(Literal({}, true, bat.d1, {})),
                                 Formula::Lit(Literal({}, true, bat.d2, {})));

  // Property 1
  Formula::Ptr reg1 = Formula::Know(0, Formula::Neg(close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg1));

  bat.Add(Formula::Lit(SfLiteral({}, bat.forward, true))->ObjRegress(&bat.tf(), bat));

  // Property 2
  Formula::Ptr reg2 = Formula::Act(bat.forward, Formula::Know(0, maybe_close->Copy()))->Regress(&bat.tf(), bat);
  //EXPECT_FALSE(reg2)); // here regression differs from ESL
  EXPECT_TRUE(bat.Entails(reg2));

  // Property 3
  Formula::Ptr reg3 = Formula::Act(bat.forward, Formula::Know(1, maybe_close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg3));

  bat.Add(Formula::Lit(SfLiteral({bat.forward}, bat.sonar, true))->ObjRegress(&bat.tf(), bat));

  // Property 4
  Formula::Ptr reg4 = Formula::Act({bat.forward, bat.sonar}, Formula::Know(1, close->Copy()))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg4));
}

TEST(formula, morri) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);

  // Property 1
  Formula::Ptr q1 = Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {})));
  EXPECT_TRUE(bat.Entails(q1));

  // Property 2
  bat.AddClause(Clause(Ewff::TRUE, {SfLiteral({}, bat.SL, true)}));
  EXPECT_TRUE(bat.Entails(Formula::Believe(2, Formula::Act(bat.SL, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                                    Formula::Lit(Literal({}, true, bat.R1, {})))))));

  // Property 3
  bat.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL}, bat.SR1, false)}));
  EXPECT_TRUE(bat.Entails(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {})))))));

  // Property 5
  EXPECT_FALSE(bat.Entails(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))));
  EXPECT_FALSE(bat.Entails(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1}, Formula::Neg(Formula::Lit(Literal({}, true, bat.L1, {})))))));
  EXPECT_TRUE(bat.Entails(Formula::And(Formula::Neg(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, true, bat.L1, {}))))),
                                       Formula::Neg(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1}, Formula::Lit(Literal({}, false, bat.L1, {}))))))));

  // Property 6
  EXPECT_TRUE(bat.Entails(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Lit(Literal({}, true, bat.R1, {}))))));

  // Property 6
  bat.AddClause(Clause(Ewff::TRUE, {SfLiteral({bat.SL,bat.SR1,bat.LV}, bat.SL, true)}));
  EXPECT_TRUE(bat.Entails(Formula::Believe(2, Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Lit(Literal({}, true, bat.L1, {}))))));
}

TEST(formula, morri_regression) {
  constexpr Setups::split_level k = 2;
  Ecai2014 bat(k);

  // Property 1
  Formula::Ptr reg1 = Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {})))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg1));

  // Property 2
  bat.Add(Formula::Lit(SfLiteral({}, bat.SL, true))->ObjRegress(&bat.tf(), bat));
  Formula::Ptr reg2 = Formula::Act(bat.SL, Formula::Believe(2, Formula::And(Formula::Lit(Literal({}, true, bat.L1, {})),
                                                                            Formula::Lit(Literal({}, true, bat.R1, {})))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg2));

  // Property 3
  bat.Add(Formula::Lit(SfLiteral({bat.SL}, bat.SR1, false))->ObjRegress(&bat.tf(), bat));
  Formula::Ptr reg3 = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.R1, {})))))->Regress(&bat.tf(), bat);
  std::cout << *reg3 << std::endl;
  EXPECT_TRUE(bat.Entails(reg3));

  // Property 5
  Formula::Ptr reg5a = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(bat.Entails(reg5a));
  Formula::Ptr reg5b = Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Neg(Formula::Lit(Literal({}, true, bat.L1, {})))))->Regress(&bat.tf(), bat);
  EXPECT_FALSE(bat.Entails(reg5b));
  Formula::Ptr reg5 = Formula::And(Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))),
                                   Formula::Neg(Formula::Act({bat.SL, bat.SR1}, Formula::Believe(2, Formula::Lit(Literal({}, false, bat.L1, {}))))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg5));

  // Property 6
  Formula::Ptr reg6 = Formula::Act({bat.SL, bat.SR1, bat.LV}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.R1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg6));

  // Property 6
  bat.Add(Formula::Lit(SfLiteral({bat.SL,bat.SR1,bat.LV}, bat.SL, true))->ObjRegress(&bat.tf(), bat));
  Formula::Ptr reg7 = Formula::Act({bat.SL, bat.SR1, bat.LV, bat.SL}, Formula::Believe(2, Formula::Lit(Literal({}, true, bat.L1, {}))))->Regress(&bat.tf(), bat);
  EXPECT_TRUE(bat.Entails(reg7));

  std::cout << queries << " Queries" << std::endl;
}

class EmptyBat : public BasicActionTheory {
 public:
  Maybe<Formula::ObjPtr> RegressOneStep(Term::Factory*,
                                        const Atom&) const override {
    return Nothing;
  }

  void GuaranteeConsistency(split_level k) override {
    s_.GuaranteeConsistency(k);
  }

  size_t n_levels() const override { return 1; }

  const StdName::SortedSet& names() const override {
    return ns_;
  }

  void AddClause(const Clause& c) override {
    s_.AddClause(c);
    ns_ = s_.hplus().WithoutPlaceholders();
  }

  bool InconsistentAt(belief_level p, split_level k) const override {
    assert(p == 0);
    return s_.Inconsistent(k);
  }

  bool EntailsAt(belief_level p,
                 const SimpleClause& c,
                 split_level k) const override {
    assert(p == 0);
    ++queries;
    std::cout << __FILE__ << ":" << __LINE__ << ": belief level " << p << ": split level " << k << ": " << c << std::endl;
    return s_.Entails(c, k);
  }

 private:
  esbl::Setup s_;
  StdName::SortedSet ns_;
};

TEST(formula, fol_incompleteness_positive1) {
  // The tautology (A x . E y . ~P(x) v P(y)) is provable in our variant of ESL.
  // Is it provable in the paper? Don't remember, check. TODO(chs)
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  auto q = Formula::Forall(x, Formula::Exists(y,
              Formula::Or(Formula::Lit(Literal({}, true, 0, {x})),
                          Formula::Lit(Literal({}, false, 0, {y})))));
  EmptyBat bat;
  for (Setup::split_level k = 1; k < 2; ++k) {
    EXPECT_EQ(bat.Entails(Formula::Know(k, q->Copy())), k > 0);
    EXPECT_EQ(bat.Entails(Formula::Know(k, q->Copy())), k > 0);
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
  EmptyBat bat;
  for (Setup::split_level k = 0; k < 5; ++k) {
    //EXPECT_EQ(bat.Entails(Formula::Know(k, q->Copy())), k > 0);
    // It holds even for k = 0 because our CNF we drop tautologous clauses from
    // the CNF.
    EXPECT_TRUE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_TRUE(bat.Entails(Formula::Know(k, q->Copy())));
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
  EmptyBat bat;
  for (Setup::split_level k = 1; k < 2; ++k) {
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
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
  EmptyBat bat;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
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
  EmptyBat bat;
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
  }
}

TEST(formula, fol_setup_universal) {
  // The setup { P(x) } should entail (A y . P(y)).
  EmptyBat bat;
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  bat.AddClause(Clause(Ewff::TRUE, SimpleClause({Literal({}, true, 0, {x})})));
  auto q = Formula::Forall(y, Formula::Lit(Literal({}, true, 0, {y})));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_TRUE(bat.Entails(Formula::Know(k, q->Copy())));
  }
}

TEST(formula, query_resolution) {
  // The query (p v q) ^ (~p v q) is subsumed by setup {q} for split k > 0.
  // And since we minimize the CNF, we obtain the query {q} and thus the query
  // should hold for k = 0 as well.
  EmptyBat bat;
  Term::Factory tf;
  const Literal p = Literal({}, true, 0, {});
  const Literal q = Literal({}, true, 1, {});
  bat.AddClause(Clause(Ewff::TRUE, SimpleClause({q})));
  auto phi = Formula::And(Formula::Or(Formula::Lit(q), Formula::Lit(p)),
                          Formula::Or(Formula::Lit(q), Formula::Lit(p.Flip())));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(bat.Entails(Formula::Know(k, phi->Copy())));
    EXPECT_TRUE(bat.Entails(Formula::Know(k, phi->Copy())));
  }
}

TEST(formula, fol_grounding1) {
  // Check that variables are actually not grounded.
  auto P = [](const Term& t) { return Literal({}, true, 0, {t}); };
  auto Q = [](const Term& t) { return Literal({}, true, 1, {t}); };
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  const Variable z = tf.CreateVariable(0);
  auto p = Formula::Exists(x, Formula::Exists(x, Formula::Exists(z,
      Formula::And(Formula::Lit(P(x)), Formula::And(Formula::Lit(P(y)), Formula::Lit(P(z)))))));
  auto q = Formula::Exists(x, Formula::Exists(x, Formula::Exists(z,
      Formula::And(Formula::Lit(Q(x)), Formula::And(Formula::Lit(Q(y)), Formula::Lit(Q(z)))))));
  EmptyBat bat;
  bat.AddClause(Clause(Ewff::TRUE, {P(x)}));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(bat.Entails(Formula::Know(k, p->Copy())));
    EXPECT_TRUE(bat.Entails(Formula::Know(k, p->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, Formula::Neg(q->Copy()))));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, Formula::Neg(q->Copy()))));
  }
}

TEST(formula, fol_grounding2) {
  // Check that variables are actually not grounded.
  auto P = [](const Term& t) { return Literal({t}, true, 0, {t}); };
  auto Q = [](const Term& t) { return Literal({t}, true, 1, {t}); };
  Term::Factory tf;
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  const Variable z = tf.CreateVariable(0);
  auto p = Formula::Exists(x, Formula::Exists(x, Formula::Exists(z,
      Formula::And(Formula::Lit(P(x)), Formula::And(Formula::Lit(P(y)), Formula::Lit(P(z)))))));
  auto q = Formula::Exists(x, Formula::Exists(x, Formula::Exists(z,
      Formula::And(Formula::Lit(Q(x)), Formula::And(Formula::Lit(Q(y)), Formula::Lit(Q(z)))))));
  EmptyBat bat;
  bat.AddClause(Clause(Ewff::TRUE, {P(x)}));
  for (Setup::split_level k = 0; k < 5; ++k) {
    EXPECT_TRUE(bat.Entails(Formula::Know(k, p->Copy())));
    EXPECT_TRUE(bat.Entails(Formula::Know(k, p->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, q->Copy())));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, Formula::Neg(q->Copy()))));
    EXPECT_FALSE(bat.Entails(Formula::Know(k, Formula::Neg(q->Copy()))));
  }
}

TEST(formula, evals) {
  Testbat bat;
  Term::Factory tf;
  Term t = tf.CreateStdName(0, 0);
  auto phi1 = Formula::Act(t, Formula::Know(2, Formula::Lit(Literal({}, false, bat.p, {}))));
  //auto phi2 = Formula::Act(t, Formula::Know(2, Formula::Lit(Literal({}, false, bat.p, {}))));
  //EXPECT_FALSE(bat.Entails(phi1->Regress(&tf, bat)));
  //EXPECT_FALSE(bat.Entails(phi2->Regress(&tf, bat)));
  bat.Add(Formula::Lit(Literal({}, false, Atom::SF, {t}))->ObjRegress(&tf, bat));
  std::cout << *phi1 << std::endl;
  std::cout << *phi1->Regress(&tf, bat) << std::endl;
  EXPECT_TRUE(bat.Entails(phi1->Regress(&tf, bat)));
  //EXPECT_FALSE(bat.Entails(phi2->Regress(&tf, bat)));
  std::cout << queries << " Queries" << std::endl;
}

