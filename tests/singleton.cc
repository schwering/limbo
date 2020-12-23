// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <cstdint>

#include <gtest/gtest.h>

#include <limbo/internal/singleton.h>

namespace limbo {
namespace internal {

int instances = 0;

class Thing : private Singleton<Thing> {
 public:
  static Thing& instance() {
    if (instance_ == nullptr) {
      instance_ = std::unique_ptr<Thing>(new Thing());
    }
    return *instance_.get();
  }

 private:
  Thing() { ++instances; }
};

TEST(SingletonTest, NumberOfInstances) {
  ASSERT_EQ(instances, 0);
  Thing::instance();
  EXPECT_EQ(instances, 1);
  Thing::instance();
  EXPECT_EQ(instances, 1);
}

}  // namespace internal
}  // namespace limbo

