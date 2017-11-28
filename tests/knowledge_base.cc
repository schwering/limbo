// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <gtest/gtest.h>

#include <limbo/knowledge_base.h>
#include <limbo/format/output.h>
#include <limbo/format/cpp/syntax.h>

namespace limbo {

using namespace limbo::format;
using namespace limbo::format::cpp;

#define REGISTER_SYMBOL(x)    RegisterSymbol(x, #x)

inline void RegisterSymbol(Term t, const std::string& n) {
  RegisterSymbol(t.symbol(), n);
}

TEST(KnowledgeBaseTest, ECAI2016Sound_Guarantee) {
  Context ctx;
  KnowledgeBase kb(ctx.sf(), ctx.tf());
  auto Bool = ctx.CreateNonrigidSort();           RegisterSort(Bool, "");
  auto Food = ctx.CreateNonrigidSort();           RegisterSort(Food, "");
  auto T = ctx.CreateName(Bool);                  REGISTER_SYMBOL(T);
  auto Aussie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.CreateFunction(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.CreateName(Food);                REGISTER_SYMBOL(roo);
  auto x = ctx.CreateVariable(Food);              REGISTER_SYMBOL(x);
  Formula::belief_level k = 1;
  Formula::belief_level l = 1;
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(Aussie == T), *(Italian != T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(Italian == T), *(Aussie != T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(Aussie == T), *(Eats(roo) == T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(T == T), *(Italian == T || Veggie == T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(Italian != T), *(Aussie == T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(Meat(roo) != T), *(T != T)))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Guarantee(Formula::Factory::Bel(k, l, *(~Fa(x, (Veggie == T && Meat(x) == T) >> (Eats(x) != T))), *(T != T)))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Guarantee(Formula::Factory::Bel(0, 0, *(Italian != T), *(Veggie != T)))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Guarantee(Formula::Factory::Bel(0, 1, *(Italian != T), *(Veggie != T)))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Guarantee(Formula::Factory::Bel(1, 0, *(Italian != T), *(Veggie != T)))));
  EXPECT_TRUE(kb.Entails(*Formula::Factory::Guarantee(Formula::Factory::Bel(1, 1, *(Italian != T), *(Veggie != T)))));
}

TEST(KnowledgeBaseTest, ECAI2016Sound_NoGuarantee) {
  Context ctx;
  KnowledgeBase kb(ctx.sf(), ctx.tf());
  auto Bool = ctx.CreateNonrigidSort();           RegisterSort(Bool, "");
  auto Food = ctx.CreateNonrigidSort();           RegisterSort(Food, "");
  auto T = ctx.CreateName(Bool);                  REGISTER_SYMBOL(T);
  auto Aussie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Aussie);
  auto Italian = ctx.CreateFunction(Bool, 0)();   REGISTER_SYMBOL(Italian);
  auto Eats = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Eats);
  auto Meat = ctx.CreateFunction(Bool, 1);        REGISTER_SYMBOL(Meat);
  auto Veggie = ctx.CreateFunction(Bool, 0)();    REGISTER_SYMBOL(Veggie);
  auto roo = ctx.CreateName(Food);                REGISTER_SYMBOL(roo);
  auto x = ctx.CreateVariable(Food);              REGISTER_SYMBOL(x);
  Formula::belief_level k = 1;
  Formula::belief_level l = 1;
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(Aussie == T), *(Italian != T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(Italian == T), *(Aussie != T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(Aussie == T), *(Eats(roo) == T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(T == T), *(Italian == T || Veggie == T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(Italian != T), *(Aussie == T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(Meat(roo) != T), *(T != T))));
  EXPECT_TRUE(kb.Add(*Formula::Factory::Bel(k, l, *(~Fa(x, (Veggie == T && Meat(x) == T) >> (Eats(x) != T))), *(T != T))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Bel(0, 0, *(Italian != T), *(Veggie != T))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Bel(0, 1, *(Italian != T), *(Veggie != T))));
  EXPECT_FALSE(kb.Entails(*Formula::Factory::Bel(1, 0, *(Italian != T), *(Veggie != T))));
  EXPECT_TRUE(kb.Entails(*Formula::Factory::Bel(1, 1, *(Italian != T), *(Veggie != T))));
}

}  // namespace limbo

