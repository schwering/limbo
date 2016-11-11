// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./pretty.h"

namespace lela {

using namespace input;
using namespace output;

#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

TEST(Input, general) {
  Symbol::Factory sf;
  Term::Factory tf;
  Context ctx(&sf, &tf);
  auto Bool = ctx.NewSort();
  auto True = ctx.NewName(Bool);            REGISTER_SYMBOL(True);
  auto HUMAN = ctx.NewSort();
  auto Father = ctx.NewFun(HUMAN, 1);       REGISTER_SYMBOL(Father);
  auto Mother = ctx.NewFun(HUMAN, 1);       REGISTER_SYMBOL(Mother);
  auto IsParentOf = ctx.NewFun(Bool, 2);    REGISTER_SYMBOL(IsParentOf);
  auto John = ctx.NewFun(Bool, 0);          REGISTER_SYMBOL(John);
  auto x = ctx.NewVar(HUMAN);               REGISTER_SYMBOL(x);
  auto y = ctx.NewVar(HUMAN);               REGISTER_SYMBOL(y);
  {
    Formula phi = Ex(x, John() == x);
    std::cout << phi << std::endl;
    std::cout << phi.reader().NF() << std::endl;
  }
  {
    Formula phi = Fa(x, John() == x);
    std::cout << phi << std::endl;
    std::cout << phi.reader().NF() << std::endl;
  }
  {
    Formula phi = Fa(x, IsParentOf(Mother(x), x) == True && IsParentOf(Father(x), x) == True);
    std::cout << phi << std::endl;
    std::cout << phi.reader().NF() << std::endl;
  }
  {
    Formula phi = Fa(x, IsParentOf(x, y) == True && IsParentOf(Father(x), x) == True);
    std::cout << phi << std::endl;
    std::cout << phi.reader().NF() << std::endl;
  }
}

}  // namespace lela

