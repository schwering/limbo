// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#ifndef SRC_RANGE_H_
#define SRC_RANGE_H_

#include <utility>

namespace lela {

template<typename T>
struct Range {
  Range() : first(), last() {}
  Range(T&& first, T&& last) : first(first), last(last) {}  // NOLINT
  T first;
  T last;
  T begin() const { return first; }
  T end() const { return last; }
};

struct EmptyRangeType {
  template<typename T>
  operator Range<T>() const {
    return Range<T>();
  }
};

constexpr EmptyRangeType EmptyRange = EmptyRangeType();

#if 0
template<typename Container>
Range<typename Container::iterator> MakeRange(Container& c) {
  return Range<typename Container::iterator>(c.begin(), c.end());
}

template<typename Container>
Range<typename Container::const_iterator> MakeRange(const Container& c) {
  return Range<typename Container::const_iterator>(c.begin(), c.end());
}
#endif

template<typename T>
Range<T> MakeRange(T&& first, T&& last) {  // NOLINT
  return Range<T>(std::forward<T>(first), std::forward<T>(last));  // NOLINT
}

}  // namespace lela

#endif  // SRC_RANGE_H_

