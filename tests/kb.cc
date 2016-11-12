// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./kb.h"
#include "./pretty.h"

using namespace lela::output;

namespace lela {

using namespace input;
using namespace output;

#define REGISTER_SORT(x)      RegisterSort(x, #x)
#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

TEST(KB, general) {
  KB kb;
  Context ctx(kb.sf(), kb.tf());
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
    kb.AddClause(Clause{ Mother(x) != y, x == y, IsParentOf(y,x) == True });
    kb.AddClause(Clause{ Mother(Jesus) == Mary });
    std::cout << kb.g_.Ground() << std::endl;
    Formula phi = Ex(x, Ex(y, IsParentOf(y,x) == True)).reader().NF();
    std::cout << phi << std::endl;
    EXPECT_TRUE(kb.Entails(0, phi.reader()));
    EXPECT_TRUE(kb.Entails(1, phi.reader()));
  }
}

}  // namespace lela

