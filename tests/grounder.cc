// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/formula.h>
#include <lela/grounder.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format;

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

TEST(GrounderTest, Ground_SplitTerms_Names) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();                  RegisterSort(s1, "");
  const Symbol::Sort s2 = sf.CreateSort();                  RegisterSort(s2, "");
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));         RegisterSymbol(n1.symbol(), "n1");
  const Term n2 = tf.CreateTerm(sf.CreateName(s2));         RegisterSymbol(n2.symbol(), "n2");
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x1.symbol(), "x1");
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x2.symbol(), "x2");
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x3.symbol(), "x3");
  //const Term x4 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x4, "x4");
  const Symbol a = sf.CreateFunction(s1, 0);                RegisterSymbol(a, "a");
  const Symbol f = sf.CreateFunction(s1, 1);                RegisterSymbol(f, "f");
  //const Symbol g = sf.CreateFunction(s2, 1);                RegisterSymbol(g, "g");
  const Symbol h = sf.CreateFunction(s2, 2);                RegisterSymbol(h, "h");
  //const Symbol i = sf.CreateFunction(s2, 2);
  //const Term c1 = tf.CreateTerm(a, {});
  //const Term f1 = tf.CreateTerm(f, {n1});
  //const Term f2 = tf.CreateTerm(h, {n1,x2});
  //const Term f3 = tf.CreateTerm(g, {f1});
  //const Term f4 = tf.CreateTerm(h, {n1,f1});
  //const Term f5 = tf.CreateTerm(i, {x1,x3});

#if 0 // Disabled because:
      // We allow only quasi-primitive formulas to be grounded.
      // (Actually, after Clause::Minimize() some of the following clauses are
      // quasi-primitive; e.g., [n1/=n1] is reduced to [].)
  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(n1,n1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(length(s.clauses()), 0);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1,n1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1, x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(length(s.clauses()), 0);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1, x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(n1, x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1], [n2=n1]. The first clause is valid and
    // hence skipped. The second is invalid and hence boiled down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1, x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1/=n1], [n2/=n1]. The second clause is valid and
    // hence skipped. The first is invalid and hence boiled down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1, x2)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1=n1], [n2=2], [n1=n2], [n2=n1]. The former two
    // clauses are valid and hence skipped. The latter ones are invalid and
    // hence boiled down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1, x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }
#endif

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(a, {}), x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [a=n1].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(f, {n1}), x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [f(n1)=n1)], [f(n1)=n2]. The clauses unify and
    // yield [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(tf.CreateTerm(f, {n1}), x2)}));
    lela::Setup s = g.Ground();
    // Grounding should be [f(n1)/=n1)], [f(n1)/=n2].
    EXPECT_EQ(length(s.clauses()), 3);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(h, {n1,x2}), x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [h(n1,nX)=nY] for X=1,2,3 and Y=1,2,3. The clauses
    // unify and yield [].
    EXPECT_EQ(length(s.clauses()), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(tf.CreateTerm(h, {n1,x2}), x1)}));
    lela::Setup s = g.Ground();
    // Grounding should be [h(n1,nX)=nY] for X=1,2,3,4 and Y=1,2,3,4.
    EXPECT_EQ(length(s.clauses()), 4*4);
    EXPECT_TRUE(s.Consistent());
  }

