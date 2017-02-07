// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 Christoph Schwering
//
// A literal is an (in)equality expression of two terms. Literals are immutable.
// If one of either terms in a literal is a function, then the left-hand side
// is a function.
//
// The most important operations are Complementary() and Subsumes() checks,
// which are only defined for primitive literals. Note that the operations
// PropagateUnit() and Subsumes() from the Clause class use hashing to speed
// them up and therefore depend on their inner workings. In other words: when
// you modify them, double-check with the Clause class.
//
// Due to the memory-wise lightweight representation of terms, copying or
// comparing literals is very fast.

#ifndef LELA_LITERAL_H_
#define LELA_LITERAL_H_

#include <cassert>

#include <algorithm>
#include <functional>
#include <utility>

#include <lela/term.h>
#include <lela/internal/ints.h>

namespace lela {

class Literal {
 public:
  typedef internal::size_t size_t;
  typedef internal::u32 u32;
  typedef internal::u64 u64;
  struct LhsHash;

  static Literal Eq(Term lhs, Term rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Term lhs, Term rhs) { return Literal(false, lhs, rhs); }

  Literal() = default;

  Term lhs() const { return Term(static_cast<u32>(data_)); }
  bool pos() const { return (data_ >> 63) == 1; }
  Term rhs() const { return Term(static_cast<u32>(data_ >> 32) & ~static_cast<u32>(1 << 31)); }

  bool null()           const { return data_ == 0; }
  bool ground()         const { return lhs().ground() && rhs().ground(); }
  bool primitive()      const { return lhs().primitive() && rhs().name(); }
  bool quasiprimitive() const { return lhs().quasiprimitive() && (rhs().name() || rhs().variable()); }

  Literal flip() const { return Literal(!pos(), lhs(), rhs()); }
  Literal dual() const { return Literal(pos(), rhs(), lhs()); }

  bool operator==(Literal a) const { return pos() == a.pos() && lhs() == a.lhs() && rhs() == a.rhs(); }
  bool operator!=(Literal a) const { return !(*this == a); }

  internal::hash32_t hash() const {
    return internal::jenkins_hash(static_cast<u32>(data_ >> 32)) ^
           internal::jenkins_hash(static_cast<u32>(data_));
  }

  // valid() holds for (t = t) and (n1 != n2) and (t1 != t2) if t1, t2 have different sorts.
  bool valid() const {
    return (pos() && lhs() == rhs()) ||
           (!pos() && lhs().name() && rhs().name() && lhs() != rhs()) ||
           (!pos() && lhs().sort() != rhs().sort());
  }

  // invalid() holds for (t != t) and (n1 = n2) and (t1 = t2) if t1, t2 have different sorts.
  bool invalid() const {
    return (!pos() && lhs() == rhs()) ||
           (pos() && lhs().name() && rhs().name() && lhs() != rhs()) ||
           (pos() && lhs().sort() != rhs().sort());
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 != t2)
  // (t1 != t2), (t1 = t2)
  // (t = n1), (t = n2) for distinct n1, n2.
  static bool Complementary(Literal a, Literal b) {
    assert(a.primitive());
    assert(b.primitive());
    const u64 x = a.data_;
    const u64 y = b.data_;
    static constexpr u64 LHS = (static_cast<u64>(1) << 32) - 1;
    static constexpr u64 RHS = ~((static_cast<u64>(1) << 63) | ((static_cast<u64>(1) << 32) - 1));
    static constexpr u64 RHS_NAME = static_cast<u64>(1) << 32;
    static constexpr u64 POS = static_cast<u64>(1) << 63;
    assert(((x ^ y) == POS) ==
           (a.lhs() == b.lhs() && a.rhs() == b.rhs() && a.pos() != b.pos()));
    assert((((x ^ y) & LHS) == 0 && (x & y & POS) == POS && (x & y & RHS_NAME) == RHS_NAME && ((x ^ y) & RHS) != 0) ==
           (a.lhs() == b.lhs() && a.pos() && b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs()));
    return (x ^ y) == POS ||
           (((x ^ y) & LHS) == 0 && (x & y & POS) == POS && (x & y & RHS_NAME) == RHS_NAME && ((x ^ y) & RHS) != 0);
  }

