// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>
#include "./pretty.h"

namespace lela {

using namespace input;
using namespace output;

TEST(Input, general) {
  Symbol::Factory sf;
  Term::Factory tf;
  Context ctx(&sf, &tf);
  auto Bool = ctx.NewSort();
  auto True = ctx.NewName(Bool);
  auto HUMAN = ctx.NewSort();
  auto Father = ctx.NewFun(HUMAN, 1);
  auto Mother = ctx.NewFun(HUMAN, 1);
  auto IsParentOf = ctx.NewFun(Bool, 2);
  auto John = ctx.NewFun(Bool, 0);
  auto x = ctx.NewVar(HUMAN);
  auto y = ctx.NewVar(HUMAN);
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

