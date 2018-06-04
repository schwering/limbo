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
  using id_t = internal::u64;

  static Literal Eq(Term lhs, Term rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Term lhs, Term rhs) { return Literal(false, lhs, rhs); }
  static Literal FromId(id_t id) { return Literal(id); }

  Literal() = default;

  bool pos() const { return id_ & 1; }
  bool neg() const { return !pos(); }
  Term lhs() const { return Term::FromId(internal::Bits<Term::id_t>::deinterleave_hi(id_)); }
  Term rhs() const { return Term::FromId(internal::Bits<Term::id_t>::deinterleave_lo(id_) & ~1ull); }

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
  // (t == n), (t != n)
  // (t != n), (t == n)
  // (t != n1), (t != n2) for distinct n1, n2.
  static bool Valid(const Literal a, const Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.neg() && b.neg() && internal::Bits<Term::id_t>::deinterleave_hi(x) == 0);
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (t == n), (t != n)
  // (t != n), (t == n)
  // (t == n1), (t == n2) for distinct n1, n2.
  static bool Complementary(const Literal a, const Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 1 || (x != 0 && a.pos() && b.pos() && internal::Bits<Term::id_t>::deinterleave_hi(x) == 0);
  }

  // ProperlySubsumes(a, b) holds when a is (t == n1) and b is (t != n2) for distinct n1, n2.
  static bool ProperlySubsumes(Literal a, Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x != 1 && (x & 1) && a.pos() && internal::Bits<Term::id_t>::deinterleave_hi(x) == 0;
  }

  // Subsumes(a, b) holds when a == b or ProperlySubsumes(a, b).
  static bool Subsumes(Literal a, Literal b) {
    const id_t x = a.id_ ^ b.id_;
    return x == 0 || (x != 1 && (x & 1) && a.pos() && internal::Bits<Term::id_t>::deinterleave_hi(x) == 0);
  }

  bool Subsumes(Literal b) const { return Subsumes(*this, b); }

  bool ProperlySubsumes(Literal b) const { return ProperlySubsumes(*this, b); }

 private:
  explicit Literal(id_t id) : id_(id) {}

  Literal(bool pos, Term lhs, Term rhs) {
    assert(rhs.name());
    id_ = internal::Bits<Term::id_t>::interleave(lhs.id(), rhs.id());
    assert(!(id_ & 1));
    id_ |= pos;
    assert(this->pos() == pos && this->lhs() == lhs && this->rhs() == rhs);
  }

  id_t id_;
};

}  // namespace limbo

#endif  // LIMBO_LITERAL_H_

