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
#include <cstdint>

#include <algorithm>
#include <functional>
#include <utility>

#include <lela/term.h>
#include <lela/internal/compar.h>

namespace lela {

class Literal {
 public:
  struct LhsHasher;

  static Literal Eq(Term lhs, Term rhs) { return Literal(true, lhs, rhs); }
  static Literal Neq(Term lhs, Term rhs) { return Literal(false, lhs, rhs); }

  Literal() = default;

  Term lhs() const { return Term(data_ & ~std::uint32_t(0)); }
  bool pos() const { return (data_ >> 63) & 1; }
  Term rhs() const { return Term(std::uint32_t(data_ >> 32) & ~(std::uint32_t(1) << 31)); }

  bool null()           const { return data_ == 0; }
  bool ground()         const { return lhs().ground() && rhs().ground(); }
  bool primitive()      const { return lhs().primitive() && rhs().name(); }
  bool quasiprimitive() const { return lhs().quasiprimitive() && (rhs().name() || rhs().variable()); }

  Literal flip() const { return Literal(!pos(), lhs(), rhs()); }
  Literal dual() const { return Literal(pos(), rhs(), lhs()); }

  bool operator==(Literal a) const { return pos() == a.pos() && lhs() == a.lhs() && rhs() == a.rhs(); }
  bool operator!=(Literal a) const { return !(*this == a); }

  internal::hash_t hash() const { return data_; }

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
    return a.lhs() == b.lhs() &&
        ((a.pos() != b.pos() && a.rhs() == b.rhs()) ||
         (a.pos() && b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs()));
  }

  // Subsumes(a, b) holds when a, b match one of the following:
  // (t1 = t2), (t1 = t2)
  // (t1 = n1), (t1 != n2) for distinct n1, n2.
  static bool Subsumes(Literal a, Literal b) {
    assert(a.primitive());
    assert(b.primitive());
    return a.lhs() == b.lhs() &&
        ((a.pos() == b.pos() && a.rhs() == b.rhs()) ||
         (a.pos() && !b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs()));
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

  static internal::Maybe<Term::Substitution> Bisimilar(Literal a, Literal b) {
    Term::Substitution sub;
    bool ok = Term::Bisimilar<config>(a.lhs(), b.lhs(), &sub) &&
              Term::Bisimilar<config>(a.rhs(), b.rhs(), &sub);
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
    data_ = (std::uint64_t(rhs.index()) << 32) | std::uint64_t(lhs.index()) | (std::uint64_t(pos) << 63);
    assert(this->lhs() == lhs);
    assert(this->rhs() == rhs);
    assert(this->pos() == pos);
  }

  std::uint64_t data_;
};

struct Literal::LhsHasher {
  std::size_t operator()(const Literal a) const { return a.lhs().hash(); }
};

}  // namespace lela


namespace std {

template<>
struct hash<lela::Literal> {
  std::size_t operator()(const lela::Literal a) const { return a.hash(); }
};

template<>
struct equal_to<lela::Literal> {
  bool operator()(const lela::Literal a, const lela::Literal b) const { return a == b; }
};

}  // namespace std

#endif  // LELA_LITERAL_H_

