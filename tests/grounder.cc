// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <gtest/gtest.h>

#include <unordered_set>

#include <limbo/formula.h>
#include <limbo/grounder.h>
#include <limbo/format/output.h>

using limbo::format::operator<<;

namespace limbo {

using namespace limbo::format;

std::unordered_set<Clause> unique(const Setup& s) {
  std::unordered_set<Clause> set;
  for (auto i : s.clauses()) {
    set.insert(s.clause(i));
  }
  return set;
}

size_t unique_length(const Setup& s) { return unique(s).size(); }

template<typename T>
size_t length(T r) {
  size_t n = 0;
  for (auto it = r.begin(); it != r.end(); ++it) {
    ++n;
  }
  return n;
}

typedef std::unordered_set<Clause> ClauseSet;
typedef std::unordered_set<Term> TermSet;
typedef std::unordered_set<Literal> LiteralSet;

ClauseSet S(const Setup& s) {
  ClauseSet set;
  for (auto i : s.clauses()) {
    set.insert(s.clause(i));
  }
  return set;
}
TermSet S(const Grounder::Names& ns) { return TermSet(ns.begin(), ns.end()); }
TermSet S(const Grounder::LhsTerms& ts) { return TermSet(ts.begin(), ts.end()); }
TermSet S(const Grounder::RhsNames& ns) { return TermSet(ns.begin(), ns.end()); }
//LiteralSet S(const Grounder::LhsRhsLiterals& as) { return LiteralSet(as.begin(), as.end()); }

std::pair<TermSet, size_t> operator+(const TermSet& ts, size_t n) { return std::make_pair(ts, n); }

bool operator==(const TermSet& ts, const std::pair<TermSet, size_t>& p) {
  for (Term t : p.first) {
    if (ts.find(t) == ts.end()) {
      return false;
    }
  }
  return ts.size() == p.first.size() + p.second;
}

TEST(GrounderTest, Ground_SplitTerms_Names) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort sa = sf.CreateSort();                  RegisterSort(sa, "");
  const Symbol::Sort sb = sf.CreateSort();                  RegisterSort(sb, "");
  const Term m1 = tf.CreateTerm(sf.CreateName(sa));         RegisterSymbol(m1.symbol(), "m1");
  const Term m2 = tf.CreateTerm(sf.CreateName(sa));         RegisterSymbol(m2.symbol(), "m2");
  const Term n1 = tf.CreateTerm(sf.CreateName(sb));         RegisterSymbol(n1.symbol(), "n1");
  const Term n2 = tf.CreateTerm(sf.CreateName(sb));         RegisterSymbol(n2.symbol(), "n2");
  const Term x1 = tf.CreateTerm(sf.CreateVariable(sa));     RegisterSymbol(x1.symbol(), "x1");
  const Term x2 = tf.CreateTerm(sf.CreateVariable(sa));     RegisterSymbol(x2.symbol(), "x2");
  const Term y1 = tf.CreateTerm(sf.CreateVariable(sb));     RegisterSymbol(y1.symbol(), "y1");
  const Symbol s_a = sf.CreateFunction(sa, 0);              RegisterSymbol(s_a, "a");
  const Symbol s_b = sf.CreateFunction(sb, 0);              RegisterSymbol(s_b, "b");
  const Symbol s_f = sf.CreateFunction(sa, 1);              RegisterSymbol(s_f, "f");
  const Symbol s_g = sf.CreateFunction(sb, 1);              RegisterSymbol(s_g, "g");
  const Term a = tf.CreateTerm(s_a, {});
  const Term b = tf.CreateTerm(s_b, {});
  const Term fm1 = tf.CreateTerm(s_f, {m1});
  const Term fm2 = tf.CreateTerm(s_f, {m2});
  const Term fn1 = tf.CreateTerm(s_f, {n1});
  const Term fn2 = tf.CreateTerm(s_f, {n2});
  const Term gm1 = tf.CreateTerm(s_f, {m1});
  const Term gm2 = tf.CreateTerm(s_f, {m2});
  const Term gn1 = tf.CreateTerm(s_f, {n1});
  const Term gn2 = tf.CreateTerm(s_f, {n2});
  {
    Grounder g(&sf, &tf);
    Grounder h(&sf, &tf);
    g.AddClause(Clause{Literal::Eq(a, m1)});
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1}));
    EXPECT_EQ(S(g.names(sb)), TermSet({}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
    g.AddClause(Clause{Literal::Eq(a, m1)});
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1}));
    EXPECT_EQ(S(g.names(sb)), TermSet({}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
    g.AddClause(Clause{Literal::Eq(fm1, m1), Literal::Eq(fm1, m2), Literal::Eq(fn1, m2)});
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)}), Clause({Literal::Eq(fm1, m1), Literal::Eq(fm1, m2), Literal::Eq(fn1, m2)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1, m2}));
    EXPECT_EQ(S(g.names(sb)), TermSet({n1}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a, fm1, fn1}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({m1, m2}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({m2}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
  }
}

TEST(GrounderTest, Ground_SplitTerms_Names_Consolidated) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort sa = sf.CreateSort();                  RegisterSort(sa, "");
  const Symbol::Sort sb = sf.CreateSort();                  RegisterSort(sb, "");
  const Term m1 = tf.CreateTerm(sf.CreateName(sa));         RegisterSymbol(m1.symbol(), "m1");
  const Term m2 = tf.CreateTerm(sf.CreateName(sa));         RegisterSymbol(m2.symbol(), "m2");
  const Term n1 = tf.CreateTerm(sf.CreateName(sb));         RegisterSymbol(n1.symbol(), "n1");
  const Term n2 = tf.CreateTerm(sf.CreateName(sb));         RegisterSymbol(n2.symbol(), "n2");
  const Term x1 = tf.CreateTerm(sf.CreateVariable(sa));     RegisterSymbol(x1.symbol(), "x1");
  const Term x2 = tf.CreateTerm(sf.CreateVariable(sa));     RegisterSymbol(x2.symbol(), "x2");
  const Term y1 = tf.CreateTerm(sf.CreateVariable(sb));     RegisterSymbol(y1.symbol(), "y1");
  const Symbol s_a = sf.CreateFunction(sa, 0);              RegisterSymbol(s_a, "a");
  const Symbol s_b = sf.CreateFunction(sb, 0);              RegisterSymbol(s_b, "b");
  const Symbol s_f = sf.CreateFunction(sa, 1);              RegisterSymbol(s_f, "f");
  const Symbol s_g = sf.CreateFunction(sb, 1);              RegisterSymbol(s_g, "g");
  const Term a = tf.CreateTerm(s_a, {});
  const Term b = tf.CreateTerm(s_b, {});
  const Term fm1 = tf.CreateTerm(s_f, {m1});
  const Term fm2 = tf.CreateTerm(s_f, {m2});
  const Term fn1 = tf.CreateTerm(s_f, {n1});
  const Term fn2 = tf.CreateTerm(s_f, {n2});
  const Term gm1 = tf.CreateTerm(s_f, {m1});
  const Term gm2 = tf.CreateTerm(s_f, {m2});
  const Term gn1 = tf.CreateTerm(s_f, {n1});
  const Term gn2 = tf.CreateTerm(s_f, {n2});
  {
    Grounder g(&sf, &tf);
    Grounder h(&sf, &tf);
    g.AddClause(Clause{Literal::Eq(a, m1)});
    g.Consolidate();
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1}));
    EXPECT_EQ(S(g.names(sb)), TermSet({}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
    g.AddClause(Clause{Literal::Eq(a, m1)});
    g.Consolidate();
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1}));
    EXPECT_EQ(S(g.names(sb)), TermSet({}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
    g.AddClause(Clause{Literal::Eq(fm1, m1), Literal::Eq(fm1, m2), Literal::Eq(fn1, m2)});
    g.Consolidate();
    EXPECT_EQ(S(g.setup()), ClauseSet({Clause({Literal::Eq(a, m1)}), Clause({Literal::Eq(fm1, m1), Literal::Eq(fm1, m2), Literal::Eq(fn1, m2)})}));
    EXPECT_EQ(S(g.names(sa)), TermSet({m1, m2}));
    EXPECT_EQ(S(g.names(sb)), TermSet({n1}));
    EXPECT_EQ(S(g.lhs_terms()), TermSet({a, fm1, fn1}));
    EXPECT_EQ(S(g.rhs_names(a)), TermSet({m1}) + 1);
    EXPECT_EQ(S(g.rhs_names(b)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm1)), TermSet({m1, m2}) + 1);
    EXPECT_EQ(S(g.rhs_names(fm2)), TermSet({}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn1)), TermSet({m2}) + 1);
    EXPECT_EQ(S(g.rhs_names(fn2)), TermSet({}) + 1);
  }
}

