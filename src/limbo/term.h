// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A Term is either a function or a name.

#ifndef LIMBO_TERM_H_
#define LIMBO_TERM_H_

#include <cassert>

#include <limbo/internal/ints.h>

namespace limbo {

class Term {
 public:
  using id_t = internal::u32;

  static Term CreateName(int i) { return Term((i << 1) | 0); }
  static Term CreateFunc(int i) { return Term((i << 1) | 1); }
  static Term FromId(id_t id) { return Term(id); }

  Term() = default;

  bool operator==(Term t) const { return id_ == t.id_; }
  bool operator!=(Term t) const { return id_ != t.id_; }
  bool operator<=(Term t) const { return id_ <= t.id_; }
  bool operator>=(Term t) const { return id_ >= t.id_; }
  bool operator<(Term t)  const { return id_ < t.id_; }
  bool operator>(Term t)  const { return id_ > t.id_; }

  id_t id()     const { return id_; }
  int index() const { return id_ >> 1; }

  bool null() const { return (id_ >> 1) == 0; }
  bool name() const { return (id_ & 1) == 0; }
  bool func() const { return !name(); }

 private:
  explicit Term(id_t id) : id_(id) { assert(!null()); }

  id_t id_;
};

}  // namespace limbo

#endif  // LIMBO_TERM_H_

