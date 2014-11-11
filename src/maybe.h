// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_MAYBE_H_
#define SRC_MAYBE_H_

#include <utility>
#include <iostream>

namespace esbl {

class Dummy {
 public:
  Dummy() { n = 0; std::cout << "default ctor" << std::endl; }
  explicit Dummy(int i) { n = i; std::cout << "int " << i << " ctor (to " << this << ")" << std::endl; }
  Dummy(const Dummy& d) { n = d.n; std::cout << "copy " << n << " ctor (" << &d << " to " << this << ")" << std::endl; }
  Dummy& operator=(Dummy& d) { n = d.n; std::cout << "copy " << n << " assignment (" << &d << " to " << this << ")" << std::endl; return *this; }
  Dummy(Dummy&& d) { n = d.n; std::cout << "move " << n << " ctor (<ref> to " << this << ")" << std::endl; }
  Dummy& operator=(Dummy&& d) { n = d.n; std::cout << "move " << n << " assignment (<ref> to " << this << ")" << std::endl; return *this; }

  int n;
};

template<typename T>
struct Maybe {
  Maybe() : succ(false) {}
  template<typename U>
  explicit Maybe(const U& val) : succ(true), val(val) {}
  explicit Maybe(T&& val) : succ(true), val(val) {}
  Maybe(bool succ, T&& val) : succ(succ), val(val) {}

  Maybe(const Maybe&) = default;
  Maybe(Maybe&&) = default;
  Maybe& operator=(Maybe&) = default;
  Maybe& operator=(Maybe&&) = default;

  template<typename U>
  Maybe(const Maybe<U>& m) : succ(m.succ), val(m.val) {}
  template<typename U>
  Maybe(Maybe<U>&& m) : succ(m.succ), val(m.val) {}
  template<typename U>
  Maybe& operator=(Maybe<U>& m) { succ = m.succ; val = m.val; }
  template<typename U>
  Maybe& operator=(Maybe<U>&& m) { succ = m.succ; val = m.val; }

  operator bool() const { return succ; }

  bool succ;
  T val;
};

struct NothingType {
  template<typename T>
  operator Maybe<T>() const {
    return Maybe<T>();
  }
};

constexpr NothingType Nothing;

//template<typename T>
//constexpr Maybe<T> Nothing() {
//  return Maybe<T>();
//}

template<typename T>
Maybe<T> Just(T&& val) {
  return Maybe<T>(val);
}

template<typename T>
Maybe<T> Perhaps(bool succ, T&& val) {
  return Maybe<T>(succ, val);
}

}

#endif