#if 0
TEST(GrounderTest, Ground_SplitTerms_Names) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort s1 = sf.CreateSort();                  RegisterSort(s1, "");
  const Symbol::Sort s2 = sf.CreateSort();                  RegisterSort(s2, "");
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));         RegisterSymbol(n1.symbol(), "n1");
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));         RegisterSymbol(n2.symbol(), "n2");
  const Term n3 = tf.CreateTerm(sf.CreateName(s2));         RegisterSymbol(n3.symbol(), "n3");
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x1.symbol(), "x1");
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x2.symbol(), "x2");
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x3.symbol(), "x3");
  //const Term x4 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x4, "x4");
  const Symbol a = sf.CreateFunction(s1, 0);                RegisterSymbol(a, "a");
  const Symbol f = sf.CreateFunction(s1, 1);                RegisterSymbol(f, "f");
  const Symbol g = sf.CreateFunction(s2, 1);                RegisterSymbol(g, "g");
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
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(unique_length(s), 0);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1,n1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1, x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1=n1]. The clause is valid and hence skipped.
    EXPECT_EQ(unique_length(s), 0);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1, x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(n1, x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1=n1], [n3=n1]. The first clause is valid and
    // hence skipped. The second is invalid and hence boiled down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(n1, x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1/=n1], [n3/=n1]. The second clause is valid and
    // hence skipped. The first is invalid and hence boiled down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(x1, x2)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1=n1], [n3=2], [n1=n3], [n3=n1]. The former two
    // clauses are valid and hence skipped. The latter ones are invalid and
    // hence boiled down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(x1, x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [n1/=n1]. The clause is invalid and hence boiled
    // down to [].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }
