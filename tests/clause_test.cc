// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./clause.h>

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

static Atom::PredId O = 2;
static Atom::PredId P = 1;
static Atom::PredId Q = 2;

Literal::Set Rel(const Clause& c, const StdName::SortedSet& hplus, const Literal&l) {
  std::deque<Literal> q;
  c.Rel(hplus, l, &q);
  Literal::Set s(q.begin(), q.end());
  return s;
}

TEST(clause, rel)
{
  Clause empty(Ewff::TRUE, SimpleClause());
  Clause c1(Ewff::Create({{x2,n2}, {x3,n3}, {x2,n1}}, {}).val,
            SimpleClause({ Literal(true, P, {n1,x2}),
                           Literal(false, P, {n1,x2}) }));
  Clause c2(Ewff::Create({}, {{x5,x6}}).val,
            SimpleClause({ Literal(true, P, {x4,x6}),
                           Literal(false, Q, {x4,x4}) }));
  Clause c3(Ewff::Create({}, {}).val,
            SimpleClause({ Literal(true, P, {x1}),
                           Literal(false, Q, {x1,x6}) }));
  Clause c4(Ewff::Create({}, {}).val,
            SimpleClause({ Literal(true, P, {x5}),
                           Literal(false, Q, {x5,x6}) }));

  EXPECT_EQ(Rel(empty, hplus, Literal(false, P, {n1,n4})).size(), 0);
  EXPECT_EQ(Rel(empty, hplus, Literal(true, P, {n1,n4})).size(), 0);
  EXPECT_EQ(Rel(c1, hplus, Literal(false, P, {n1,n4})).size(), 1);
  EXPECT_EQ(Rel(c1, hplus, Literal(true, P, {n1,n4})).size(), 1);
  EXPECT_EQ(Rel(c2, hplus, Literal(false, P, {n1,n4})).size(), 0);
  EXPECT_EQ(Rel(c2, hplus, Literal(true, P, {n1,n4})).size(), 1);
  EXPECT_EQ(Rel(c2, hplus, Literal(true, P, {n1,n4})).size(), 1);
  EXPECT_EQ(Rel(c2, hplus, Literal(false, P, {n2,n4})).size(), 0);
  EXPECT_EQ(Rel(c2, hplus, Literal(true, P, {n2,n4})).size(), 1);
  EXPECT_EQ(Rel(c3, hplus, Literal(false, P, {n2})).size(), 0);
  EXPECT_EQ(Rel(c3, hplus, Literal(true, P, {n2})).size(), 1);
  EXPECT_EQ(Rel(c3, hplus, Literal(true, P, {n1})).size(), 1);
  for (const StdName& n : names) {
    EXPECT_TRUE(Rel(c3, hplus, Literal(true, P, {n})).size() == 1);
    EXPECT_TRUE(Rel(c3, hplus, Literal(true, P, {n})) == Literal::Set({Literal(true, Q, {n,x6})}));
  }
  for (const StdName& n : names) {
    EXPECT_TRUE(Rel(c3, hplus, Literal(false, Q, {n,n})).size() == 1);
    EXPECT_TRUE(Rel(c3, hplus, Literal(false, Q, {n,n})) == Literal::Set({Literal(false, P, {n})}));
  }
  EXPECT_EQ(Rel(c4, hplus, Literal(false, P, {n2})).size(), 0);
  EXPECT_EQ(Rel(c4, hplus, Literal(true, P, {n2})).size(), 1);
  EXPECT_EQ(Rel(c4, hplus, Literal(false, Q, {n2,x3})).size(), 1);
  EXPECT_EQ(Rel(c4, hplus, Literal(false, Q, {n2,x6})).size(), 1);
}

