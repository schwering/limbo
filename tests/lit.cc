// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering

#include <gtest/gtest.h>

#include <limbo/lit.h>

namespace limbo {

TEST(LitTest, FunConstruction) {
  EXPECT_TRUE(Fun().null());
  EXPECT_FALSE(Fun());
  EXPECT_EQ(Fun().id(), 0);
  EXPECT_FALSE(Fun::FromId(1).null());
  EXPECT_TRUE(Fun::FromId(1));
  EXPECT_EQ(Fun::FromId(1).id(), 1);
}

TEST(LitTest, FunComparison) {
  EXPECT_EQ(Fun::FromId(2), Fun::FromId(2));
  EXPECT_GE(Fun::FromId(2), Fun::FromId(2));
  EXPECT_LE(Fun::FromId(2), Fun::FromId(2));
  EXPECT_NE(Fun::FromId(1), Fun::FromId(2));
  EXPECT_LT(Fun::FromId(1), Fun::FromId(2));
  EXPECT_LE(Fun::FromId(1), Fun::FromId(2));
  EXPECT_GT(Fun::FromId(2), Fun::FromId(1));
  EXPECT_GE(Fun::FromId(2), Fun::FromId(1));
}

TEST(LitTest, NameConstruction) {
  EXPECT_TRUE(Name().null());
  EXPECT_FALSE(Name());
  EXPECT_EQ(Name().id(), 0);
  EXPECT_FALSE(Name::FromId(1).null());
  EXPECT_TRUE(Name::FromId(1));
  EXPECT_EQ(Name::FromId(1).id(), 1);
}

TEST(LitTest, NameComparison) {
  EXPECT_EQ(Name::FromId(2), Name::FromId(2));
  EXPECT_GE(Name::FromId(2), Name::FromId(2));
  EXPECT_LE(Name::FromId(2), Name::FromId(2));
  EXPECT_NE(Name::FromId(1), Name::FromId(2));
  EXPECT_LT(Name::FromId(1), Name::FromId(2));
  EXPECT_LE(Name::FromId(1), Name::FromId(2));
  EXPECT_GT(Name::FromId(2), Name::FromId(1));
  EXPECT_GE(Name::FromId(2), Name::FromId(1));
}

TEST(LitTest, LitComparison) {
  Fun f1 = Fun::FromId(1);
  Fun f2 = Fun::FromId(2);
  Name n1 = Name::FromId(1);
  Name n2 = Name::FromId(2);
  EXPECT_TRUE(Lit().null());
  EXPECT_FALSE(Lit::Eq(f1, n1).null());
  EXPECT_EQ(Lit::Eq(f1, n1), Lit::Eq(f1, n1));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Eq(f1, n2));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Eq(f2, n1));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Eq(f2, n2));
  EXPECT_EQ(Lit::Eq(f1, n1), Lit::Eq(f1, n1).flip().flip());
  EXPECT_EQ(Lit::Eq(f1, n1), Lit::Neq(f1, n1).flip());
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Neq(f1, n1));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Neq(f1, n2));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Neq(f2, n1));
  EXPECT_NE(Lit::Eq(f1, n1), Lit::Neq(f2, n2));
  EXPECT_LE(Lit::Eq(f1, n1), Lit::Eq(f1, n1));
  EXPECT_GE(Lit::Eq(f1, n1), Lit::Eq(f1, n1));
  EXPECT_LT(Lit::Eq(f1, n1), Lit::Eq(f2, n2));
  EXPECT_GT(Lit::Eq(f2, n2), Lit::Eq(f1, n1));
}

TEST(LitTest, LitValid) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);
  EXPECT_FALSE(Lit::Valid(Lit::Eq (f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::Valid(Lit::Neq(f, m), Lit::Neq(f, m)));
  EXPECT_TRUE (Lit::Valid(Lit::Eq (f, m), Lit::Neq(f, m)));
  EXPECT_TRUE (Lit::Valid(Lit::Neq(f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::Valid(Lit::Eq (f, m), Lit::Eq (f, n)));
  EXPECT_FALSE(Lit::Valid(Lit::Eq (f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::Valid(Lit::Neq(f, m), Lit::Eq (f, n)));
  EXPECT_TRUE (Lit::Valid(Lit::Neq(f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::Valid(Lit::Neq(f, n), Lit::Neq(g, n)));
}

TEST(LitTest, LitComplementary) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);
  EXPECT_FALSE(Lit::Complementary(Lit::Eq (f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::Complementary(Lit::Neq(f, m), Lit::Neq(f, m)));
  EXPECT_TRUE (Lit::Complementary(Lit::Eq (f, m), Lit::Neq(f, m)));
  EXPECT_TRUE (Lit::Complementary(Lit::Neq(f, m), Lit::Eq (f, m)));
  EXPECT_TRUE (Lit::Complementary(Lit::Eq (f, m), Lit::Eq (f, n)));
  EXPECT_FALSE(Lit::Complementary(Lit::Eq (f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::Complementary(Lit::Neq(f, m), Lit::Eq (f, n)));
  EXPECT_FALSE(Lit::Complementary(Lit::Neq(f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::Complementary(Lit::Eq (f, m), Lit::Eq (g, n)));
  EXPECT_FALSE(Lit::Complementary(Lit::Eq (f, n), Lit::Eq (g, n)));
}

TEST(LitTest, LitProperlySubsumes) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Neq(f, m), Lit::Neq(f, m)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Neq(f, m)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Neq(f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Eq (f, n)));
  EXPECT_TRUE (Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Neq(f, m), Lit::Eq (f, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Neq(f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Neq(g, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, n), Lit::Neq(g, n)));
}

TEST(LitTest, LitSubsumes) {
  Fun f = Fun::FromId(1);
  Fun g = Fun::FromId(2);
  Name m = Name::FromId(1);
  Name n = Name::FromId(2);
  EXPECT_TRUE (Lit::Subsumes(Lit::Eq (f, m), Lit::Eq (f, m)));
  EXPECT_TRUE (Lit::Subsumes(Lit::Neq(f, m), Lit::Neq(f, m)));
  EXPECT_FALSE(Lit::Subsumes(Lit::Eq (f, m), Lit::Neq(f, m)));
  EXPECT_FALSE(Lit::Subsumes(Lit::Neq(f, m), Lit::Eq (f, m)));
  EXPECT_FALSE(Lit::Subsumes(Lit::Eq (f, m), Lit::Eq (f, n)));
  EXPECT_TRUE (Lit::Subsumes(Lit::Eq (f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::Subsumes(Lit::Neq(f, m), Lit::Eq (f, n)));
  EXPECT_FALSE(Lit::Subsumes(Lit::Neq(f, m), Lit::Neq(f, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, m), Lit::Neq(g, n)));
  EXPECT_FALSE(Lit::ProperlySubsumes(Lit::Eq (f, n), Lit::Neq(g, n)));
}

}  // namespace limbo

