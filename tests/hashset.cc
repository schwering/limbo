// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <cstdint>

#include <gtest/gtest.h>

#include <lela/internal/hashset.h>

#include <lela/format/output.h>

struct Value {
  Value() = default;
  Value(int x) : x(x) {}
  bool operator==(const Value i) const { return x == i.x; }
  bool operator!=(const Value i) const { return x != i.x; }
  int x;
};

std::ostream& operator<<(std::ostream& os, Value i) { return std::cout << i.x; }

using namespace lela::format::output;

namespace lela {
namespace internal {

TEST(HashSetTest, main) {
  struct Hash { internal::hash_t operator()(Value i) const { return i.x/2; } };
  struct Equal { bool operator()(Value i, Value j) const { return i.x == j.x; } };
  typedef HashSet<Value, Hash, Equal> HS;
  HS hs(10);
  for (int x = 0; x < 10; ++x) {
    bool r = hs.Add(x);
    EXPECT_TRUE(r);
    EXPECT_TRUE(hs.Contains(x));
    EXPECT_TRUE(hs.ContainsHash(Hash()(x)));
  }
  EXPECT_EQ(hs.size(), 10);
  EXPECT_EQ(hs.size(), std::distance(hs.begin(), hs.end()));
  EXPECT_GE(hs.capacity(), 10);
  for (int x = 0; x < 10; ++x) {
    for (auto it = hs.bucket_begin(Value(x)), jt = hs.bucket_end(); it != jt; ++it) {
      EXPECT_TRUE(it->x == x-1 || it->x == x || it->x == x+1);
    }
    EXPECT_EQ(std::distance(hs.bucket_begin(Value(x)), hs.bucket_end()), 2);
  }
  for (int x = 0; x < 10; ++x) {
    bool r = hs.Add(x);
    EXPECT_FALSE(r);
  }
  EXPECT_EQ(hs.size(), 10);
  EXPECT_EQ(hs.size(), std::distance(hs.begin(), hs.end()));
  for (int x = 0; x < 10; ++x) {
    if (x % 2 == 0) {
      hs.Remove(x);
      EXPECT_FALSE(hs.Contains(x));
      EXPECT_TRUE(hs.ContainsHash(Hash()(x)));
    }
  }
  EXPECT_EQ(hs.size(), 5);
  EXPECT_EQ(hs.size(), std::distance(hs.begin(), hs.end()));
  {
    std::vector<Value> xs = std::vector<Value>(hs.begin(), hs.end());
    std::vector<Value> ys = {Value(1),Value(3),Value(5),Value(7),Value(9)};
    EXPECT_EQ(xs, ys);
  }
}

}  // namespace internal
}  // namespace lela