TEST(clause, subsumption)
{
  Clause empty(Ewff::TRUE, SimpleClause());
  Clause c1(Ewff::Create({{x2,n2}, {x2,n3}, {x2,n1}}, {}).val,
            {Literal(true, P, {n1,x2}),
             Literal(false, P, {n1,x2})});
  Clause c2(Ewff::Create({}, {{x4,x6}}).val,
            {Literal(true, P, {x4,x6}),
             Literal(false, Q, {x4,x4})});
  Clause c3(Ewff::Create({}, {{x4,x6}}).val,
            {Literal(true, O, {x4,x6}),
             Literal(true, P, {x4,x6}),
             Literal(false, Q, {x4,x4})});
  Clause d1(Ewff::TRUE,
            {Literal(true, P, {n1,n4}),
             Literal(false, P, {n1,n4})});
  Clause d2(Ewff::TRUE,
            {Literal(true, P, {n4,n6}),
             Literal(false, Q, {n4,n4})});
  Clause d3(Ewff::TRUE,
            {Literal(true, O, {n4,n6}),
             Literal(true, P, {n4,n6}),
             Literal(false, Q, {n4,n4})});

  EXPECT_TRUE(empty.Subsumes(d1));
  EXPECT_TRUE(empty.Subsumes(d2));
  EXPECT_TRUE(empty.Subsumes(d3));

  EXPECT_TRUE(c1.Subsumes(d1));
  EXPECT_FALSE(c1.Subsumes(d2));
  EXPECT_FALSE(c1.Subsumes(d3));

  EXPECT_FALSE(c2.Subsumes(d1));
  EXPECT_TRUE(c2.Subsumes(d2));
  EXPECT_TRUE(c2.Subsumes(d3));

  EXPECT_FALSE(c3.Subsumes(d1));
  EXPECT_FALSE(c3.Subsumes(d2));
  EXPECT_TRUE(c3.Subsumes(d3));

  for (const auto& c : {c1,c2,c3}) {
    for (const auto& d : {d1,d2,d3}) {
      EXPECT_FALSE(d.Subsumes(c));
    }
  }
}

TEST(clause, tautologous) {
  Term::Factory tf;
  const StdName m = tf.CreateStdName(1, 0);
  const StdName n = tf.CreateStdName(0, 0);
  const Variable x = tf.CreateVariable(0);
  const Variable y = tf.CreateVariable(0);
  const Atom::PredId P = 0;
  const Atom::PredId Q = 1;
  Clause empty(Ewff::TRUE, SimpleClause());
  Clause tauto0(Ewff::Create({}, {}).val, {Literal(true, P, {m}), Literal(false, P, {m})});
  Clause tauto1(Ewff::Create({}, {}).val, {Literal(true, P, {x}), Literal(false, P, {x})});
  Clause tauto2(Ewff::Create({}, {}).val, {Literal(true, P, {x}), Literal(false, P, {y})});
  Clause tauto3(Ewff::Create({{x, m}, {y, n}}, {}).val, {Literal(true, P, {x}), Literal(false, P, {x})});
  Clause tauto4(Ewff::Create({{x, m}, {y, n}}, {}).val, {Literal(true, P, {x}), Literal(false, P, {y})});
  Clause tauto5(Ewff::Create({{x, m}, {y, n}}, {{x, y}}).val, {Literal(true, P, {x}), Literal(false, P, {x})});
  Clause nontauto0(Ewff::Create({}, {}).val, {Literal(true, P, {x}), Literal(true, P, {y})});
  Clause nontauto1(Ewff::Create({}, {}).val, {Literal(true, P, {x}), Literal(false, Q, {y})});
  Clause nontauto2(Ewff::Create({}, {}).val, {Literal(true, P, {m}), Literal(false, P, {n})});
  Clause nontauto3(Ewff::Create({{x, m}, {y, n}}, {{x, y}}).val, {Literal(true, P, {m}), Literal(false, P, {n})});
  Clause nontauto4(Ewff::Create({{x, m}, {y, n}}, {{x, y}}).val, {Literal(true, P, {x}), Literal(false, P, {y})});
  Clause nontauto5(Ewff::Create({}, {{x, y}}).val, {Literal(true, P, {x}), Literal(false, P, {y})});
  EXPECT_FALSE(empty.Tautologous());
  EXPECT_TRUE(tauto0.Tautologous());
  EXPECT_TRUE(tauto1.Tautologous());
  EXPECT_TRUE(tauto2.Tautologous());
  EXPECT_TRUE(tauto3.Tautologous());
  EXPECT_TRUE(tauto4.Tautologous());
  EXPECT_TRUE(tauto5.Tautologous());
  EXPECT_FALSE(nontauto1.Tautologous());
  EXPECT_FALSE(nontauto2.Tautologous());
  EXPECT_FALSE(nontauto3.Tautologous());
  EXPECT_FALSE(nontauto4.Tautologous());
  EXPECT_FALSE(nontauto5.Tautologous());
}

