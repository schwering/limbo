// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/iter.h>

namespace lela {
namespace internal {

TEST(IterTest, incr_iterator) {
  {
    struct Int {
      Int(int n) : n_(n) {}
      int operator()() const { return n_; }
     private:
      int n_;
    };
    typedef incr_iterator<Int> iterator;
    iterator begin = iterator(5);
    iterator end = iterator(10);
    EXPECT_EQ(std::vector<int>(begin, end), std::vector<int>({5,6,7,8,9}));
  }
}

TEST(IterTest, nested_iterator) {
  {
    typedef std::vector<int> vec;
    typedef std::vector<vec> vecvec;
    vec xs{1,2,3};
    vec ys{4,5,6};
    vec zs{7,8,9};
    vecvec all{xs,ys,zs};
    typedef nested_iterator<vecvec::iterator> iterator;
    iterator begin = iterator(all.begin(), all.end());
    iterator end = iterator(all.end(), all.end());
    EXPECT_EQ(std::vector<int>(begin, end), std::vector<int>({1,2,3,4,5,6,7,8,9}));
  }
}

TEST(IterTest, transform_iterator) {
  {
    struct Func { int operator()(int x) const { return 2*x; } };
    typedef std::vector<int> vec;
    vec xs{1,2,3};
    typedef transform_iterator<Func, vec::iterator> iterator;
    iterator begin = iterator(Func(), xs.begin());
    iterator end = iterator(Func(), xs.end());
    EXPECT_EQ(std::vector<int>(begin, end), std::vector<int>({2,4,6}));
  }
}

TEST(IterTest, transform_range) {
  {
    struct Func { int operator()(int x) const { return 2*x; } };
    typedef std::vector<int> vec;
    vec xs{1,2,3};
    auto r = transform_range(Func(), xs.begin(), xs.end());
    EXPECT_EQ(std::vector<int>(r.begin(), r.end()), std::vector<int>({2,4,6}));
  }
}

TEST(IterTest, filter_iterator) {
  {
    struct Pred { int operator()(int x) const { return x % 2 == 0; } };
    typedef std::vector<int> vec;
    vec xs{1,2,3,4,5,6,7};
    vec ys{2,3,4,6};
    typedef filter_iterator<Pred, vec::iterator> iterator;
    {
      iterator begin = iterator(Pred(), xs.begin(), xs.end());
      iterator end = iterator(Pred(), xs.end(), xs.end());
      EXPECT_EQ(std::vector<int>(begin, end), std::vector<int>({2,4,6}));
    }
    {
      iterator begin = iterator(Pred(), ys.begin(), ys.end());
      iterator end = iterator(Pred(), ys.end(), ys.end());
      EXPECT_EQ(std::vector<int>(begin, end), std::vector<int>({2,4,6}));
    }
  }
}

TEST(IterTest, filter_range) {
  {
    struct Pred { int operator()(int x) const { return x % 2 == 0; } };
    typedef std::vector<int> vec;
    vec xs{1,2,3,4,5,6,7};
    vec ys{2,3,4,6};
    {
      auto r = filter_range(Pred(), xs.begin(), xs.end());
      EXPECT_EQ(std::vector<int>(r.begin(), r.end()), std::vector<int>({2,4,6}));
    }
    {
      auto r = filter_range(Pred(), ys.begin(), ys.end());
      EXPECT_EQ(std::vector<int>(r.begin(), r.end()), std::vector<int>({2,4,6}));
    }
  }
}

TEST(IterTest, join_ranges) {
  {
    typedef std::vector<int> vec;
    vec xs{1,2,3,4,5};
    vec ys{6,7,8,9};
    auto j = join_ranges(xs.begin(), xs.end(), ys.begin(), ys.end());
    EXPECT_EQ(std::vector<int>(j.begin(), j.end()), std::vector<int>({1,2,3,4,5,6,7,8,9}));
  }
}

}  // namespace internal
}  // namespace lela

