// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>

#include <iostream>

#include <limbo/formula.h>

#include <limbo/format/output.h>

namespace limbo {

using namespace limbo::format;

using Abc = Alphabet;
using F = Formula;

TEST(FormulaTest, Rectify) {
  Alphabet* abc = Alphabet::Instance();
  Abc::Sort s = abc->CreateSort(false);
  Abc::VarSymbol x = abc->CreateVar(s);         LIMBO_REG(x);
  Abc::VarSymbol y = abc->CreateVar(s);         LIMBO_REG(y);
  Abc::VarSymbol z = abc->CreateVar(s);         LIMBO_REG(z);
  Abc::VarSymbol u = abc->CreateVar(s);         LIMBO_REG(u);
  Abc::NameSymbol n = abc->CreateName(s, 0);    LIMBO_REG(n);
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
}

}  // namespace limbo

