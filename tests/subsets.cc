// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering

#include <vector>

#include <gtest/gtest.h>

#include <limbo/internal/subsets.h>

namespace limbo {
namespace internal {

TEST(SubsetsTest, subsets) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  Xs.push_back({11, 22, 33});
  std::vector<std::vector<int>> Ys;
  AllCombinedSubsetsOfSize(Xs, 3, [&](const std::vector<int>& xs) { Ys.push_back(xs); /*printf("xs = "); for (auto x : xs) { printf("%d ", x); } printf("\n");*/ return true; });
  for (const std::vector<int>& ys : Ys) {
    printf("ys = "); for (auto y : ys) { printf("%d ", y); } printf("\n");
  }
}

TEST(SubsetsTest, subsets0) {
  std::vector<std::vector<int>> Xs;
  Xs.push_back({1, 2, 3});
  std::vector<std::vector<int>> Ys;
  AllCombinedSubsetsOfSize(Xs, 3, [&](const std::vector<int>& xs) { Ys.push_back(xs); /*printf("xs = "); for (auto x : xs) { printf("%d ", x); } printf("\n");*/ return true; });
  for (const std::vector<int>& ys : Ys) {
    printf("ys = "); for (auto y : ys) { printf("%d ", y); } printf("\n");
  }
}

}  // namespace internal
}  // namespace limbo

