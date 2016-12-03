// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <cstdint>

#include <gtest/gtest.h>

#include <lela/internal/hash.h>

namespace lela {
namespace internal {

TEST(HashTest, hash) {
  std::vector<uint64_t> ints1;
  std::vector<uint64_t> ints2;
  for (uint64_t i = 0, n = 1; i <= 19; ++i, n *= 10UL) {
    ASSERT_LE(0UL + n, UINT64_MAX);
    ASSERT_LE(1UL + n, UINT64_MAX);
    ints1.push_back(0UL + n);
    ints2.push_back(1UL + n);
  }
  for (uint64_t i1 : ints1) {
    uint64_t copy = i1;
    EXPECT_EQ(i1, copy);
    EXPECT_EQ(fnv1a_hash(i1), fnv1a_hash(copy));
    for (uint64_t i2 : ints2) {
      EXPECT_NE(i1, i2);
      EXPECT_NE(fnv1a_hash(i1), fnv1a_hash(i2));
    }
  }
}

}  // namespace internal
}  // namespace lela

