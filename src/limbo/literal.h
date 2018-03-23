// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A literal is an (in)equality expression of two terms. Literals are immutable.
// If one of either terms in a literal is a function, then the left-hand side
// is a function.
//
// The most important operations are Complementary() and [Properly]Subsumes()
// checks, which are only defined for primitive literals. Note that the
// operations PropagateUnit() and Subsumes() from the Clause class use hashing to
// speed them up and therefore depend on their inner workings. In other words:
// when you modify them, double-check with the Clause class.
//
// Literal is a friend class of Term and builds on Term's memory layout. This
// makes Literals very lightweight for fast copying and comparing of Literals.

#ifndef LIMBO_LITERAL_H_
#define LIMBO_LITERAL_H_

#include <cassert>

#include <functional>
#include <utility>

#include <limbo/term.h>

#include <limbo/internal/ints.h>
#include <limbo/internal/maybe.h>

namespace limbo {

class Literal {
 public:
  template<typename T>
  using Maybe = internal::Maybe<T>;
  struct LhsHash;

  static Literal Eq(Term lhs, Term rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Term lhs, Term rhs) { return Literal(false, lhs, rhs); }

  Literal() = default;

  Term lhs() const { return Term(static_cast<Term::Id>((id_ & kBitMaskLhs) >> kFirstBitLhs)); }
  bool pos() const { return (id_ & kBitMaskPos) != 0; }
  Term rhs() const { return Term(static_cast<Term::Id>((id_ & kBitMaskRhs) >> kFirstBitRhs)); }

  bool null()            const { return id_ == 0; }
  bool trivial()         const { return lhs().name() && rhs().name(); }
  bool primitive()       const { return lhs().primitive() && rhs().name(); }
  bool quasi_trivial()   const { return lhs().quasi_name() && rhs().quasi_name(); }
  bool quasi_primitive() const { return lhs().quasi_primitive() && rhs().quasi_name(); }
  bool well_formed()     const { return trivial() || primitive() || quasi_trivial() || quasi_primitive(); }
  bool ground()          const { return rhs().ground() && lhs().ground(); }

  Literal flip() const { return Literal(id_ ^ kBitMaskPos); }
  Literal dual() const { return Literal(pos(), rhs(), lhs()); }

  bool operator==(Literal a) const { return pos() == a.pos() && lhs() == a.lhs() && rhs() == a.rhs(); }
  bool operator!=(Literal a) const { return !(*this == a); }
  bool operator<(Literal a) const { return lhs() < a.lhs() || (lhs() == a.lhs() && id_ < a.id_); }

  static Literal Min(Term lhs) { return Literal(lhs); }

  internal::hash32_t hash() const {
    return internal::jenkins_hash(static_cast<Term::Id>(id_ >> 32)) ^
           internal::jenkins_hash(static_cast<Term::Id>(id_));
  }

  // valid() holds for (t = t) and (n1 != n2) and (t1 != t2) if t1, t2 have different sorts.
  bool valid() const {
    return (pos() && lhs() == rhs()) ||
           (!pos() && lhs().name() && rhs().name() && lhs() != rhs()) ||
           (!pos() && lhs().sort() != rhs().sort());
  }

  // unsatisfiable() holds for (t != t) and (n1 = n2) and (t1 = t2) if t1, t2 have different sorts.
  bool unsatisfiable() const {
    return (!pos() && lhs() == rhs()) ||
           (pos() && lhs().name() && rhs().name() && lhs() != rhs()) ||
           (pos() && lhs().sort() != rhs().sort());
  }

  // Valid(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 != t2)
  // (t1 != t2), (t1 = t2)
  // (t1 != n1), (t1 != n2) for distinct n1, n2.
  static bool Valid(const Literal a, const Literal b) {
    return (a.lhs() == b.lhs() && a.pos() != b.pos() && a.rhs() == b.rhs()) ||
           (a.lhs() == b.lhs() && !a.pos() && !b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs());
  }

  // Complementary(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 != t2)
  // (t1 != t2), (t1 = t2)
  // (t = n1), (t = n2) for distinct n1, n2.
  static bool Complementary(const Literal a, const Literal b) {
    return (a.lhs() == b.lhs() && a.pos() != b.pos() && a.rhs() == b.rhs()) ||
           (a.lhs() == b.lhs() && a.pos() && b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs());
  }

