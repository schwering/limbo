// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/solver.h>
#include <lela/format/syntax.h>
#include <lela/format/output.h>

namespace lela {

using namespace lela::format;

#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

TEST(SolverTest, Entails) {
  {
    Solver solver;
    Context ctx(solver.sf(), solver.tf());
    auto Bool = ctx.NewSort();                RegisterSort(Bool, "");
    auto True = ctx.NewName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.NewSort();               RegisterSort(Human, "");
    auto Jesus = ctx.NewName(Human);          REGISTER_SYMBOL(Jesus);
    auto Mary = ctx.NewName(Human);           REGISTER_SYMBOL(Mary);
    auto Joe = ctx.NewName(Human);            REGISTER_SYMBOL(Joe);
    auto Father = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.NewFun(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.NewVar(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.NewVar(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Mother(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Mother(Jesus) == Mary });
      Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
      EXPECT_TRUE(solver.Entails(0, phi.reader()));
      EXPECT_TRUE(solver.Entails(1, phi.reader()));
      EXPECT_TRUE(solver.Entails(0, phi.reader()));
      EXPECT_TRUE(solver.Entails(1, phi.reader()));
    }
  }

  {
    Solver solver;
    Context ctx(solver.sf(), solver.tf());
    auto Bool = ctx.NewSort();                RegisterSort(Bool, "");
    auto True = ctx.NewName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.NewSort();               RegisterSort(Human, "");
    auto Jesus = ctx.NewName(Human);          REGISTER_SYMBOL(Jesus);
    auto Mary = ctx.NewName(Human);           REGISTER_SYMBOL(Mary);
    auto Joe = ctx.NewName(Human);            REGISTER_SYMBOL(Joe);
    auto God = ctx.NewName(Human);            REGISTER_SYMBOL(God);
    auto Father = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.NewFun(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.NewVar(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.NewVar(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Father(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Father(Jesus) == Mary, Father(Jesus) == God });
      Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
      EXPECT_FALSE(solver.Entails(0, phi.reader()));
      EXPECT_TRUE(solver.Entails(1, phi.reader()));
      EXPECT_FALSE(solver.Entails(0, phi.reader()));
      EXPECT_TRUE(solver.Entails(1, phi.reader()));
    }
  }

  {
    Solver solver;
    Context ctx(solver.sf(), solver.tf());
    auto Bool = ctx.NewSort();                RegisterSort(Bool, "");
    auto True = ctx.NewName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.NewSort();               RegisterSort(Human, "");
    auto Jesus = ctx.NewName(Human);          REGISTER_SYMBOL(Jesus);
    auto Mary = ctx.NewName(Human);           REGISTER_SYMBOL(Mary);
    auto Joe = ctx.NewName(Human);            REGISTER_SYMBOL(Joe);
    auto God = ctx.NewName(Human);            REGISTER_SYMBOL(God);
    auto HolyGhost = ctx.NewName(Human);      REGISTER_SYMBOL(HolyGhost);
    auto Father = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.NewFun(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.NewVar(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.NewVar(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Father(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Father(Jesus) == Mary, Father(Jesus) == God, Father(Jesus) == HolyGhost });
      Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
      EXPECT_FALSE(solver.Entails(0, phi.reader()));
      EXPECT_TRUE(solver.Entails(1, phi.reader()));
    }
  }
}

TEST(SolverTest, Consistent) {
  {
    Solver solver;
    Context ctx(solver.sf(), solver.tf());
    auto Bool = ctx.NewSort();                RegisterSort(Bool, "");
    auto True = ctx.NewName(Bool);            REGISTER_SYMBOL(True);
    auto Human = ctx.NewSort();               RegisterSort(Human, "");
    auto Jesus = ctx.NewName(Human);          REGISTER_SYMBOL(Jesus);
    auto Mary = ctx.NewName(Human);           REGISTER_SYMBOL(Mary);
    auto Joe = ctx.NewName(Human);            REGISTER_SYMBOL(Joe);
    auto Father = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Father);
    auto Mother = ctx.NewFun(Human, 1);       REGISTER_SYMBOL(Mother);
    auto IsParentOf = ctx.NewFun(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
    auto x = ctx.NewVar(Human);               REGISTER_SYMBOL(x);
    auto y = ctx.NewVar(Human);               REGISTER_SYMBOL(y);
    {
      solver.AddClause(Clause{ Mother(x) != y, x == y, IsParentOf(y,x) == True });
      solver.AddClause(Clause{ Mother(Jesus) == Mary });
      Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
      EXPECT_TRUE(solver.EntailsComplete(0, phi.reader()));
      EXPECT_TRUE(solver.EntailsComplete(1, phi.reader()));
      EXPECT_TRUE(solver.EntailsComplete(0, phi.reader()));
      EXPECT_TRUE(solver.EntailsComplete(1, phi.reader()));
    }
  }
}

TEST(SolverTest, KR2016) {
  Solver solver;
  Context ctx(solver.sf(), solver.tf());
  auto Human = ctx.NewSort();             RegisterSort(Human, "");
  auto sue = ctx.NewName(Human);          REGISTER_SYMBOL(sue);
  auto jane = ctx.NewName(Human);         REGISTER_SYMBOL(jane);
  auto mary = ctx.NewName(Human);         REGISTER_SYMBOL(mary);
  auto george = ctx.NewName(Human);       REGISTER_SYMBOL(george);
  auto father = ctx.NewFun(Human, 1);     REGISTER_SYMBOL(father);
  auto bestFriend = ctx.NewFun(Human, 1); REGISTER_SYMBOL(bestFriend);
  solver.AddClause({ bestFriend(mary) == sue, bestFriend(mary) == jane });
  solver.AddClause({ father(sue) == george });
  solver.AddClause({ father(jane) == george });
  EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{father(bestFriend(mary)) == george}).reader()));
  EXPECT_TRUE(solver.Entails(1, Formula::Clause(Clause{father(bestFriend(mary)) == george}).reader()));
}

TEST(SolverTest, ECAI2016Sound) {
  Solver solver;
  Context ctx(solver.sf(), solver.tf());
  auto Bool = ctx.NewSort();              RegisterSort(Bool, "");
  auto Food = ctx.NewSort();              RegisterSort(Food, "");
  auto T = ctx.NewName(Bool);             REGISTER_SYMBOL(T);
  //auto F = ctx.NewName(Bool);             REGISTER_SYMBOL(F);
  auto Aussie = ctx.NewFun(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.NewFun(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.NewFun(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.NewFun(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.NewFun(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.NewName(Food);           REGISTER_SYMBOL(roo);
  auto x = ctx.NewVar(Food);              REGISTER_SYMBOL(x);
  solver.AddClause({ Meat(roo) == T });
  solver.AddClause({ Meat(x) != T, Eats(x) != T, Veggie != T });
  solver.AddClause({ Aussie != T, Italian != T });
  solver.AddClause({ Aussie == T, Italian == T });
  solver.AddClause({ Aussie != T, Eats(roo) == T });
  solver.AddClause({ Italian == T, Veggie == T });
  EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{Aussie != T}).reader()));
  EXPECT_TRUE(solver.Entails(1, Formula::Clause(Clause{Aussie != T}).reader()));
}

TEST(SolverTest, ECAI2016Complete) {
  Solver solver;
  Context ctx(solver.sf(), solver.tf());
  auto Bool = ctx.NewSort();              RegisterSort(Bool, "");
  auto Food = ctx.NewSort();              RegisterSort(Food, "");
  auto T = ctx.NewName(Bool);             REGISTER_SYMBOL(T);
  //auto F = ctx.NewName(Bool);             REGISTER_SYMBOL(F);
  auto Aussie = ctx.NewFun(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.NewFun(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.NewFun(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.NewFun(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.NewFun(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.NewName(Food);           REGISTER_SYMBOL(roo);
  auto x = ctx.NewVar(Food);              REGISTER_SYMBOL(x);
  solver.AddClause({ Meat(roo) == T });
  solver.AddClause({ Meat(x) != T, Eats(x) != T, Veggie != T });
  solver.AddClause({ Aussie != T, Italian != T });
  solver.AddClause({ Aussie == T, Italian == T });
  solver.AddClause({ Aussie != T, Eats(roo) == T });
  solver.AddClause({ Italian == T, Veggie == T });
  EXPECT_TRUE(solver.EntailsComplete(0, Formula::Clause(Clause{Italian != T}).reader()));
  EXPECT_FALSE(solver.EntailsComplete(1, Formula::Clause(Clause{Italian != T}).reader()));
  EXPECT_FALSE(solver.Consistent(0, Formula::Clause(Clause{Italian == T}).reader()));
  EXPECT_TRUE(solver.Consistent(1, Formula::Clause(Clause{Italian == T}).reader()));
}

TEST(SolverTest, Bool) {
  Solver solver;
  Context ctx(solver.sf(), solver.tf());
  Symbol::Factory sf;
  Term::Factory tf;
  auto BOOL = sf.CreateSort();
  auto T = ctx.NewName(BOOL);
  auto P = ctx.NewFun(BOOL, 0)();
  {
    EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{P == T}).reader()));
    EXPECT_FALSE(solver.Entails(1, Formula::Clause(Clause{P == T}).reader()));
    EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{P != T}).reader()));
    EXPECT_FALSE(solver.Entails(1, Formula::Clause(Clause{P != T}).reader()));
    EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{P == T}).reader()));
    EXPECT_FALSE(solver.Entails(1, Formula::Clause(Clause{P == T}).reader()));
    EXPECT_FALSE(solver.Entails(0, Formula::Clause(Clause{P != T}).reader()));
    EXPECT_FALSE(solver.Entails(1, Formula::Clause(Clause{P != T}).reader()));
  }
}

TEST(SolverTest, Constants) {
  Solver solver;
  UnregisterAll();
  Context ctx(solver.sf(), solver.tf());
  Symbol::Factory sf;
  Term::Factory tf;
  auto SomeSort = sf.CreateSort();    RegisterSort(SomeSort, "");
  auto a = ctx.NewFun(SomeSort, 0)(); REGISTER_SYMBOL(a);
  auto b = ctx.NewFun(SomeSort, 0)(); REGISTER_SYMBOL(b);
  {
    for (int i = 0; i < 2; ++i) {
      for (int k = 0; k <= 3; ++k) {
        EXPECT_FALSE(solver.Entails(k, Formula::Clause(Clause{a == b}).reader()));
        EXPECT_FALSE(solver.Entails(k, Formula::Clause(Clause{a != b}).reader()));
      }
    }
  }
}

}  // namespace lela

