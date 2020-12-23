// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <cstdint>

#include <gtest/gtest.h>

#include <limbo/internal/ints.h>

namespace limbo {
namespace internal {

TEST(IntsTest, ints) {
  EXPECT_EQ(sizeof(i8  ), 1u);
  EXPECT_EQ(sizeof(i16 ), 2u);
  EXPECT_EQ(sizeof(i32 ), 4u);
  EXPECT_EQ(sizeof(i64 ), 8u);
  EXPECT_EQ(sizeof(u8  ), 1u);
  EXPECT_EQ(sizeof(u16 ), 2u);
  EXPECT_EQ(sizeof(u32 ), 4u);
  EXPECT_EQ(sizeof(u64 ), 8u);
  EXPECT_EQ(sizeof(uint), sizeof(int));
  EXPECT_EQ(sizeof(size_t), 8);
  EXPECT_EQ(sizeof(uptr_t), sizeof(void*));
  EXPECT_EQ(sizeof(iptr_t), sizeof(void*));
}

TEST(IntsTest, BitInterleaver) {
  EXPECT_EQ(BitInterleaver<u16>::merge(0b100100, 0b000000), 0b100000100000);
  EXPECT_EQ(BitInterleaver<u16>::merge(0b000000, 0b100100), 0b010000010000);
  EXPECT_EQ(BitInterleaver<u16>::merge(0b000101, 0b000011), 0b000000100111);
  EXPECT_EQ(BitInterleaver<u32>::merge(0b100100, 0b000000), 0b100000100000L);
  EXPECT_EQ(BitInterleaver<u32>::merge(0b000000, 0b100100), 0b010000010000L);
  EXPECT_EQ(BitInterleaver<u32>::merge(0b000101, 0b000011), 0b000000100111L);
}

TEST(IntsTest, BitConcatenator) {
  EXPECT_EQ(BitConcatenator<u16>::merge(0b100100, 0b000000), 0b100100 << 16);
  EXPECT_EQ(BitConcatenator<u16>::merge(0b000000, 0b100100), 0b100100);
  EXPECT_EQ(BitConcatenator<u16>::merge(0b000101, 0b000011), (0b000101 << 16) | 0b000011);
  EXPECT_EQ(BitConcatenator<u32>::merge(0b100100, 0b000000), 0b100100L << 32);
  EXPECT_EQ(BitConcatenator<u32>::merge(0b000000, 0b100100), 0b100100L);
  EXPECT_EQ(BitConcatenator<u32>::merge(0b000101, 0b000011), (0b000101L << 32) | 0b000011L);
}

TEST(IntsTest, next_power_of_two) {
  EXPECT_EQ(next_power_of_two(128), 128);
  EXPECT_EQ(next_power_of_two(127), 128);
  EXPECT_EQ(next_power_of_two(111), 128);
  EXPECT_EQ(next_power_of_two(47), 64);
}

}  // namespace internal
}  // namespace limbo

