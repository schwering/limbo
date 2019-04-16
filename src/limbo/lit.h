// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2019 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A literal is an equality or inequality of a function and a name.
// Fun, Name, Lit are trivial types and not zero-initialized implicitly.

#ifndef LIMBO_LIT_H_
#define LIMBO_LIT_H_

#include <cassert>
#include <type_traits>

#include <limbo/internal/ints.h>

namespace limbo {

class Fun {
 public:
  using id_t = internal::u32;

  static Fun FromId(id_t id) { return Fun(id); }

  explicit Fun() = default;

  Fun(const Fun&)            = default;
  Fun& operator=(const Fun&) = default;
  Fun(Fun&&)                 = default;
  Fun& operator=(Fun&&)      = default;

  bool operator==(Fun f) const { return id_ == f.id_; }
  bool operator!=(Fun f) const { return id_ != f.id_; }
  bool operator<=(Fun f) const { return id_ <= f.id_; }
  bool operator>=(Fun f) const { return id_ >= f.id_; }
  bool operator< (Fun f) const { return id_ <  f.id_; }
  bool operator> (Fun f) const { return id_ >  f.id_; }

  explicit operator bool() const { return id_; }
  explicit operator int()  const { return id_; }
  bool null() const { return id_ == 0; }

 private:
  explicit Fun(id_t id) : id_(id) { assert(!null()); }

  id_t id_;
};

class Name {
 public:
  using id_t = Fun::id_t;

  static Name FromId(id_t id) { return Name(id); }

  explicit Name() = default;

  Name(const Name&)            = default;
  Name& operator=(const Name&) = default;
  Name(Name&&)                 = default;
  Name& operator=(Name&&)      = default;

  bool operator==(Name n) const { return id_ == n.id_; }
  bool operator!=(Name n) const { return id_ != n.id_; }
  bool operator<=(Name n) const { return id_ <= n.id_; }
  bool operator>=(Name n) const { return id_ >= n.id_; }
  bool operator< (Name n) const { return id_ <  n.id_; }
  bool operator> (Name n) const { return id_ >  n.id_; }

  explicit operator bool() const { return id_; }
  explicit operator int()  const { return id_; }
  bool null() const { return id_ == 0; }

 private:
  explicit Name(id_t id) : id_(id) { assert(!null()); }

  id_t id_;
};

class Lit {
 public:
  using id_t = internal::u64;
  using Bits = internal::Bits<Fun::id_t>;

  static Lit Eq(Fun fun, Name name) { return Lit(true, fun, name); }
  static Lit Neq(Fun fun, Name name) { return Lit(false, fun, name); }
  static Lit FromId(id_t id) { return Lit(id); }

  explicit Lit() = default;
  explicit Lit(bool pos, Fun fun, Name name) : Lit(Bits::interleave(int(fun), (int(name) << 1) | pos)) {}

  Lit(const Lit&)            = default;
  Lit& operator=(const Lit&) = default;
  Lit(Lit&&)                 = default;
  Lit& operator=(Lit&&)      = default;

  bool pos()  const { return id_ & 1; }
  bool neg()  const { return !pos(); }
  Fun  fun()  const { return Fun::FromId(Bits::deinterleave_hi(id_)); }
  Name name() const { return Name::FromId(Bits::deinterleave_lo(id_) >> 1); }

  explicit operator bool() const { return id_; }
  explicit operator int() const { return id_; }
  bool null() const { return id_ == 0; }

  Lit flip() const { return Lit(id_ ^ 1); }

  bool operator==(Lit a) const { return id_ == a.id_; }
  bool operator!=(Lit a) const { return id_ != a.id_; }
  bool operator<=(Lit a) const { return id_ <= a.id_; }
  bool operator>=(Lit a) const { return id_ >= a.id_; }
  bool operator<(Lit a)  const { return id_ < a.id_; }
  bool operator>(Lit a)  const { return id_ > a.id_; }

  // Valid(a, b) holds when a, b match one of the following:
  // (f == n), (f != n)
  // (f != n), (f == n)
  // (f != n1), (f != n2) for distinct n1, n2.
  static bool Valid(Lit a, Lit b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.neg() && b.neg() && (x & Bits::kHi) == 0);
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (f == n), (f != n)
  // (f != n), (f == n)
  // (f == n1), (f == n2) for distinct n1, n2.
  static bool Complementary(Lit a, Lit b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.pos() && b.pos() && (x & Bits::kHi) == 0);
  }

  // ProperlySubsumes(a, b) holds when a is (f == n1) and b is (f != n2) for distinct n1, n2.
  static bool ProperlySubsumes(Lit a, Lit b) {
    const id_t x = a.id_ ^ b.id_;
    return x != 1 && (x & 1) && a.pos() && (x & Bits::kHi) == 0;
  }

  // Subsumes(a, b) holds when a == b or ProperlySubsumes(a, b).
  static bool Subsumes(Lit a, Lit b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 0 || (x != 1 && (x & 1) && a.pos() && (x & Bits::kHi) == 0);
  }

  bool Subsumes(Lit b) const { return Subsumes(*this, b); }

  bool ProperlySubsumes(Lit b) const { return ProperlySubsumes(*this, b); }

 private:
  static_assert(sizeof(Fun::id_t) == sizeof(Name::id_t), "Fun::id_t and Name::id_t must be identical");
  static_assert(sizeof(Fun::id_t) + sizeof(Name::id_t) == sizeof(id_t), "Fun::id_t and Name::id_t must fit in id_t");

  explicit Lit(id_t id) : id_(id) {}

  id_t id_;
};

static_assert(std::is_trivial<Fun>::value, "Fun should be trivial type");
static_assert(std::is_trivial<Name>::value, "Name should be trivial type");
static_assert(std::is_trivial<Lit>::value, "Lit should be trivial type");

}  // namespace limbo

#endif  // LIMBO_LIT_H_

