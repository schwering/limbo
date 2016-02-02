// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <cassert>
#include <algorithm>
#include <set>
#include <utility>
#include "./compar.h"
#include "./literal.h"
#include "./maybe.h"

namespace lela {

class Clause {
  typedef std::vector<Literal> Vector;

  Clause() = default;
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;
  template<class InputIt> Clause(InputIt first, InputIt last) : ls_(first, last) { }

  void Minimize() {
    std::sort(ls_.begin(), ls_.end(), Literal::Comparator());
    std::remove_if(ls_.begin(), ls_.end(), [](const Literal& a) { return a.Invalid(); });
    ls_.erase(std::unique(ls_.begin(), ls_.end()), ls_.end());
  }

  bool Valid() const { return std::any_of(ls_.begin(), ls_.end(), [](const Literal& a) { return a.Valid(); }); }
  bool Invalid() const { return std::all_of(ls_.begin(), ls_.end(), [](const Literal& a) { return a.Invalid(); }); }

  bool Subsumes(const Clause& c) const {
    // Some kind of hashing might help to check if subsumption could be true.
    for (Literal a : ls_) {
      bool subsumed = false;
      for (Literal b : c.ls_) {
        subsumed = a.Subsumes(b);
      }
      if (!subsumed) {
        return false;
      }
    }
    return true;
  }

  bool Propagate(Literal a) {
    const size_t pre = ls_.size();
    std::remove_if(ls_.begin(), ls_.end(), [a](Literal b) { return Literal::Complementary(a, b); });
    const size_t post = ls_.size();
    return pre != post;
  }

  bool ground() const { return std::all_of(ls_.begin(), ls_.end(), [](Literal l) { return l.ground(); }); }
  bool primitive() const { return std::all_of(ls_.begin(), ls_.end(), [](Literal l) { return l.primitive(); }); }

  Clause Substitute(Term pre, Term post) const {
    Clause c;
    std::transform(ls_.begin(), ls_.end(), c.ls_.begin(),
                   [pre, post](Literal a) { return a.Substitute(pre, post); });
    return c;
  }

  Clause Ground(const Term::Substitution& theta) const {
    Clause c;
    std::transform(ls_.begin(), ls_.end(), c.ls_.begin(),
                   [&theta](Literal a) { return a.Ground(theta); });
    return c;
  }

 private:
  Vector ls_;
};

#if 0
class SimpleClause : public Literal::Set {
 public:
  struct Comparator;

  static const SimpleClause EMPTY;

  using Literal::Set::Set;
  static SimpleClause ResolveWrt(const SimpleClause& c1, const SimpleClause& c2,
                                 const Literal& l1, const Literal& l2);

  bool operator==(const SimpleClause& c) const {
    return std::operator==(*this, c);
  }

  SimpleClause Substitute(const Unifier& theta) const;
  SimpleClause Ground(const Assignment& theta) const;
  std::list<Unifier> Subsumes(const SimpleClause& c) const;

  std::list<SimpleClause> Instances(const StdName::SortedSet& hplus) const;

  Variable::Set Variables() const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

  bool ground() const;
  bool unit() const { return size() == 1; }

 private:
  void SubsumedBy(const const_iterator first, const const_iterator last,
                  const Unifier& theta, std::list<Unifier>* thetas) const;

  void GenerateInstances(const Variable::Set::const_iterator first,
                         const Variable::Set::const_iterator last,
                         const StdName::SortedSet& hplus,
                         Assignment* theta,
                         std::list<SimpleClause>* instances) const;
};

class Clause {
 public:
  struct Comparator;
  class Set;

  static const Clause EMPTY;

  Clause() = default;
  Clause(const Ewff& e, const SimpleClause& ls)
      : e_(e), ls_(ls) { e_.RestrictVariable(ls_.Variables()); }
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;

  bool operator==(const Clause& c) const {
    return ls_ == c.ls_ && e_ == c.e_;
  }

  void Rel(const StdName::SortedSet& hplus,
           const Literal& goal_lit,
           std::deque<Literal>* rel) const;
  bool Subsumes(const Clause& c) const;
  bool Tautologous() const;
  bool SplitRelevant(const Atom& a, const Clause& c, int k) const;
  static size_t ResolveWrt(const Clause& c1, const Clause& c2, Atom::PredId p,
                           Clause::Set* rs);

  const Ewff& ewff() const { return e_; }
  const SimpleClause& literals() const { return ls_; }
  bool empty() const { return ls_.empty(); }
  bool unit() const { return ls_.unit(); }

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  Maybe<Clause> Substitute(const Unifier& theta) const;
  static Maybe<Clause> ResolveWrt(const Clause& c1, const Clause& c2,
                                  const Literal& l1, const Literal& l2);

  Ewff e_;
  SimpleClause ls_;
};

struct SimpleClause::Comparator {
  typedef SimpleClause value_type;

  bool operator()(const SimpleClause& c, const SimpleClause& d) const {
    return comp(c.size(), c, d.size(), d);
  }

 private:
  LexicographicComparator<LessComparator<size_t>,
                          LexicographicContainerComparator<Literal::Set>> comp;
};

struct Clause::Comparator {
  typedef Clause value_type;

  bool operator()(const Clause& c, const Clause& d) const {
    return comp(c.ls_, c.e_, d.ls_, d.e_);
  }

 private:
  LexicographicComparator<SimpleClause::Comparator,
                          Ewff::Comparator> comp;
};

class Clause::Set : public std::set<Clause, Comparator> {
 public:
  using std::set<Clause, Comparator>::set;

  const_iterator first_unit() const {
    auto it = begin();
    if (it != end() && it->empty()) {
      ++it;
    }
    if (it == end() || !it->unit()) {
      return end();
    }
    return it;
  }
};

std::ostream& operator<<(std::ostream& os, const SimpleClause& c);
std::ostream& operator<<(std::ostream& os, const Clause& c);
std::ostream& operator<<(std::ostream& os, const Clause::Set& cs);
#endif

}  // namespace lela

#endif  // SRC_CLAUSE_H_

