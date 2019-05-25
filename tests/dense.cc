// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <gtest/gtest.h>

#include <limbo/internal/dense.h>

namespace limbo {
namespace internal {

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

template<typename Source, typename Target>
struct To { Target operator()(Source x) const { return Target(x); } };

template<typename Key, typename Value>
using IntMap = DenseMap<Key, Value, int, 0, To<Key, int>, To<int, Key>, SlowAdjustBoundCheck>;

TEST(DenseMapTest, general) {
  IntMap<int, std::string> map;
  map[0] = "zero";
  map[2] = "two";
  EXPECT_EQ(map[0], "zero");
  EXPECT_EQ(map[1], "");
  EXPECT_EQ(map[2], "two");
  EXPECT_EQ(length(map.keys()), 3u);
  EXPECT_EQ(length(map.values()), 3u);
  EXPECT_EQ(map.upper_bound(), 2);

  const IntMap<int, std::string> map2 = map;
  EXPECT_EQ(length(map2.keys()), 3u);
  EXPECT_EQ(length(map2.values()), 3u);
  EXPECT_EQ(map2[0], "zero");
  EXPECT_EQ(map2[1], "");
  EXPECT_EQ(map2[2], "two");

  map[1] = "one";
  EXPECT_EQ(map[0], "zero");
  EXPECT_EQ(map[1], "one");
  EXPECT_EQ(map[2], "two");

  EXPECT_EQ(map2[0], "zero");
  EXPECT_EQ(map2[1], "");
  EXPECT_EQ(map2[2], "two");

  //map.set_null_value("null");
  map[4] = "four";
  EXPECT_EQ(length(map.keys()), 5u);
  EXPECT_EQ(length(map.values()), 5u);
  EXPECT_EQ(map.upper_bound(), 4);
  EXPECT_EQ(map[0], "zero");
  EXPECT_EQ(map[1], "one");
  EXPECT_EQ(map[2], "two");
  EXPECT_EQ(map[3], "");
  EXPECT_EQ(map[4], "four");
}

}  // namespace internal
}  // namespace limbo

