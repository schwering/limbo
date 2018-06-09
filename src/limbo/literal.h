// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A literal is an equality or inequality of a function and a object.

#ifndef LIMBO_LITERAL_H_
#define LIMBO_LITERAL_H_

#include <cassert>

#include <limbo/internal/ints.h>

namespace limbo {

class Fun {
 public:
  using id_t = internal::u32;

  Fun() = default;
  explicit Fun(id_t id) : id_(id) { assert(!null()); }

  bool operator==(Fun f) const { return id_ == f.id_; }
  bool operator!=(Fun f) const { return id_ != f.id_; }
  bool operator<=(Fun f) const { return id_ <= f.id_; }
  bool operator>=(Fun f) const { return id_ >= f.id_; }
  bool operator<(Fun f)  const { return id_ < f.id_; }
  bool operator>(Fun f)  const { return id_ > f.id_; }

  bool null() const { return id_ == 0; }
  int index() const { return id_; }

 private:
  id_t id_;
};

class Name {
 public:
  using id_t = Fun::id_t;

  Name() = default;
  explicit Name(id_t id) : id_(id) { assert(!null()); }

  bool operator==(Name n) const { return id_ == n.id_; }
  bool operator!=(Name n) const { return id_ != n.id_; }
  bool operator<=(Name n) const { return id_ <= n.id_; }
  bool operator>=(Name n) const { return id_ >= n.id_; }
  bool operator<(Name n)  const { return id_ < n.id_; }
  bool operator>(Name n)  const { return id_ > n.id_; }

  bool null() const { return id_ == 0; }
  int index() const { return id_; }

 private:
  id_t id_;
};

class Literal {
 public:
  using id_t = internal::u64;

  static Literal Eq(Fun lhs, Name rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Fun lhs, Name rhs) { return Literal(false, lhs, rhs); }
  static Literal FromId(id_t id) { return Literal(id); }

  Literal() = default;

  bool pos() const { return id_ & 1; }
  bool neg() const { return !pos(); }
  Fun lhs() const { return Fun(internal::Bits<Fun::id_t>::deinterleave_hi(id_)); }
  Name rhs() const { return Name(internal::Bits<Name::id_t>::deinterleave_lo(id_) & ~1ull); }

  bool null() const { return id_ == 0; }

  Literal flip() const { return Literal(id_ ^ kPos); }

  bool operator==(Literal a) const { return id_ == a.id_; }
  bool operator!=(Literal a) const { return id_ != a.id_; }
  bool operator<=(Literal a) const { return id_ <= a.id_; }
  bool operator>=(Literal a) const { return id_ >= a.id_; }
  bool operator<(Literal a)  const { return id_ < a.id_; }
  bool operator>(Literal a)  const { return id_ > a.id_; }

  // Valid(a, b) holds when a, b match one of the following:
  // (f == n), (f != n)
  // (f != n), (f == n)
  // (f != n1), (f != n2) for distinct n1, n2.
  static bool Valid(const Literal a, const Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.neg() && b.neg() && internal::Bits<Fun::id_t>::deinterleave_hi(x) == 0);
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (f == n), (f != n)
  // (f != n), (f == n)
  // (f == n1), (f == n2) for distinct n1, n2.
  static bool Complementary(const Literal a, const Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.pos() && b.pos() && internal::Bits<Fun::id_t>::deinterleave_hi(x) == 0);
  }

  // ProperlySubsumes(a, b) holds when a is (f == n1) and b is (f != n2) for distinct n1, n2.
  static bool ProperlySubsumes(Literal a, Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x != 1 && (x & 1) && a.pos() && internal::Bits<Fun::id_t>::deinterleave_hi(x) == 0;
  }

  // Subsumes(a, b) holds when a == b or ProperlySubsumes(a, b).
  static bool Subsumes(Literal a, Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 0 || (x != 1 && (x & 1) && a.pos() && internal::Bits<Fun::id_t>::deinterleave_hi(x) == 0);
  }

  bool Subsumes(Literal b) const { return Subsumes(*this, b); }

  bool ProperlySubsumes(Literal b) const { return ProperlySubsumes(*this, b); }

 private:
  static constexpr id_t kPos = static_cast<id_t>(1) << (sizeof(id_t) * 8 - 1);

  explicit Literal(id_t id) : id_(id) {}

  Literal(bool pos, Fun lhs, Name rhs) {
    id_ = internal::Bits<Fun::id_t>::interleave(lhs.index(), rhs.index());
    id_ |= pos * kPos;
  }

  id_t id_;
};

}  // namespace limbo

#endif  // LIMBO_LITERAL_H_

