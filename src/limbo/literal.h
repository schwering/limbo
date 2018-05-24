// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A literal is an equality or inequality of a function and a name or two
// names.

#ifndef LIMBO_LITERAL_H_
#define LIMBO_LITERAL_H_

#include <cassert>

#include <limbo/term.h>

#include <limbo/internal/ints.h>

namespace limbo {

class Literal {
 public:
  using u64 = internal::u64;

  static Literal Eq(Term lhs, Term rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Term lhs, Term rhs) { return Literal(false, lhs, rhs); }
  static Literal FromId(u64 id) { return Literal(id); }

  Literal() = default;

  Term lhs() const { return Term::FromId(deinterleave_lhs(id_)); }
  bool pos() const { return id_ & 1; }
  bool neg() const { return !pos(); }
  Term rhs() const { return Term::FromId(deinterleave_rhs(id_) & ~static_cast<u32>(1)); }

  bool null() const { return id_ == 0; }
  bool triv() const { return (id_ & 2) == 0; }
  bool prim() const { return !triv(); }

  Literal flip() const { return Literal(id_ ^ 1); }

  bool operator==(Literal a) const { return id_ == a.id_; }
  bool operator!=(Literal a) const { return id_ != a.id_; }
  bool operator<=(Literal a) const { return id_ <= a.id_; }
  bool operator>=(Literal a) const { return id_ >= a.id_; }
  bool operator<(Literal a)  const { return id_ < a.id_; }
  bool operator>(Literal a)  const { return id_ > a.id_; }

  // valid() holds for (n == n) and (n1 != n2).
  bool valid() const { return triv() && (pos() == (lhs() == rhs())); }

  // unsat() holds for (n != n) and (n1 == n2).
  bool unsat() const { return triv() && (pos() != (lhs() == rhs())); }

  // Valid(a, b) holds when a, b match one of the following:
  // (t1 == t2), (t1 != t2)
  // (t1 != t2), (t1 == t2)
  // (t1 != n1), (t1 != n2) for distinct n1, n2.
  static bool Valid(const Literal a, const Literal b) {
    const u64 x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.neg() && b.neg() && deinterleave_lhs(x) == 0);
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 != t2)
  // (t1 != t2), (t1 = t2)
  // (t = n1), (t = n2) for distinct n1, n2.
  static bool Complementary(const Literal a, const Literal b) {
    const u64 x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.pos() && b.pos() && deinterleave_lhs(x) == 0);
  }

  // ProperlySubsumes(a, b) holds when a is (t1 = n1) and b is (t1 != n2) for distinct n1, n2.
  static bool ProperlySubsumes(Literal a, Literal b) {
    return a.lhs() == b.lhs() && a.pos() && !b.pos() && a.rhs() != b.rhs();
  }

  static bool Subsumes(Literal a, Literal b) { return a == b || ProperlySubsumes(a, b); }

  bool Subsumes(Literal b) const { return Subsumes(*this, b); }

  bool ProperlySubsumes(Literal b) const { return ProperlySubsumes(*this, b); }

 private:
  using u32 = internal::u32;

  static u64 interleave(u32 x, u32 y) {
    return _pdep_u64(x, 0xaaaaaaaaaaaaaaaa) | _pdep_u64(y, 0x5555555555555555);
  }

  static u32 deinterleave_lhs(u64 z) {
    return _pext_u64(z, 0xaaaaaaaaaaaaaaaa);
  }

  static u32 deinterleave_rhs(u64 z) {
    return _pext_u64(z, 0x5555555555555555);
  }

  explicit Literal(u64 id) : id_(id) {}

  Literal(bool pos, Term lhs, Term rhs) {
    assert(rhs.name());
    id_ = interleave(lhs.id(), rhs.id());
    assert(!(id_ & 1));
    id_ |= pos;
  }

  u64 id_;
};

}  // namespace limbo

#endif  // LIMBO_LITERAL_H_

