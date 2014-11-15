// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./kr2014.h>
#include <./clause.h>

using namespace esbl;
using namespace bats;

TEST(setup, gl_static) {
  Kr2014 bat;
  auto& s = bat.setup();
  s.GuaranteeConsistency(3);
  EXPECT_TRUE(s.Entails({Literal({}, false, bat.d0, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, false, bat.d1, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d0, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d1, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d2, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d2, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d3, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d3, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, true, bat.d2, {}),
                         Literal({}, true, bat.d3, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d2, {}),
                          Literal({}, false, bat.d3, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, true, bat.d1, {}),
                         Literal({}, true, bat.d2, {}),
                         Literal({}, true, bat.d3, {})}, 0));
}

TEST(setup, gl_dynamic) {
  Kr2014 bat;
  auto& s = bat.setup();
  s.GuaranteeConsistency(3);
  EXPECT_TRUE(s.Entails({Literal({bat.forward}, false, bat.d0, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({bat.forward}, true, bat.d0, {})}, 0));
  s.AddClause(Clause(Ewff::TRUE, {Literal({}, true, Atom::SF, {bat.forward})}));
  EXPECT_FALSE(s.Entails({Literal({bat.forward}, true, bat.d1, {}),
                          Literal({bat.forward}, true, bat.d2, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({bat.forward}, true, bat.d1, {}),
                         Literal({bat.forward}, true, bat.d2, {})}, 1));
  s.AddClause(Clause(Ewff::TRUE, {Literal({bat.forward}, true, Atom::SF, {bat.sonar})}));
  const TermSeq z = {bat.forward, bat.sonar};
  EXPECT_TRUE(s.Entails({Literal(z, false, bat.d0, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal(z, false, bat.d0, {})}, 1));
  EXPECT_TRUE(s.Entails({Literal(z, true, bat.d1, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal(z, true, bat.d1, {})}, 1));
}

TEST(setup, eventual_completeness_static) {
  esbl::Setup s;
  const Literal p({}, true, 1, {});
  const Literal q({}, true, 2, {});
  EXPECT_FALSE(s.Entails({p, p.Flip()}, 0));
  EXPECT_TRUE(s.Entails({p, p.Flip()}, 1));
  EXPECT_TRUE(s.Entails({p, p.Flip()}, 2));
  EXPECT_FALSE(s.Entails({p, q}, 0));
  EXPECT_FALSE(s.Entails({p, q}, 1));
  EXPECT_FALSE(s.Entails({p, q}, 2));
}

#if 0
// This test not hold because PEL only considers static literals.
// Whether this limitation is actually feasible needs to be investigated.
TEST(setup, eventual_completeness_dynamic) {
  Term::Factory tf;
  const StdName a = tf.CreateStdName(1, 1);
  const StdName b = tf.CreateStdName(2, 2);
  esbl::Setup s;
  const Literal p({a}, true, 1, {b});
  const Literal q({b}, true, 2, {a});
  EXPECT_FALSE(s.Entails({p, p.Flip()}, 0));
  EXPECT_TRUE(s.Entails({p, p.Flip()}, 1));
  EXPECT_TRUE(s.Entails({p, p.Flip()}, 2));
  EXPECT_FALSE(s.Entails({p, q}, 0));
  EXPECT_FALSE(s.Entails({p, q}, 1));
  EXPECT_FALSE(s.Entails({p, q}, 2));
}
#endif

TEST(setup, inconsistency) {
  for (int i = -3; i <= 3; ++i) {
    esbl::Setup s;
    const Literal a = Literal({}, true, 1, {});
    const Literal b = Literal({}, true, 2, {});
    s.AddClause(Clause(Ewff::TRUE, {a, b}));
    s.AddClause(Clause(Ewff::TRUE, {a, b.Flip()}));
    s.AddClause(Clause(Ewff::TRUE, {a.Flip(), b}));
    s.AddClause(Clause(Ewff::TRUE, {a.Flip(), b.Flip()}));
    EXPECT_FALSE(s.Inconsistent(0));
    if (i < 0) {
      for (int k = -i; k >= 0; --k) {
        EXPECT_EQ(s.Inconsistent(k), (k > 0));
        EXPECT_EQ(s.Entails(SimpleClause::EMPTY, k), (k > 0));
        //printf("%sconsistent k=%d\n", (k>0 ? "in" : ""), k);
      }
    } else {
      for (int k = 0; k <= i; ++k) {
        EXPECT_EQ(s.Inconsistent(k), (k > 0));
        EXPECT_EQ(s.Entails(SimpleClause::EMPTY, k), (k > 0));
        //printf("%sconsistent k=%d\n", (k>0 ? "in" : ""), k);
      }
    }
  }
}

TEST(setup, eventual_inconsistency_long) {
  constexpr size_t SETUP_SIZE = 6;
  for (size_t n = 1; n < SETUP_SIZE; ++n) {
    // Create a setup over n literals. For each combination of signs, there
    // shall be one clause in the setup. That is, we have 2^n clauses.
    esbl::Setup s;
    for (size_t i = 0; i < (1U << n); ++i) {
      // Create the i-th of 2^n many clauses.
      SimpleClause c;
      for (size_t bit = 0; bit < n; ++bit) {
        const bool sign = ((i >> bit) & 1) != 0;
        const Literal l({}, sign, bit+1, {});
        c.insert(l);
      }
      s.AddClause(Clause(Ewff::TRUE, c));
    }
    for (size_t k = 0; k < n-1; ++k) {
      EXPECT_FALSE(s.Inconsistent(k));
    }
    for (size_t k = n-1; k <= n+1; ++k) {
      EXPECT_TRUE(s.Inconsistent(k));
    }
  }
}

TEST(setup, eventual_consistency_long) {
  constexpr size_t SETUP_SIZE = 6;
  for (size_t n = 1; n < SETUP_SIZE; ++n) {
    // Create a setup over n literals. For each combination of signs, there
    // shall be one clause in the setup. That is, we have 2^n clauses.
    esbl::Setup s;
    const SimpleClause query({Literal({}, true, n+1, {})});
    for (size_t i = 0; i < (1U << n); ++i) {
      // Create the i-th of 2^n many clauses.
      SimpleClause c;
      for (size_t bit = 0; bit < n; ++bit) {
        const bool sign = ((i >> bit) & 1) != 0;
        const Literal l({}, sign, bit+1, {});
        c.insert(l);
      }
      s.AddClause(Clause(Ewff::TRUE, c));
    }
    for (size_t k = 0; k < n-1; ++k) {
      EXPECT_FALSE(s.Entails(query, k));
    }
    for (size_t k = n-1; k <= n+1; ++k) {
      EXPECT_TRUE(s.Entails(query, k));
    }
  }
}

