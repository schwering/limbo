// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./formula.h"
#include "./print.h"

using namespace lela;

TEST(formula, formula) {
  Symbol::Factory sf;
  Term::Factory tf;
  const Symbol::Sort s1 = sf.CreateSort();
  const Symbol::Sort s2 = sf.CreateSort();
  const Term n1 = tf.CreateTerm(sf.CreateName(s1));
  const Term n2 = tf.CreateTerm(sf.CreateName(s1));
  const Term x2 = tf.CreateTerm(sf.CreateVariable(s1));
  const Symbol f = sf.CreateFunction(s1, 1);
  const Symbol h = sf.CreateFunction(s2, 2);
  const Term f1 = tf.CreateTerm(f, {n1});
  const Term f2 = tf.CreateTerm(h, {n1,x2});

  Clause c1({Literal::Eq(f1,n1)});
  Clause c2({Literal::Neq(f2,n2)});

  Formula phi = Formula::Or(Formula::Clause(c1), Formula::Not(Formula::Clause(c2)));
  std::cout << phi << std::endl;
}

