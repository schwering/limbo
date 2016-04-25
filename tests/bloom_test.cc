// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./term.h"
#include "./print.h"

using namespace lela;

TEST(bloom_test, symbol) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x1 = tf.CreateTerm(sf.CreateVariable(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Symbol f = sf.CreateFunction(s1, 1);
  const Symbol g = sf.CreateFunction(s2, 1);
  const Symbol h = sf.CreateFunction(s2, 2);
  const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});
  const Term f3 = tf.CreateTerm(g, {f1});
  const Term f4 = tf.CreateTerm(h, {n1,f1});
  const std::vector<Term> ts({n1,n2,x1,x2,f1,f2,f3});

  BloomFilter bf0;
  BloomFilter bf1;

  for (Term t : ts) {
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf1));
    EXPECT_FALSE(bf1.Contains(t.hash()));
  }
  for (Term t : ts) {
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf1));
    EXPECT_FALSE(bf1.Contains(t.hash()));
    bf1.Add(t.hash());
    EXPECT_TRUE(bf1.Contains(t.hash()));
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf1));
  }

  for (Term t : ts) {
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf0));
    EXPECT_FALSE(bf0.Contains(t.hash()));
  }
  for (Term t : ts) {
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf0));
    EXPECT_FALSE(bf0.Contains(t.hash()));
    bf0.Add(t.hash());
    EXPECT_TRUE(bf0.Contains(t.hash()));
    EXPECT_TRUE(BloomFilter::Subset(bf0, bf0));
  }

  bf0.Add(f4.hash());
  EXPECT_TRUE(bf0.Contains(f4.hash()));
  EXPECT_FALSE(bf1.Contains(f4.hash()));
  EXPECT_FALSE(BloomFilter::Subset(bf0, bf1));

  bf0.Clear();
  EXPECT_TRUE(BloomFilter::Subset(bf0, bf1));
  EXPECT_FALSE(BloomFilter::Subset(bf1, bf0));
}

