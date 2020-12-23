// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// Functions to determine all subsets.

#ifndef LIMBO_INTERNAL_SUBSETS_H_
#define LIMBO_INTERNAL_SUBSETS_H_

#include <cassert>
#include <cstdio>
#include <algorithm>
#include <vector>

namespace limbo {
namespace internal {

template<typename SetContainer,
         typename UnaryPredicate,
         typename RandomAccessIt = typename SetContainer::const_iterator,
         typename T = typename SetContainer::value_type>
bool AllSubsetsOfSize(RandomAccessIt X_begin,
                      RandomAccessIt X_end,
                      const int xs_wanted,
                      SetContainer* xs,
                      UnaryPredicate pred = UnaryPredicate()) {
  int X_size = int(std::distance(X_begin, X_end));
  if (xs_wanted == 0 || X_size == 0) {
    return xs_wanted == 0 && pred(xs);
  } else if (X_size < xs_wanted) {
    return false;
  } else {
    if (X_size > xs_wanted) {
      const bool succ = AllSubsetsOfSize(std::next(X_begin), X_end, xs_wanted, xs, pred);
      if (!succ) {
        return false;
      }
    }
    const T& x = *X_begin;
    xs->push_back(x);
    const bool succ = AllSubsetsOfSize(std::next(X_begin), X_end, xs_wanted - 1, xs, pred);
    xs->pop_back();
    return succ;
  }
}

template<typename SetSetContainer,
         typename UnaryPredicate,
         typename SetContainer = typename SetSetContainer::value_type,
         typename T = typename SetContainer::value_type>
bool AllCombinedSubsetsOfSize(const SetSetContainer& Xs,
                              const std::vector<int>& n_not_yet_covered_in,
                              const int xs_wanted,
                              const int index,
                              std::vector<T>* xs,
                              UnaryPredicate pred = UnaryPredicate()) {
  assert(index <= int(Xs.size()));
  if (index == int(Xs.size())) {
    return Xs.empty() || (xs_wanted == 0 && pred(*xs));
  }
  assert(!Xs[index].empty());
  // What is the minimum number of functions we xs_wanted to assign from this bucket?
  //     (xs_wanted - min_assign < n_not_yet_covered_in[index] - (n_not_yet_covered_in.size() - index - 1))
  // <=> (min_assign > xs_wanted - n_not_yet_covered_in[index] + n_not_yet_covered_in.size() - index - 1)
  // <=> (min_assign >= xs_wanted - n_not_yet_covered_in[index] + n_not_yet_covered_in.size() - index - 1)
  const int min_assign = std::max(0,
      xs_wanted - n_not_yet_covered_in[index] + int(n_not_yet_covered_in.size()) - index - 1);
  // What is the maximum number of functions we can assign from this buckt?
  const int max_assign = std::min(xs_wanted - xs->empty(), int(Xs[index].size()) - 1);
  for (int i = min_assign; i <= max_assign; ++i) {
    const bool succ = AllSubsetsOfSize(Xs[index].begin(), Xs[index].end(), i, xs, [&](std::vector<T>* xs) {
      bool succ = AllCombinedSubsetsOfSize(Xs, n_not_yet_covered_in, xs_wanted - i, index + 1, xs, pred);
      return succ;
    });
    if (!succ) {
      return false;
    }
  }
  //std::printf("%d: true\n", __LINE__);
  return true;
}

template<typename SetSetContainer,
         typename UnaryPredicate,
         typename SetContainer = typename SetSetContainer::value_type,
         typename T = typename SetContainer::value_type>
bool AllCombinedSubsetsOfSize(const SetSetContainer& Xs, const int xs_wanted, UnaryPredicate pred = UnaryPredicate()) {
  SetContainer xs;
  std::vector<int> n_not_yet_covered_in(Xs.size());
  for (int i = int(n_not_yet_covered_in.size()) - 2; i >= 0; --i) {
    n_not_yet_covered_in[i] = int(Xs[i+1].size()) + n_not_yet_covered_in[i+1];
  }
  //for (int i = 0; i < Xs.size(); ++i) { printf("Xs[%d] =", i); for (const T f : Xs[i]) { printf(" %d", int(f)); } printf("\n"); }
  //for (int i = 0; i < Xs.size(); ++i) { printf("n_not_yet_covered_in[%d] = %d\n", i, n_not_yet_covered_in[i]); } printf("\n");
  return AllCombinedSubsetsOfSize(Xs, n_not_yet_covered_in, xs_wanted, 0, &xs, pred);
}

}  // namespace internal
}  // namespace limbo

#endif  // LIMBO_INTERNAL_SUBSETS_H_

