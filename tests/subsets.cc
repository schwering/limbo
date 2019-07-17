// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering

#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <limbo/internal/subsets.h>
#include <limbo/io/output.h>

namespace limbo {
namespace internal {

TEST(SubsetsTest, subsets_empty_set_cardinality_zero) {
  std::vector<std::vector<int>> Xs;
  std::set<std::vector<int>> Ys;
  int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 0, [&](const std::vector<int>& xs) { std::cout << xs << std::endl; Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 0);
  r = AllCombinedSubsetsOfSize(Xs, 0, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return false; });
  EXPECT_TRUE(r);
}

TEST(SubsetsTest, subsets_empty_set_cardinality_one) {
  std::vector<std::vector<int>> Xs;
  std::set<std::vector<int>> Ys;
  int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 1, [&](const std::vector<int>& xs) { std::cout << xs << std::endl; Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 0);
  r = AllCombinedSubsetsOfSize(Xs, 0, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return false; });
  EXPECT_TRUE(r);
}

TEST(SubsetsTest, subsets_two_ternary_sets_cardinality_zero) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  Xs.push_back({11, 22, 33});
  std::set<std::vector<int>> Ys;
  unsigned int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 0, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 0);
  r = AllCombinedSubsetsOfSize(Xs, 0, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return false; });
  EXPECT_TRUE(r);
}

TEST(SubsetsTest, subsets_two_ternary_sets_cardinality_one) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  Xs.push_back({11, 22, 33});
  std::set<std::vector<int>> Ys;
  unsigned int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 1, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 0);
  r = AllCombinedSubsetsOfSize(Xs, 1, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return false; });
  EXPECT_TRUE(r);
}

TEST(SubsetsTest, subsets_two_ternary_sets_cardinality_two) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  Xs.push_back({11, 22, 33});
  std::set<std::vector<int>> Ys;
  int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 2, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 3*3);
  for (int i = 1; i <= 3; ++i) {
    for (int j = 11; j <= 33; j += 11) {
      EXPECT_TRUE(Ys.find({i,j}) != Ys.end());
    }
  }
}

TEST(SubsetsTest, subsets_two_ternary_sets_cardinality_three) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  Xs.push_back({11, 22, 33});
  std::set<std::vector<int>> Ys;
  int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 3, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  //for (const std::vector<int>& ys : Ys) {
  //  printf("ys = "); for (auto y : ys) { printf("%d ", y); } printf("\n");
  //}
  EXPECT_EQ(Ys.size(), unsigned(n_subsets));
  EXPECT_EQ(Ys.size(), unsigned(3*3+3*3));
  for (int i = 1; i <= 3; ++i) {
    EXPECT_TRUE(Ys.find({i,11,22}) != Ys.end());
    EXPECT_TRUE(Ys.find({i,11,33}) != Ys.end());
    EXPECT_TRUE(Ys.find({i,22,33}) != Ys.end());
  }
  for (int i = 11; i <= 33; i += 11) {
    EXPECT_TRUE(Ys.find({1,2,i}) != Ys.end());
    EXPECT_TRUE(Ys.find({1,3,i}) != Ys.end());
    EXPECT_TRUE(Ys.find({2,3,i}) != Ys.end());
  }
}

TEST(SubsetsTest, subsets_one_ternary_set_cardinality_three) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  std::set<std::vector<int>> Ys;
  int n_subsets = 0;
  bool r = AllCombinedSubsetsOfSize(Xs, 3, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return true; });
  EXPECT_TRUE(r);
  EXPECT_EQ(n_subsets, 0);
  r = AllCombinedSubsetsOfSize(Xs, 3, [&](const std::vector<int>& xs) { Ys.insert(xs); ++n_subsets; return false; });
  EXPECT_TRUE(r);
}

}  // namespace internal
}  // namespace limbo

