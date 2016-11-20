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
    std::cout << solver.grounder().Ground() << std::endl;
    Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
    std::cout << phi << std::endl;
    EXPECT_TRUE(solver.Entails(0, phi.reader()));
    EXPECT_TRUE(solver.Entails(1, phi.reader()));
    EXPECT_TRUE(solver.Entails(0, phi.reader()));
    EXPECT_TRUE(solver.Entails(1, phi.reader()));
  }
}

TEST(SolverTest, Entails2) {
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
    std::cout << solver.grounder().Ground() << std::endl;
    Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
    std::cout << phi << std::endl;
    EXPECT_FALSE(solver.Entails(0, phi.reader()));
    EXPECT_TRUE(solver.Entails(1, phi.reader()));
    EXPECT_FALSE(solver.Entails(0, phi.reader()));
    EXPECT_TRUE(solver.Entails(1, phi.reader()));
  }
}

TEST(SolverTest, Entails3) {
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
    std::cout << solver.grounder().Ground() << std::endl;
    Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
    std::cout << phi << std::endl;
    EXPECT_FALSE(solver.Entails(0, phi.reader()));
    EXPECT_TRUE(solver.Entails(1, phi.reader()));
  }
}

}  // namespace lela