  // ProperlySubsumes(a, b) holds when a is (t1 = n1) and b is (t1 != n2) for distinct n1, n2.
  static bool ProperlySubsumes(Literal a, Literal b) {
    return a.lhs() == b.lhs() && a.pos() && !b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs();
  }

  static bool Subsumes(Literal a, Literal b) { return a == b || ProperlySubsumes(a, b); }

  bool Subsumes(Literal b) const { return Subsumes(*this, b); }

  bool ProperlySubsumes(Literal b) const { return ProperlySubsumes(*this, b); }

  template<typename UnaryFunction>
  Literal Substitute(UnaryFunction theta, Term::Factory* tf) const {
    return Literal(pos(), lhs().Substitute(theta, tf), rhs().Substitute(theta, tf));
  }

  template<Term::UnificationConfiguration config = Term::kDefaultConfig>
  static Maybe<Term::Substitution> Unify(Literal a, Literal b) {
    Term::Substitution sub;
    bool ok = Term::Unify<config>(a.lhs(), b.lhs(), &sub) &&
              Term::Unify<config>(a.rhs(), b.rhs(), &sub);
    return ok ? internal::Just(sub) : internal::Nothing;
  }

  static Maybe<Term::Substitution> Isomorphic(Literal a, Literal b) {
    Term::Substitution sub;
    bool ok = Term::Isomorphic(a.lhs(), b.lhs(), &sub);
    if (ok) {
      if (a.rhs() == b.rhs()) {
        sub.Add(a.rhs(), b.rhs());
        ok = true;
      } else {
        const Maybe<Term> ar = sub(a.rhs());
        ok = ar && ar == sub(b.rhs());
      }
    }
    return ok ? internal::Just(sub) : internal::Nothing;
  }

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const {
    lhs().Traverse(f);
    rhs().Traverse(f);
  }

 private:
  typedef internal::u64 Id;

  // Lhs should occupy first bits so "lhs = null" is the minimum wrt operator< for all Literals with lhs.
  static constexpr Id kFirstBitPos = sizeof(Id) * 8 - 1;
  static constexpr Id kFirstBitLhs = 0;
  static constexpr Id kFirstBitRhs = sizeof(Term::Id) * 8;
  static constexpr Id kBitMaskPos  = 1UL << kFirstBitPos;
  static constexpr Id kBitMaskLhs  = ((1UL << (sizeof(Term::Id) * 8 - 1)) - 1) << kFirstBitLhs;
  static constexpr Id kBitMaskRhs  = ((1UL << (sizeof(Term::Id) * 8 - 1)) - 1) << kFirstBitRhs;

  explicit Literal(Id id) : id_(id) {}

  explicit Literal(Term lhs) {
    id_ = static_cast<Id>(lhs.id()) << kFirstBitLhs;
    assert(this->lhs() == lhs);
    assert(this->rhs().null());
    assert(!this->pos());
  }

  Literal(bool pos, Term lhs, Term rhs) {
    assert(!lhs.null());
    assert(!rhs.null());
    if (!(lhs < rhs)) {
      Term tmp = lhs;
      lhs = rhs;
      rhs = tmp;
    }
    if ((!lhs.function() && rhs.function()) || rhs.quasi_primitive()) {
      Term tmp = lhs;
      lhs = rhs;
      rhs = tmp;
    }
    assert(!rhs.function() || lhs.function());
    id_ = (static_cast<Id>(lhs.id()) << kFirstBitLhs) |
            (static_cast<Id>(pos)      << kFirstBitPos) |
            (static_cast<Id>(rhs.id()) << kFirstBitRhs);
    assert(this->lhs() == lhs);
    assert(this->rhs() == rhs);
    assert(this->pos() == pos);
  }

  Id id_;
};

struct Literal::LhsHash {
  internal::hash32_t operator()(const Literal a) const { return a.lhs().hash(); }
};

}  // namespace limbo


namespace std {

template<>
struct hash<limbo::Literal> {
  limbo::internal::hash32_t operator()(const limbo::Literal a) const { return a.hash(); }
};

template<>
struct equal_to<limbo::Literal> {
  bool operator()(const limbo::Literal a, const limbo::Literal b) const { return a == b; }
};

}  // namespace std

#endif  // LIMBO_LITERAL_H_