//  {
//    Grounder g(&sf, &tf);
//    g.AddClause(Clause({Literal::Neq(c1, x1)}));
//    g.PrepareForQuery(Formula::Exists(x1, Formula::Clause({Literal::Eq(x1, x1)})).reader());
//    g.PrepareForQuery(Formula::Exists(x2, Formula::Clause({Literal::Eq(x2, x2)})).reader());
//    g.PrepareForQuery(Formula::Exists(x3, Formula::Clause({Literal::Eq(x3, x3)})).reader());
//    g.PrepareForQuery(Formula::Exists(x4, Formula::Clause({Literal::Eq(x4, x4)})).reader());
//    lela::Setup s = g.Ground();
//    EXPECT_EQ(length(s.clauses()), 2);
//  }

  {
    //Formula phi = Formula::Exists(x3, Formula::Clause({Literal::Eq(tf.CreateTerm(h, {n1,x3}), tf.CreateTerm(a, {}))}));
    Formula phi = Formula::Exists(x3, Formula::Clause({Literal::Eq(tf.CreateTerm(h, {n1,x3}), tf.CreateTerm(f, {tf.CreateTerm(a, {})}))}));
    Grounder g(&sf, &tf);
    g.PrepareForQuery(1, phi.reader());
    Grounder::TermSet terms = g.SplitTerms();
    Grounder::SortedTermSet names = g.Names();
    //std::cout << phi << std::endl;
    //std::cout << names << std::endl;
    //std::cout << terms << std::endl;
    EXPECT_EQ(names.size(), 2);
    EXPECT_EQ(names[n1.symbol().sort()].size(), 2);
    EXPECT_EQ(names[x3.symbol().sort()].size(), 1);
    EXPECT_EQ(names[a.sort()].size(), 2);
    EXPECT_EQ(names[f.sort()].size(), 2);
    EXPECT_EQ(names[h.sort()].size(), 1);
    Term nx3 = *names[x3.symbol().sort()].begin();
    Term nSplit =  names[a.sort()][ names[a.sort()][0] == n1 ? 1 : 0 ];
    EXPECT_NE(nx3, n1);
    EXPECT_NE(nSplit, n1);
    EXPECT_EQ(std::set<Term>(terms.begin(), terms.end()),
              std::set<Term>({tf.CreateTerm(a, {}), tf.CreateTerm(f, {n1}), tf.CreateTerm(f, {nSplit}), tf.CreateTerm(h, {n1, nx3})}));
  }

  {
    Clause c{Literal::Eq(tf.CreateTerm(h, {n1,n2}), n2)}; 
    Clause d{Literal::Eq(tf.CreateTerm(h, {x1,n2}), n2)}; 
    Clause e{Literal::Eq(tf.CreateTerm(f, {x1}), n1)}; 
    Formula phi = Formula::Exists(x3, Formula::Clause({Literal::Eq(tf.CreateTerm(h, {n1,x3}), x3)}));
    Grounder g(&sf, &tf);
    const class Setup* last;
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.Ground();
      EXPECT_EQ(length(s->clauses()), 0);
      EXPECT_EQ(g.setups_.size(), 1);
      last = s;
    }
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      EXPECT_EQ(length(s->clauses()), 0);
      EXPECT_EQ(g.setups_.size(), 1);
      EXPECT_EQ(s, last);
      last = s;
    }
    g.AddClause(c);  // adds new name, re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      EXPECT_EQ(length(s->clauses()), 1);
      EXPECT_EQ(g.setups_.size(), 1);
      last = s;
    }
    g.PrepareForQuery(0, phi.reader());  // adds new plus name, re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      //EXPECT_NE(last, s);
      EXPECT_EQ(length(s->clauses()), 1);
      EXPECT_EQ(g.setups_.size(), 1);
      last = s;
    }
    g.AddClause(d);  // adds two new plus names (one for x, one for the Lemma 8 fix), re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      //EXPECT_NE(last, s);
      EXPECT_EQ(length(s->clauses()), 3);
      EXPECT_EQ(g.setups_.size(), 1);
      last = s;
    }
    g.PrepareForQuery(1, phi.reader());  // adds no new plus name, 
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      EXPECT_EQ(last, s);
      EXPECT_EQ(length(s->clauses()), 3);
      EXPECT_EQ(g.setups_.size(), 1);
      last = s;
    }
    g.AddClause(e);  // adds no new names
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      const class Setup* s = &g.Ground();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 3);
      EXPECT_NE(last, s);
      EXPECT_EQ(length(s->clauses()), 3+3);
      EXPECT_EQ(g.setups_.size(), 2);
      last = s;
    }
  }
}

}  // namespace lela