  // Subsumes(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 = t2)
  // (t1 = n1), (t1 != n2) for distinct n1, n2.
  static bool Subsumes(Literal a, Literal b) {
    assert(a.primitive());
    assert(b.primitive());
    const u64 x = a.data_;
    const u64 y = b.data_;
    static constexpr u64 LHS = (static_cast<u64>(1) << 32) - 1;
    static constexpr u64 RHS = ~((static_cast<u64>(1) << 63) | ((static_cast<u64>(1) << 32) - 1));
    static constexpr u64 RHS_NAME = static_cast<u64>(1) << 32;
    static constexpr u64 POS = static_cast<u64>(1) << 63;
    assert(((x ^ y) == 0) ==
           (a.lhs() == b.lhs() && a.pos() == b.pos() && a.rhs() == b.rhs()));
    assert((((x ^ y) & LHS) == 0 && (x & ~y & POS) == POS && (x & y & RHS_NAME) == RHS_NAME && ((x ^ y) & RHS) != 0) ==
           (a.lhs() == b.lhs() && a.pos() && !b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs()));
    return (x ^ y) == 0 ||
           (((x ^ y) & LHS) == 0 && (x & ~y & POS) == POS && (x & y & RHS_NAME) == RHS_NAME && ((x ^ y) & RHS) != 0);
  }

  bool Subsumes(Literal b) const {
    return Subsumes(*this, b);
  }

  template<typename UnaryFunction>
  Literal Substitute(UnaryFunction theta, Term::Factory* tf) const {
    return Literal(pos(), lhs().Substitute(theta, tf), rhs().Substitute(theta, tf));
  }

  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static internal::Maybe<Term::Substitution> Unify(Literal a, Literal b) {
    Term::Substitution sub;
    bool ok = Term::Unify<config>(a.lhs(), b.lhs(), &sub) &&
              Term::Unify<config>(a.rhs(), b.rhs(), &sub);
    return ok ? internal::Just(sub) : internal::Nothing;
  }

  static internal::Maybe<Term::Substitution> Isomorphic(Literal a, Literal b) {
    Term::Substitution sub;
    bool ok = Term::Isomorphic(a.lhs(), b.lhs(), &sub) &&
              Term::Isomorphic(a.rhs(), b.rhs(), &sub);
    return ok ? internal::Just(sub) : internal::Nothing;
  }

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const {
    lhs().Traverse(f);
    rhs().Traverse(f);
  }

 private:
  Literal(bool pos, Term lhs, Term rhs) {
    assert(!lhs.null());
    assert(!rhs.null());
    if (!(lhs < rhs)) {
      Term tmp = lhs;
      lhs = rhs;
      rhs = tmp;
    }
    if (!lhs.function() && rhs.function()) {
      Term tmp = lhs;
      lhs = rhs;
      rhs = tmp;
    }
    assert(!rhs.function() || lhs.function());
    data_ = (static_cast<u64>(rhs.id()) << 32) |
             static_cast<u64>(lhs.id()) |
            (static_cast<u64>(pos) << 63);
    assert(this->lhs() == lhs);
    assert(this->rhs() == rhs);
    assert(this->pos() == pos);
  }

  u64 data_;
};

struct Literal::LhsHash {
  internal::hash32_t operator()(const Literal a) const { return a.lhs().hash(); }
};

}  // namespace lela


namespace std {

template<>
struct hash<lela::Literal> {
  size_t operator()(const lela::Literal a) const { return a.hash(); }
};

template<>
struct equal_to<lela::Literal> {
  bool operator()(const lela::Literal a, const lela::Literal b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_LITERAL_H_

