// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_COMPAR_H_
#define SRC_COMPAR_H_

#include <algorithm>

template<class T>
struct LessComparator {
  typedef T value_type;
  bool operator()(const T& t1, const T& t2) const {
    return t1 < t2;
  }
};

template<class T, class Compar = typename T::key_compare>
struct LexicographicContainerComparator {
  typedef T value_type;
  bool operator()(const T& t1, const T& t2) const {
    return std::lexicographical_compare(t1.begin(), t1.end(),
                                        t2.begin(), t2.end(),
                                        comp);
  }

 private:
  Compar comp;
};

template<class Compar, class... Compars>
struct LexicographicComparator {
  bool operator()(const typename Compar::value_type& x,
                  const typename Compars::value_type&... xs,
                  const typename Compar::value_type& y,
                  const typename Compars::value_type&... ys) const {
    if (head_comp(x, y)) {
      return true;
    }
    if (head_comp(y, x)) {
      return false;
    }
    return tail_comp(xs..., ys...);
  }

 private:
  Compar head_comp;
  LexicographicComparator<Compars...> tail_comp;
};

template<class Compar>
struct LexicographicComparator<Compar> {
  bool operator()(const typename Compar::value_type& x,
                  const typename Compar::value_type& y) const {
    return comp(x, y);
  }

 private:
  Compar comp;
};

template<class InputIt, class UnaryPredicate>
typename std::iterator_traits<InputIt>::difference_type
count_while(InputIt first, InputIt last, UnaryPredicate p) {
  typename std::iterator_traits<InputIt>::difference_type ret = 0;
  for (; first != last && p(*first); ++first) {
    ++ret;
  }
  return ret;
}

#endif  // SRC_COMPAR_H_