#endif

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(a, {}), x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [a=n1], [a=n2], which yields [].
    //EXPECT_EQ(unique_length(s), 2);  // the second clause is replaced with []
    //const_cast<limbo::Setup&>(s).Minimize();
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(a, {}), n1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [a=n1].
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(f, {n1}), x1)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [f(n1)=n1)], [f(n1)=n3]. The clauses unify and
    // yield [].
    //EXPECT_EQ(unique_length(s), 2);
    //const_cast<limbo::Setup&>(s).Minimize();
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(tf.CreateTerm(f, {n1}), x2)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [f(n1)/=n1)], [f(n1)/=n3], [f(n1)/=n4].
    //EXPECT_EQ(unique_length(s), 3);
    //const_cast<limbo::Setup&>(s).Minimize();
    EXPECT_EQ(unique_length(s), 3);
    EXPECT_TRUE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Eq(tf.CreateTerm(h, {n1,x2}), x3)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [h(n1,nX)=nY] for X=1,2,3 and Y=4,5. The clauses
    // unify and yield [].
    //EXPECT_EQ(unique_length(s), 3+1);  // after [h(n1,nX)=n4] for all nX have been added, adding any [h(n1,nX)=n5] yields []
    //const_cast<limbo::Setup&>(s).Minimize();
    EXPECT_EQ(unique_length(s), 1);
    EXPECT_FALSE(s.Consistent());
  }

  {
    Grounder g(&sf, &tf);
    g.AddClause(Clause({Literal::Neq(tf.CreateTerm(h, {n1,x2}), x3)}));
    const limbo::Setup& s = g.setup();
    // Grounding should be [h(n1,nX)=nY] for X=1,2,3 and Y=4,5.
    //EXPECT_EQ(unique_length(s), 3*2);
    //const_cast<limbo::Setup&>(s).Minimize();
    EXPECT_EQ(unique_length(s), 3*2);
    EXPECT_TRUE(s.Consistent());
  }

