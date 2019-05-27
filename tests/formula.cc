// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>

#include <iostream>

#include <limbo/formula.h>
#include <limbo/io/output.h>

namespace limbo {

using Abc = Alphabet;
using F = Formula;

TEST(FormulaTest, Satisfies) {
  std::vector<Fun> f;
  std::vector<Name> n;
  f.push_back(Fun());
  n.push_back(Name());
  for (int i = 1; i <= 10; ++i) {
    f.push_back(Fun::FromId(i));
    n.push_back(Name::FromId(i));
  }
  TermMap<Fun, Name> model;
  for (int i = 1; i <= 10; ++i) {
    model.FitForKey(f[i]);
    model[f[i]] = n[i];
  }
  auto eq = [&f, &n](int i, int j) { return Lit::Eq(f[i], n[j]); };
  auto neq = [&f, &n](int i, int j) { return Lit::Neq(f[i], n[j]); };
  auto feq = [&eq](int i, int j) { return Formula::Lit(eq(i, j)); };
  auto fneq = [&neq](int i, int j) { return Formula::Lit(neq(i, j)); };

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(RFormula().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(feq(1, 1).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(1, 1)});
  }

  // variantsion of one disjunction
  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(feq(1, 1), feq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(feq(1, 1), fneq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(fneq(1, 1), feq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(2, 2)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::Or(fneq(1, 1), fneq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  // variantsion of one conjunction
  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::And(feq(1, 1), feq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1), eq(2, 2)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(feq(1, 1), fneq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(fneq(1, 1), feq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(fneq(1, 1), fneq(2, 2)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  // variations of two disjunctions
  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(feq(1, 1), Formula::Or(feq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(1, 1)});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::Or(feq(1, 1), feq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(1, 1)});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(fneq(1, 1), Formula::Or(feq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(2, 2)});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::Or(fneq(1, 1), feq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(2, 2)});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(fneq(1, 1), Formula::Or(fneq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(3, 3)});
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::Or(fneq(1, 1), fneq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>{eq(3, 3)});
  }

  // variations of two conjunctions
  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::And(feq(1, 1), Formula::And(feq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1), eq(2, 2), eq(3, 3)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::And(Formula::And(feq(1, 1), feq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1), eq(2, 2), eq(3, 3)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(fneq(1, 1), Formula::And(feq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(Formula::And(fneq(1, 1), feq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(feq(1, 1), Formula::And(fneq(2, 2), feq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(Formula::And(feq(1, 1), fneq(2, 2)), feq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(feq(1, 1), Formula::And(fneq(2, 2), fneq(3, 3))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::And(Formula::And(feq(1, 1), fneq(2, 2)), fneq(3, 3)).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>());
  }

  // variations of a disjunction of two conjunctions
  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::And(feq(1, 1), feq(2, 2)),
                            Formula::And(feq(3, 3), feq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1), eq(2, 2)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::And(feq(1, 1), feq(2, 2)),
                            Formula::And(fneq(3, 3), fneq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(1, 1), eq(2, 2)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::And(feq(1, 1), fneq(2, 2)),
                            Formula::And(feq(3, 3), feq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(3, 3), eq(4, 4)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_TRUE(Formula::Or(Formula::And(fneq(1, 1), feq(2, 2)),
                            Formula::And(feq(3, 3), feq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({eq(3, 3), eq(4, 4)}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::Or(Formula::And(feq(1, 1), fneq(2, 2)),
                             Formula::And(feq(3, 3), fneq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::Or(Formula::And(fneq(1, 1), feq(2, 2)),
                             Formula::And(feq(3, 3), fneq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({}));
  }

  {
    std::vector<Lit> reason;
    EXPECT_FALSE(Formula::Or(Formula::And(feq(1, 1), fneq(2, 2)),
                             Formula::And(fneq(3, 3), feq(4, 4))).readable().SatisfiedBy(model, &reason));
    EXPECT_EQ(reason, std::vector<Lit>({}));
  }
}

TEST(FormulaTest, Rectify) {
  Alphabet* abc = Alphabet::Instance();
  Abc::Sort s = abc->CreateSort(false);
  Abc::VarSymbol x = abc->CreateVar(s);         LIMBO_REG(x);
  Abc::VarSymbol y = abc->CreateVar(s);         LIMBO_REG(y);
  Abc::VarSymbol z = abc->CreateVar(s);         LIMBO_REG(z);
  Abc::VarSymbol u = abc->CreateVar(s);         LIMBO_REG(u);
  Abc::NameSymbol m = abc->CreateName(s, 0);    LIMBO_REG(m);
  Abc::NameSymbol n = abc->CreateName(s, 0);    LIMBO_REG(n);
  Abc::NameSymbol o = abc->CreateName(s, 0);    LIMBO_REG(o);
  Abc::FunSymbol c = abc->CreateFun(s, 0);      LIMBO_REG(c);
  Abc::FunSymbol f = abc->CreateFun(s, 2);      LIMBO_REG(f);
  Abc::FunSymbol g = abc->CreateFun(s, 1);      LIMBO_REG(g);

  auto arg = [](Formula&& f1) { std::list<F> args; args.emplace_back(std::move(f1)); return args; };
  auto args = [](Formula&& f1, Formula&& f2) { std::list<F> args; args.emplace_back(std::move(f1)); args.emplace_back(std::move(f2)); return args; };
  F fxy = F::Fun(f, args(F::Var(x), F::Var(y)));
  F fyz = F::Fun(f, args(F::Var(y), F::Var(z)));
  F gfxy = F::Fun(g, arg(fxy.Clone()));
  F gfyz = F::Fun(g, arg(fyz.Clone()));
  F w = F::Exists(x, F::Or(F::Forall(y, F::Exists(z, F::Equals(fxy.Clone(), fyz.Clone()))),
                           F::Exists(x, F::Forall(y, F::Exists(z, F::Exists(u, F::Equals(gfxy.Clone(), gfyz.Clone())))))));

  {
    std::cout << "" << std::endl;
    F phi(F::Know(0, F::Exists(x, F::Equals(F::Fun(c, std::list<F>{}), F::Name(n, std::list<F>{})))));
    std::cout << "Orig: " << phi << std::endl;
    phi.Rectify();
    std::cout << "Rect: " << phi << std::endl;
    phi.Skolemize();
    std::cout << "Skol: " << phi << std::endl;
    phi.PushInwards();
    std::cout << "Push: " << phi << std::endl;
    phi.Strip();
    std::cout << "Strp: " << phi << std::endl;
  }

  {
    std::cout << "" << std::endl;
    F phi(w.Clone());
    std::cout << "Orig: " << phi << std::endl;
    phi.Rectify();
    std::cout << "Rect: " << phi << std::endl;
    phi.Flatten();
    std::cout << "Flat: " << phi << std::endl;
    phi.PushInwards();
    std::cout << "Push: " << phi << std::endl;
    phi.Strip();
    std::cout << "Strp: " << phi << std::endl;
  }

  {
    Abc::DenseMap<Abc::Sort, std::vector<Name>> subst;
    subst[s] = {Name::FromId(1), Name::FromId(2), Name::FromId(3)};
    std::cout << "" << std::endl;
    //F phi(F::And(F::Exists(x, F::Equals(F::Var(x), F::Var(x))), F::Exists(x, F::Equals(F::Var(x), F::Var(x)))));
    F phi(F::Exists(x, F::Equals(F::Var(x), F::Var(x))));
    std::cout << "Orig: " << phi << std::endl;
    phi.Ground(subst);
    std::cout << "Grou: " << phi << std::endl;
  }

  {
    Abc::DenseMap<Abc::Sort, std::vector<Name>> subst;
    subst[s] = {Name::FromId(1), Name::FromId(2), Name::FromId(3)};
    std::cout << "" << std::endl;
    F phi(F::And(F::Forall(x, F::Equals(F::Var(x), F::Var(x))), F::Exists(x, F::Equals(F::Var(x), F::Var(x)))));
    std::cout << "Orig: " << phi << std::endl;
    phi.Ground(subst);
    std::cout << "Grou: " << phi << std::endl;
  }
}

}  // namespace limbo

