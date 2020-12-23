// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <cstdint>

#include <gtest/gtest.h>

#include <limbo/internal/ringbuffer.h>

namespace limbo {
namespace internal {

TEST(RingBufferTest, EmptyRingBuffer) {
  RingBuffer<int> rb;
  EXPECT_EQ(rb.size(), 0);
}

TEST(RingBufferTest, RingBufferFrontAndBack) {
  RingBuffer<int> rb;
  for (int i = 0; i < 1000; ++i) {
    rb.PushFront(-i - 1);
    rb.PushBack(i);
  }
  EXPECT_EQ(rb.size(), 2000);
  for (int i = 0; i < rb.size(); ++i) {
    EXPECT_EQ(rb[i], i - 1000);
  }

  {
    RingBuffer<int> tmp(std::move(rb));
    EXPECT_TRUE(rb.empty());
    EXPECT_EQ(rb.size(), 0);
    EXPECT_FALSE(tmp.empty());
    EXPECT_EQ(tmp.size(), 2000);
    for (int i = 0; i < tmp.size(); ++i) {
      EXPECT_EQ(tmp[i], i - 1000);
    }
    rb = std::move(tmp);
  }

  for (int i = 999; i >= 0; --i) {
    int x = rb.PopBack();
    EXPECT_EQ(x, i);
  }
  EXPECT_EQ(rb.size(), 1000);
  for (int i = 0; i < rb.size(); ++i) {
    EXPECT_EQ(rb[i], i - 1000);
  }
  for (int i = -1; i >= 1000; --i) {
    int x = rb.PopBack();
    EXPECT_EQ(x, i);
  }
}

}  // namespace internal
}  // namespace limbo

