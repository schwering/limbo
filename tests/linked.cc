// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#include <gtest/gtest.h>

#include <limbo/internal/intmap.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/linked.h>

namespace limbo {
namespace internal {

template<typename T>
size_t length(T r) { return std::distance(r.begin(), r.end()); }

TEST(Linked, general) {
  {
    typedef IntMap<int, int> Map;
    Linked<Map> x;
    x.container()[0] = 0;
    x.container()[1] = 1;
    x.container()[2] = 2;
    EXPECT_EQ(x.container().n_keys(), 3);
    EXPECT_EQ(x.container()[0], 0);
    EXPECT_EQ(x.container()[1], 1);
    EXPECT_EQ(x.container()[2], 2);
    EXPECT_EQ(length(x), 1);
    auto mapped = transform_crange(x, [](const Map& map) { return map[1]; });
    EXPECT_EQ(length(mapped), 1);
    EXPECT_EQ(*mapped.begin(), 1);
  }
  {
    typedef IntMultiMap<int, int> Map;
    Linked<Map> x;
    x.container()[0].insert({1,2,3});
    x.container()[1].insert({11,22,33});
    x.container()[2].insert({111,222,333});
    Linked<Map> y(&x);
    y.container()[0].insert({4,5,6});
    y.container()[1].insert({44,55,66});
    y.container()[2].insert({444,555,666});
    EXPECT_EQ(x.container().n_keys(), 3);
    EXPECT_EQ(y.container().n_keys(), 3);
    EXPECT_EQ(length(x), 1);
    EXPECT_EQ(length(y), 2);
    auto one = [](const Map&) -> int { return 1; };
    auto size = [](const Map& m) -> int { return m.n_keys(); };
    auto sum = [](int x, int y) -> int { return x + y; };
    EXPECT_EQ(x.Fold(one, sum), 1);
    EXPECT_EQ(y.Fold(one, sum), 2);
    EXPECT_EQ(x.Fold(size, sum), 3);
    EXPECT_EQ(y.Fold(size, sum), 6);
    {
      auto mapped = transform_crange(y, [](const Map& map) -> const Map::Set& { return map[1]; });
      EXPECT_EQ(length(mapped), 2);
      auto it = mapped.begin();
      EXPECT_EQ(*it, Map::Set({44,55,66}));
      ++it;
      EXPECT_EQ(*it, Map::Set({11,22,33}));
      EXPECT_EQ(length(flatten_crange(mapped)), 6);
      auto flattened = flatten_crange(mapped);
      EXPECT_EQ(Map::Set(flattened.begin(), flattened.end()), Map::Set({11,22,33,44,55,66}));
    }
    {
      auto mapped = y.transform([](const Map& map) -> const Map::Set& { return map[1]; });
      EXPECT_EQ(length(mapped), 2);
      auto it = mapped.begin();
      EXPECT_EQ(*it, Map::Set({44,55,66}));
      ++it;
      EXPECT_EQ(*it, Map::Set({11,22,33}));
      EXPECT_EQ(length(flatten_crange(mapped)), 6);
    }
    {
      auto flattened = y.transform_flatten([](const Map& map) -> const Map::Set& { return map[1]; });
      EXPECT_EQ(Map::Set(flattened.begin(), flattened.end()), Map::Set({11,22,33,44,55,66}));
    }
  }
}

}  // namespace internal
}  // namespace limbo

