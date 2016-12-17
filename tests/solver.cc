// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/solver.h>
#include <lela/format/output.h>
#include <lela/format/cpp/syntax.h>

namespace lela {

using namespace lela::format::cpp;
using namespace lela::format::output;

#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

TEST(SolverTest, Entails) {
  {
    Context ctx;
    Solver& solver = *ctx.solver();
    auto Bool = ctx.CreateSort();                RegisterSort(Bool, "");
    auto True = ctx.CreateName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.CreateSort();               RegisterSort(Human, "");
    auto Sonny = ctx.CreateName(Human);          REGISTER_SYMBOL(Sonny);
    auto Mary = ctx.CreateName(Human);           REGISTER_SYMBOL(Mary);
    auto Frank = ctx.CreateName(Human);          REGISTER_SYMBOL(Frank);
    auto Father = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.CreateFunction(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.CreateVariable(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.CreateVariable(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Mother(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Mother(Sonny) == Mary });
      Formula::Ref phi = Ex(x, Ex(y, IsParentOf(y,x) == True))->NF(ctx.sf(), ctx.tf());
      EXPECT_TRUE(solver.Entails(0, *phi));
      EXPECT_TRUE(solver.Entails(1, *phi));
      EXPECT_TRUE(solver.Entails(0, *phi));
      EXPECT_TRUE(solver.Entails(1, *phi));
    }
  }

  {
    Context ctx;
    Solver& solver = *ctx.solver();
    auto Bool = ctx.CreateSort();                RegisterSort(Bool, "");
    auto True = ctx.CreateName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.CreateSort();               RegisterSort(Human, "");
    auto Sonny = ctx.CreateName(Human);          REGISTER_SYMBOL(Sonny);
    auto Mary = ctx.CreateName(Human);           REGISTER_SYMBOL(Mary);
    auto Frank = ctx.CreateName(Human);          REGISTER_SYMBOL(Frank);
    auto Fred = ctx.CreateName(Human);           REGISTER_SYMBOL(Fred);
    auto Father = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.CreateFunction(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.CreateVariable(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.CreateVariable(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Father(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Father(Sonny) == Mary, Father(Sonny) == Fred });
      Formula::Ref phi = Ex(x, Ex(y, IsParentOf(y,x) == True))->NF(ctx.sf(), ctx.tf());
      EXPECT_FALSE(solver.Entails(0, *phi));
      EXPECT_TRUE(solver.Entails(1, *phi));
      EXPECT_FALSE(solver.Entails(0, *phi));
      EXPECT_TRUE(solver.Entails(1, *phi));
    }
  }

  {
    Context ctx;
    Solver& solver = *ctx.solver();
    auto Bool = ctx.CreateSort();                RegisterSort(Bool, "");
    auto True = ctx.CreateName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.CreateSort();               RegisterSort(Human, "");
    auto Sonny = ctx.CreateName(Human);          REGISTER_SYMBOL(Sonny);
    auto Mary = ctx.CreateName(Human);           REGISTER_SYMBOL(Mary);
    auto Frank = ctx.CreateName(Human);          REGISTER_SYMBOL(Frank);
    auto Fred = ctx.CreateName(Human);           REGISTER_SYMBOL(Fred);
    auto Fox = ctx.CreateName(Human);            REGISTER_SYMBOL(Fox);
    auto Father = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.CreateFunction(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.CreateVariable(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.CreateVariable(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Father(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Father(Sonny) == Mary, Father(Sonny) == Fred, Father(Sonny) == Fox });
      Formula::Ref phi = Ex(x, Ex(y, IsParentOf(y,x) == True))->NF(ctx.sf(), ctx.tf());
      EXPECT_FALSE(solver.Entails(0, *phi));
      EXPECT_TRUE(solver.Entails(1, *phi));
    }
  }
}

TEST(SolverTest, Consistent) {
  {
    Context ctx;
    Solver& solver = *ctx.solver();
    auto Bool = ctx.CreateSort();                RegisterSort(Bool, "");
    auto True = ctx.CreateName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.CreateSort();               RegisterSort(Human, "");
    auto Sonny = ctx.CreateName(Human);          REGISTER_SYMBOL(Sonny);
    auto Mary = ctx.CreateName(Human);           REGISTER_SYMBOL(Mary);
    auto Frank = ctx.CreateName(Human);          REGISTER_SYMBOL(Frank);
    auto Father = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.CreateFunction(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.CreateFunction(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.CreateVariable(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.CreateVariable(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Mother(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Mother(Sonny) == Mary });
      Formula::Ref phi = Ex(x, Ex(y, IsParentOf(y,x) == True))->NF(ctx.sf(), ctx.tf());
      EXPECT_TRUE(solver.EntailsComplete(0, *phi));
      EXPECT_TRUE(solver.EntailsComplete(1, *phi));
      EXPECT_TRUE(solver.EntailsComplete(0, *phi));
      EXPECT_TRUE(solver.EntailsComplete(1, *phi));
    }
  }
}

TEST(SolverTest, KR2016) {
  Context ctx;
  Solver& solver = *ctx.solver();
  auto Human = ctx.CreateSort();             RegisterSort(Human, "");
  auto sue = ctx.CreateName(Human);          REGISTER_SYMBOL(sue);
  auto jane = ctx.CreateName(Human);         REGISTER_SYMBOL(jane);
  auto mary = ctx.CreateName(Human);         REGISTER_SYMBOL(mary);
  auto george = ctx.CreateName(Human);       REGISTER_SYMBOL(george);
  auto father = ctx.CreateFunction(Human, 1);     REGISTER_SYMBOL(father);
  auto bestFriend = ctx.CreateFunction(Human, 1); REGISTER_SYMBOL(bestFriend);
  solver.AddClause({ bestFriend(mary) == sue, bestFriend(mary) == jane });
  solver.AddClause({ father(sue) == george });
  solver.AddClause({ father(jane) == george });
  // Since our normal form converts
  //   father(bestFriend(mary)) == george
  // to
  //   Fa x (x != bestFriend(mary) || father(x) == george)
  // the query already comes out true at belief level 0.
  //std::cout << *Formula::Atomic(Clause{father(bestFriend(mary)) == george}) << std::endl;
  //std::cout << *Formula::Atomic(Clause{father(bestFriend(mary)) == george})->NF(ctx.sf(), ctx.tf()) << std::endl;
  //EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{father(bestFriend(mary)) == george})->NF(ctx.sf(), ctx.tf())));
  EXPECT_TRUE(solver.Entails(0, *Formula::Atomic(Clause{father(bestFriend(mary)) == george})->NF(ctx.sf(), ctx.tf())));
  EXPECT_TRUE(solver.Entails(1, *Formula::Atomic(Clause{father(bestFriend(mary)) == george})->NF(ctx.sf(), ctx.tf())));
}

TEST(SolverTest, ECAI2016Sound) {
  Context ctx;
  Solver& solver = *ctx.solver();
  auto Bool = ctx.CreateSort();              RegisterSort(Bool, "");
  auto Food = ctx.CreateSort();              RegisterSort(Food, "");
  auto T = ctx.CreateName(Bool);             REGISTER_SYMBOL(T);
  //auto F = ctx.CreateName(Bool);             REGISTER_SYMBOL(F);
  auto Aussie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.CreateFunction(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.CreateName(Food);           REGISTER_SYMBOL(roo);
  auto x = ctx.CreateVariable(Food);              REGISTER_SYMBOL(x);
  solver.AddClause({ Meat(roo) == T });
  solver.AddClause({ Meat(x) != T, Eats(x) != T, Veggie != T });
  solver.AddClause({ Aussie != T, Italian != T });
  solver.AddClause({ Aussie == T, Italian == T });
  solver.AddClause({ Aussie != T, Eats(roo) == T });
  solver.AddClause({ Italian == T, Veggie == T });
  EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{Aussie != T})->NF(ctx.sf(), ctx.tf())));
  EXPECT_TRUE(solver.Entails(1, *Formula::Atomic(Clause{Aussie != T})->NF(ctx.sf(), ctx.tf())));
}

TEST(SolverTest, ECAI2016Complete) {
  Context ctx;
  Solver& solver = *ctx.solver();
  auto Bool = ctx.CreateSort();              RegisterSort(Bool, "");
  auto Food = ctx.CreateSort();              RegisterSort(Food, "");
  auto T = ctx.CreateName(Bool);             REGISTER_SYMBOL(T);
  //auto F = ctx.CreateName(Bool);             REGISTER_SYMBOL(F);
  auto Aussie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.CreateFunction(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.CreateName(Food);           REGISTER_SYMBOL(roo);
  auto x = ctx.CreateVariable(Food);              REGISTER_SYMBOL(x);
  solver.AddClause({ Meat(roo) == T });
  solver.AddClause({ Meat(x) != T, Eats(x) != T, Veggie != T });
  solver.AddClause({ Aussie != T, Italian != T });
  solver.AddClause({ Aussie == T, Italian == T });
  solver.AddClause({ Aussie != T, Eats(roo) == T });
  solver.AddClause({ Italian == T, Veggie == T });
  EXPECT_TRUE(solver.EntailsComplete(0, *Formula::Atomic(Clause{Italian != T})->NF(ctx.sf(), ctx.tf())));
  EXPECT_FALSE(solver.EntailsComplete(1, *Formula::Atomic(Clause{Italian != T})->NF(ctx.sf(), ctx.tf())));
  EXPECT_FALSE(solver.Consistent(0, *Formula::Atomic(Clause{Italian == T})->NF(ctx.sf(), ctx.tf())));
  EXPECT_TRUE(solver.Consistent(1, *Formula::Atomic(Clause{Italian == T})->NF(ctx.sf(), ctx.tf())));
}

TEST(SolverTest, Bool) {
  Context ctx;
  Solver& solver = *ctx.solver();
  auto BOOL = ctx.sf()->CreateSort();
  auto T = ctx.CreateName(BOOL);
  auto P = ctx.CreateFunction(BOOL, 0)();
  {
    EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{P == T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(1, *Formula::Atomic(Clause{P == T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{P != T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(1, *Formula::Atomic(Clause{P != T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{P == T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(1, *Formula::Atomic(Clause{P == T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(0, *Formula::Atomic(Clause{P != T})->NF(ctx.sf(), ctx.tf())));
    EXPECT_FALSE(solver.Entails(1, *Formula::Atomic(Clause{P != T})->NF(ctx.sf(), ctx.tf())));
  }
}

TEST(SolverTest, Constants) {
  UnregisterAll();
  Context ctx;
  Solver& solver = *ctx.solver();
  auto SomeSort = ctx.sf()->CreateSort();  RegisterSort(SomeSort, "");
  auto a = ctx.CreateFunction(SomeSort, 0)(); REGISTER_SYMBOL(a);
  auto b = ctx.CreateFunction(SomeSort, 0)(); REGISTER_SYMBOL(b);
  {
    for (int i = 0; i < 2; ++i) {
      for (int k = 0; k <= 3; ++k) {
        EXPECT_FALSE(solver.Entails(k, *Formula::Atomic(Clause{a == b})->NF(ctx.sf(), ctx.tf())));
        EXPECT_FALSE(solver.Entails(k, *Formula::Atomic(Clause{a != b})->NF(ctx.sf(), ctx.tf())));
      }
    }
  }
}

}  // namespace lela

