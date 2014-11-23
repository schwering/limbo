// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <set>
#include <vector>
#include "./atom.h"

namespace esbl {

class Literal : public Atom {
 public:
  struct Comparator;
  struct BySizeOnlyCompatator;
  struct BySignAndPredOnlyComparator;
  class Set;

  Literal(bool sign, const Atom& a) : Atom(a), sign_(sign) {}
  Literal(const TermSeq& z, bool sign, PredId pred, const TermSeq& args)
      : Atom(z, pred, args), sign_(sign) {}
  Literal(const Literal&) = default;
  Literal& operator=(const Literal&) = default;

  bool operator==(const Literal& l) const {
    return Atom::operator==(l) && sign_ == l.sign_;
  }

  static Literal Positive(const Atom& a) { return Literal(true, a); }
  static Literal Negative(const Atom& a) { return Literal(false, a); }

  Literal Flip() const { return Literal(!sign(), *this); }
  Literal Positive() const { return Literal(true, *this); }
  Literal Negative() const { return Literal(false, *this); }

  Literal PrependActions(const TermSeq& z) const {
    return Literal(sign(), Atom::PrependActions(z));
  }

  Literal Substitute(const Unifier& theta) const {
    return Literal(sign(), Atom::Substitute(theta));
  }

  Literal Ground(const Assignment& theta) const {
    return Literal(sign(), Atom::Ground(theta));
  }

  bool sign() const { return sign_; }

 private:
  bool sign_;
};

class SfLiteral : public Literal {
 public:
  SfLiteral(const TermSeq& z, const Term t, bool r)
      : Literal(z, r, Atom::SF, {t}) {}

  SfLiteral(const SfLiteral&) = default;
  SfLiteral& operator=(const SfLiteral&) = default;
};

struct Literal::Comparator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return comp(l1.pred(), l1.sign_, l1.z(), l1.args(),
                l2.pred(), l2.sign_, l2.z(), l2.args());
  }

  Literal LowerBound(bool sign, const PredId& p) const {
    return Literal({}, sign, p, {});
  }

  Literal UpperBound(bool sign, const PredId& p) const {
    assert(p + 1 > p);
    return Literal({}, !sign, !sign ? p : p + 1, {});
  }

 private:
  LexicographicComparator<LessComparator<PredId>,
                          LessComparator<bool>,
                          LessComparator<TermSeq>,
                          LessComparator<TermSeq>> comp;
};

struct Literal::BySizeOnlyCompatator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return comp(l1.pred(), l1.sign_, l1.z().size(), l1.args().size(),
                l2.pred(), l2.sign_, l2.z().size(), l2.args().size());
  }

 private:
  LexicographicComparator<LessComparator<PredId>,
                          LessComparator<bool>,
                          LessComparator<size_t>,
                          LessComparator<size_t>> comp;
};

struct Literal::BySignAndPredOnlyComparator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return comp(l1.pred(), l1.sign_,
                l2.pred(), l2.sign_);
  }

 private:
  LexicographicComparator<LessComparator<PredId>,
                          LessComparator<bool>> comp;
};

class Literal::Set : public std::set<Literal, Comparator> {
 public:
  using std::set<Literal, Comparator>::set;

  template<typename T>
  struct iter_range {
    iter_range(T&& first, T&& last) : first(first), last(last) {}  // NOLINT
    T first;
    T last;
    T begin() const { return first; }
    T end() const { return last; }
  };

  iter_range<iterator> range(bool sign, PredId pred) {
    return iter_range<iterator>(
        lower_bound(key_comp().LowerBound(sign, pred)),
        lower_bound(key_comp().UpperBound(sign, pred)));
  }

  iter_range<const_iterator> range(bool sign, PredId pred) const {
    return iter_range<const_iterator>(
        lower_bound(key_comp().LowerBound(sign, pred)),
        lower_bound(key_comp().UpperBound(sign, pred)));
  }
};

std::ostream& operator<<(std::ostream& os, const Literal& l);
std::ostream& operator<<(std::ostream& os, const Literal::Set& ls);

}  // namespace esbl

#endif  // SRC_LITERAL_H_