//  {
//    Grounder g(&sf, &tf);
//    g.AddClause(Clause({Literal::Neq(c1, x1)}));
//    g.PrepareForQuery(Formula::Factory::Exists(x1, *Formula::Factory::Atomic({Literal::Eq(x1, x1)}))->NF(&sf, &tf));
//    g.PrepareForQuery(Formula::Factory::Exists(x2, *Formula::Factory::Atomic({Literal::Eq(x2, x2)}))->NF(&sf, &tf));
//    g.PrepareForQuery(Formula::Factory::Exists(x3, *Formula::Factory::Atomic({Literal::Eq(x3, x3)}))->NF(&sf, &tf));
//    g.PrepareForQuery(Formula::Factory::Exists(x4, *Formula::Factory::Atomic({Literal::Eq(x4, x4)}))->NF(&sf, &tf));
//    const limbo::Setup& s = g.setup();
//    EXPECT_EQ(unique_length(s), 2);
//  }

  {
    //auto phi = Formula::Factory::Exists(x3, Formula::Factory::Atomic({Literal::Eq(tf.CreateTerm(h, {n1,x3}), tf.CreateTerm(a, {}))}));
    auto phi = Formula::Factory::Exists(x3, Formula::Factory::Atomic({Literal::Eq(tf.CreateTerm(h, {n1,x3}), tf.CreateTerm(g, {tf.CreateTerm(a, {})}))}))->NF(&sf, &tf);
    // NF introduces two new variables of the same sort as x3, and one new of the same sort as n1.
    Grounder gg(&sf, &tf);
    gg.PrepareForQuery(*phi);
    Grounder::TermSet terms = gg.SplitTerms();
    Grounder::SortedTermSet names = gg.Names();
    //std::cout << phi << std::endl;
    //std::cout << names << std::endl;
    //std::cout << terms << std::endl;
    EXPECT_NE(x3.sort(), n1.sort());
    EXPECT_NE(x3.sort(), a.sort());
    EXPECT_EQ(names.n_keys(), 2);
    EXPECT_EQ(n1.symbol().sort(), a.sort());
    EXPECT_EQ(x3.symbol().sort(), g.sort());
    EXPECT_EQ(x3.symbol().sort(), h.sort());
    EXPECT_EQ(names[n1.symbol().sort()].size(), 1+1+1);
    EXPECT_EQ(names[x3.symbol().sort()].size(), 0+2+1);
    Term na_1 = *names[a.sort()].begin();
    Term na_2 = *std::next(names[a.sort()].begin());
    Term na_3 = *std::next(std::next(names[a.sort()].begin()));
    EXPECT_TRUE(na_1 == n1 || na_2 == n1 || na_3 == n1);
    Term nx3_1 = *names[x3.sort()].begin();
    Term nx3_2 = *std::next(names[x3.sort()].begin());
    Term nx3_3 = *std::next(std::next(names[x3.sort()].begin()));
    EXPECT_TRUE(nx3_1 != nx3_2 && nx3_2 != nx3_3 && nx3_1 != nx3_3);
    EXPECT_EQ(std::set<Term>(terms.begin(), terms.end()),
              std::set<Term>({tf.CreateTerm(a, {}),
                             tf.CreateTerm(g, {n1}),
                             tf.CreateTerm(g, {na_1}),
                             tf.CreateTerm(g, {na_2}),
                             tf.CreateTerm(g, {na_3}),
                             tf.CreateTerm(h, {n1, nx3_1}),
                             tf.CreateTerm(h, {n1, nx3_2}),
                             tf.CreateTerm(h, {n1, nx3_3})}));
  }

  {
    Clause c{Literal::Eq(tf.CreateTerm(h, {n1,n3}), n3)}; 
    Clause d{Literal::Eq(tf.CreateTerm(h, {x1,n3}), n3)}; 
    Clause e{Literal::Eq(tf.CreateTerm(f, {x1}), n1)}; 
    auto phi = Formula::Factory::Exists(x3, Formula::Factory::Atomic({Literal::Eq(tf.CreateTerm(h, {n1,x3}), x3)}))->NF(&sf, &tf);
    Grounder g(&sf, &tf);
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.setup();
      EXPECT_EQ(unique_length(*s), 0);
      EXPECT_TRUE(bool(g.setup_));
    }
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      EXPECT_EQ(unique_length(*s), 0);
      EXPECT_TRUE(bool(g.setup_));
    }
    g.AddClause(c);  // adds new name, re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 0);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      EXPECT_EQ(unique_length(*s), 1);
      EXPECT_TRUE(bool(g.setup_));
    }
    g.PrepareForQuery(0, *phi);  // adds new plus name, re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      EXPECT_EQ(unique_length(*s), 1);
      EXPECT_TRUE(bool(g.setup_));
    }
    g.AddClause(d);  // adds two new plus names (one for x, one for the Lemma 8 fix), re-ground everything
    {
      EXPECT_TRUE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 1);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      EXPECT_EQ(unique_length(*s), 3);
      EXPECT_TRUE(bool(g.setup_));
    }
    g.PrepareForQuery(1, *phi);  // adds no new plus name, 
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      EXPECT_EQ(unique_length(*s), 3);
      EXPECT_TRUE(bool(g.setup_));
    }
    g.AddClause(e);  // adds no new names
    {
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 1);
      EXPECT_EQ(g.processed_clauses_.size(), 2);
      const class Setup* s = &g.setup();
      EXPECT_FALSE(g.names_changed_);
      EXPECT_EQ(g.unprocessed_clauses_.size(), 0);
      EXPECT_EQ(g.processed_clauses_.size(), 3);
      EXPECT_EQ(unique_length(*s), 3+3);
      EXPECT_TRUE(bool(g.setup_));
    }
  }
}

