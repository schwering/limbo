// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <gtest/gtest.h>

#include <lela/internal/iter.h>

namespace lela {
namespace internal {

struct Int {
  Int(int n) : n_(n) {}

  Int operator-() const { return Int(-n_); }
  Int operator+(Int i) const { return Int(n_ + i.n_); }
  Int operator+(int i) const { return Int(n_ + i); }
  Int operator-(Int i) const { return Int(n_ - i.n_); }
  Int operator-(int i) const { return Int(n_ - i); }
  Int operator*(Int i) const { return Int(n_ * i.n_); }
  Int operator*(int i) const { return Int(n_ * i); }
  Int operator/(Int i) const { return Int(n_ / i.n_); }
  Int operator/(int i) const { return Int(n_ / i); }
  Int operator%(Int i) const { return Int(n_ % i.n_); }
  Int operator%(int i) const { return Int(n_ % i); }

  Int& operator++() { ++n_; return *this; }
  Int& operator--() { --n_; return *this; }
  bool operator==(Int i) const { return n_ == i.n_; }
  bool operator!=(Int i) const { return !(*this == i); }

  int val() const { return n_; }

 private:
  int n_;
};

TEST(IterTest, int_iterator) {
  {
    struct PlusOne {
      Int operator()(Int i) const { return i + 1; }
    };
    typedef int_iterator<Int, PlusOne> iterator;
    iterator begin = iterator(5);
    iterator end = iterator(10);
    EXPECT_EQ(std::vector<Int>(begin, end), std::vector<Int>({6,7,8,9,10}));
    for (auto it = begin; it != end; ++it) {
      EXPECT_EQ((*it).val(), it->val());
    }
    for (auto it = begin, jt = begin; it != end; ++it) {
      EXPECT_EQ(*it, *jt++);
    }
  }
}

TEST(IterTest, flatten_iterator) {
  {
    typedef std::vector<Int> vec;
    typedef std::vector<vec> vecvec;
    vec xs{1,2,3};
    vec ys{4,5,6};
    vec zs{7,8,9};
    vecvec all{xs,ys,zs};
    typedef flatten_iterator<vecvec::iterator> iterator;
    iterator begin = iterator(all.begin(), all.end());
    iterator end = iterator(all.end(), all.end());
    EXPECT_EQ(std::vector<Int>(begin, end), std::vector<Int>({1,2,3,4,5,6,7,8,9}));
    for (auto it = begin; it != end; ++it) {
      EXPECT_EQ((*it).val(), it->val());
    }
    for (auto it = begin, jt = begin; it != end; ++it) {
      EXPECT_EQ(*it, *jt++);
    }
  }
}

TEST(IterTest, transform_iterator) {
  {
    struct Func { Int operator()(Int x) const { return x*2; } };
    typedef std::vector<Int> vec;
    vec xs{1,2,3};
    typedef transform_iterator<vec::iterator, Func> iterator;
    iterator begin = iterator(xs.begin());
    iterator end = iterator(xs.end());
    EXPECT_EQ(std::vector<Int>(begin, end), std::vector<Int>({2,4,6}));
    for (auto it = begin; it != end; ++it) {
      EXPECT_EQ((*it).val(), it->val());
    }
    for (auto it = begin, jt = begin; it != end; ++it) {
      EXPECT_EQ(*it, *jt++);
    }
  }
}

TEST(IterTest, transform_range) {
  {
    struct Func { int operator()(int x) const { return 2*x; } };
    typedef std::vector<int> vec;
    vec xs{1,2,3};
    auto r = transform_range(xs.begin(), xs.end(), Func());
    EXPECT_EQ(std::vector<int>(r.begin(), r.end()), std::vector<int>({2,4,6}));
  }
}

TEST(IterTest, filter_iterator) {
  {
    struct Pred { bool operator()(Int x) const { return x % 2 == 0; } };
    typedef std::vector<Int> vec;
    vec xs{1,2,3,4,5,6,7};
    vec ys{2,3,4,6};
    typedef filter_iterator<vec::iterator, Pred> iterator;
    {
      iterator begin = iterator(xs.begin(), xs.end());
      iterator end = iterator(xs.end(), xs.end());
      EXPECT_EQ(std::vector<Int>(begin, end), std::vector<Int>({2,4,6}));
      for (auto it = begin; it != end; ++it) {
        EXPECT_EQ((*it).val(), it->val());
      }
      for (auto it = begin, jt = begin; it != end; ++it) {
        EXPECT_EQ(*it, *jt++);
      }
    }
    {
      iterator begin = iterator(ys.begin(), ys.end());
      iterator end = iterator(ys.end(), ys.end());
      EXPECT_EQ(std::vector<Int>(begin, end), std::vector<Int>({2,4,6}));
      for (auto it = begin; it != end; ++it) {
        EXPECT_EQ((*it).val(), it->val());
      }
      for (auto it = begin, jt = begin; it != end; ++it) {
        EXPECT_EQ(*it, *jt++);
      }
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
      auto r = filter_range(xs.begin(), xs.end(), Pred());
      EXPECT_EQ(std::vector<int>(r.begin(), r.end()), std::vector<int>({2,4,6}));
    }
    {
      auto r = filter_range(ys.begin(), ys.end(), Pred());
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

