// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_LITERAL_H_
#define SRC_LITERAL_H_

#include <set>
#include <vector>
#include "./atom.h"
#include "./range.h"

namespace lela {

class Literal : public Atom {
 public:
  struct Comparator;
  struct BySizeOnlyCompatator;
  struct BySignAndPredOnlyComparator;
  class Set;

  Literal(bool sign, const Atom& a) : Atom(a), sign_(sign) {}
  Literal(bool sign, PredId pred, const TermSeq& args)
      : Atom(pred, args), sign_(sign) {}
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

struct Literal::Comparator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return comp(l1.pred(), l1.sign_, l1.args(),
                l2.pred(), l2.sign_, l2.args());
  }

  Literal LowerBound(const PredId& p) const {
    return Literal(false, p, {});
  }

  Literal LowerBound(bool sign, const PredId& p) const {
    return Literal(sign, p, {});
  }

  Literal UpperBound(const PredId& p) const {
    assert(p + 1 > p);
    return Literal(false, p + 1, {});
  }

  Literal UpperBound(bool sign, const PredId& p) const {
    assert(p + 1 > p);
    return Literal(!sign, !sign ? p : p + 1, {});
  }

 private:
  LexicographicComparator<LessComparator<PredId>,
                          LessComparator<bool>,
                          LessComparator<TermSeq>> comp;
};

struct Literal::BySizeOnlyCompatator {
  typedef Literal value_type;

  bool operator()(const Literal& l1, const Literal& l2) const {
    return comp(l1.pred(), l1.sign_, l1.args().size(),
                l2.pred(), l2.sign_, l2.args().size());
  }

 private:
  LexicographicComparator<LessComparator<PredId>,
                          LessComparator<bool>,
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

  Range<const_iterator> range(PredId pred) const {
    return Range<const_iterator>(
        lower_bound(key_comp().LowerBound(pred)),
        lower_bound(key_comp().UpperBound(pred)));
  }

  Range<const_iterator> range(bool sign, PredId pred) const {
    return Range<const_iterator>(
        lower_bound(key_comp().LowerBound(sign, pred)),
        lower_bound(key_comp().UpperBound(sign, pred)));
  }
};

std::ostream& operator<<(std::ostream& os, const Literal& l);
std::ostream& operator<<(std::ostream& os, const Literal::Set& ls);

}  // namespace lela

#endif  // SRC_LITERAL_H_