TEST(GrounderTest, Assignments) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort s1 = sf.CreateSort();                  RegisterSort(s1, "");
  const Symbol::Sort s2 = sf.CreateSort();                  RegisterSort(s2, "");
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));         RegisterSymbol(n1.symbol(), "n1");
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));         RegisterSymbol(n2.symbol(), "n2");
  const Term n3 = tf.CreateTerm(sf.CreateName(s2));         RegisterSymbol(n3.symbol(), "n3");
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x1.symbol(), "x1");
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));     RegisterSymbol(x2.symbol(), "x2");
  const Term x3 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x3.symbol(), "x3");
  //const Term x4 = tf.CreateTerm(sf.CreateVariable(s2));     RegisterSymbol(x4, "x4");
  //const Symbol a = sf.CreateFunction(s1, 0);                RegisterSymbol(a, "a");
  const Symbol f = sf.CreateFunction(s1, 1);                RegisterSymbol(f, "f");
  //const Symbol g = sf.CreateFunction(s2, 1);                RegisterSymbol(g, "g");
  //const Symbol h = sf.CreateFunction(s2, 2);                RegisterSymbol(h, "h");
  {
    Grounder::SortedTermSet ts;
    ts.insert(n1);
    Grounder::Assignments as({}, &ts);
    EXPECT_EQ(length(as), 1);
    Term fx1 = tf.CreateTerm(f, {x1});
    Term fn1 = tf.CreateTerm(f, {n1});
    auto it = as.begin();
    auto a = *it;
    EXPECT_EQ(fx1.Substitute(a, &tf), fx1);
    EXPECT_NE(fx1.Substitute(a, &tf), fn1);
  }
  {
    Grounder::SortedTermSet ts;
    ts.insert(n1);
    Grounder::Assignments as({x1}, &ts);
    EXPECT_EQ(length(as), 1);
    Term fx1 = tf.CreateTerm(f, {x1});
    Term fn1 = tf.CreateTerm(f, {n1});
    auto it = as.begin();
    auto a = *it;
    EXPECT_NE(fx1.Substitute(a, &tf), fx1);
    EXPECT_EQ(fx1.Substitute(a, &tf), fn1);
  }
  {
    Grounder::SortedTermSet ts;
    ts.insert(n1);
    ts.insert(n2);
    Grounder::Assignments as({x1}, &ts);
    EXPECT_EQ(length(as), 2);
    Term fx1 = tf.CreateTerm(f, {x1});
    Term fn1 = tf.CreateTerm(f, {n1});
    Term fn2 = tf.CreateTerm(f, {n2});
    Grounder::TermSet substitutes;
    auto it = as.begin();
    auto a = *it;
    substitutes.insert(fx1.Substitute(a, &tf));
    auto jt = ++it;
    auto b = *jt;
    substitutes.insert(fx1.Substitute(b, &tf));
    EXPECT_EQ(substitutes.size(), 2);
    EXPECT_EQ(substitutes, Grounder::TermSet({fn1, fn2}));
    EXPECT_TRUE(substitutes.find(fx1) == substitutes.end());
  }
  {
    Grounder::SortedTermSet ts;
    ts.insert(n1);
    ts.insert(n2);
    ts.insert(n3);
    Grounder::Assignments as({x1,x2,x3}, &ts);
    EXPECT_EQ(length(as), 4);
  }
}

TEST(GrounderTest, Ground_SplitNames) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort Bool = sf.CreateSort();                            RegisterSort(Bool, "");
  const Symbol::Sort Human = sf.CreateSort();                           RegisterSort(Human, "");
  const Symbol::Sort Animal = sf.CreateSort();                          RegisterSort(Animal, "");
  //
  const Term T          = tf.CreateTerm(sf.CreateName(Bool));           RegisterSymbol(T.symbol(), "T");
  //const Term F          = tf.CreateTerm(sf.CreateName(Bool));
  //
  const Symbol IsHuman  = sf.CreateFunction(Bool, 1);                   RegisterSymbol(IsHuman, "IsHuman");
  const Term x          = tf.CreateTerm(sf.CreateVariable(Human));      RegisterSymbol(x.symbol(), "x");
  const Term xIsHuman   = tf.CreateTerm(IsHuman, {x});
  //
  const Symbol IsAnimal = sf.CreateFunction(Bool, 1);                   RegisterSymbol(IsAnimal, "IsAnimal");
  const Term a          = tf.CreateTerm(sf.CreateFunction(Animal, 0));  RegisterSymbol(a.symbol(), "a");
  const Term aIsAnimal  = tf.CreateTerm(IsAnimal, {a});
  //
  auto phi = Formula::Factory::Exists(x, Formula::Factory::Atomic(Clause({ Literal::Eq(xIsHuman, T), Literal::Neq(aIsAnimal, T) })))->NF(&sf, &tf);
  {
    Grounder g(&sf, &tf);
    g.PrepareForQuery(0, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_EQ(terms.size(), 0);
  }
  {
    Grounder g(&sf, &tf);
    g.PrepareForQuery(1, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
  {
    Grounder g(&sf, &tf);
    g.PrepareForQuery(2, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
  {
    Grounder g(&sf, &tf);
    g.PrepareForQuery(3, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
}

TEST(GrounderTest, Ground_SplitNames_iterated) {
  Symbol::Factory& sf = *Symbol::Factory::Instance();
  Term::Factory& tf = *Term::Factory::Instance();
  const Symbol::Sort Bool = sf.CreateSort();                            RegisterSort(Bool, "");
  const Symbol::Sort Human = sf.CreateSort();                           RegisterSort(Human, "");
  const Symbol::Sort Animal = sf.CreateSort();                          RegisterSort(Animal, "");
  //
  const Term T          = tf.CreateTerm(sf.CreateName(Bool));           RegisterSymbol(T.symbol(), "T");
  //const Term F          = tf.CreateTerm(sf.CreateName(Bool));
  //
  const Symbol IsHuman  = sf.CreateFunction(Bool, 1);                   RegisterSymbol(IsHuman, "IsHuman");
  const Term x          = tf.CreateTerm(sf.CreateVariable(Human));      RegisterSymbol(x.symbol(), "x");
  const Term xIsHuman   = tf.CreateTerm(IsHuman, {x});
  //
  const Symbol IsAnimal = sf.CreateFunction(Bool, 1);                   RegisterSymbol(IsAnimal, "IsAnimal");
  const Term a          = tf.CreateTerm(sf.CreateFunction(Animal, 0));  RegisterSymbol(a.symbol(), "a");
  const Term aIsAnimal  = tf.CreateTerm(IsAnimal, {a});
  //
  auto phi = Formula::Factory::Exists(x, Formula::Factory::Atomic(Clause({ Literal::Eq(xIsHuman, T), Literal::Neq(aIsAnimal, T) })))->NF(&sf, &tf);
  // same as previous test except that we re-use the grounder
  Grounder g(&sf, &tf);
  {
    g.PrepareForQuery(0, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_EQ(terms.size(), 0);
  }
  {
    g.PrepareForQuery(1, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
  {
    g.PrepareForQuery(2, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
  {
    g.PrepareForQuery(3, *phi);
    Grounder::SortedTermSet names = g.Names();
    EXPECT_EQ(names[Bool].size(), 1+1);
    EXPECT_EQ(names[Human].size(), 1+1);
    EXPECT_EQ(names[Animal].size(), 0+2);
    Grounder::TermSet terms = g.SplitTerms();
    EXPECT_NE(terms.size(), 0);
  }
}
#endif

}  // namespace limbo

