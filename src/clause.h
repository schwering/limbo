// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_CLAUSE_H_
#define SRC_CLAUSE_H_

#include <cassert>
#include <deque>
#include <list>
#include <set>
#include <utility>
#include "./compar.h"
#include "./ewff.h"
#include "./literal.h"
#include "./maybe.h"

namespace esbl {

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

  SimpleClause PrependActions(const TermSeq& z) const;

  SimpleClause Substitute(const Unifier& theta) const;
  SimpleClause Ground(const Assignment& theta) const;
  std::list<Unifier> Subsumes(const SimpleClause& c) const;

  std::list<SimpleClause> Instances(const StdName::SortedSet& hplus) const;

  Atom::Set Sensings() const;

  Variable::Set Variables() const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

  bool ground() const;

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
      : Clause(false, e, ls) {}
  Clause(bool box, const Ewff& e, const SimpleClause& ls)
      : box_(box), e_(e), ls_(ls) { e_.RestrictVariable(ls_.Variables()); }
  Clause(const Clause&) = default;
  Clause& operator=(const Clause&) = default;

  bool operator==(const Clause& c) const {
    return ls_ == c.ls_ && box_ == c.box_ && e_ == c.e_;
  }

  Clause InstantiateBox(const TermSeq& z) const;

  void Rel(const StdName::SortedSet& hplus,
           const Literal& ext_l,
           std::deque<Literal>* rel) const;
  bool Subsumes(const Clause& c) const;
  bool SplitRelevant(const Atom& a, const Clause& c, int k) const;
  static size_t ResolveWrt(const Clause& c1, const Clause& c2, const Atom& a,
                           Clause::Set* rs);

  bool box() const { return box_; }
  const Ewff& ewff() const { return e_; }
  const SimpleClause& literals() const { return ls_; }
  bool is_empty() const { return ls_.size() == 0; }
  bool is_unit() const { return ls_.size() == 1; }

  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

 private:
  Maybe<Clause> Substitute(const Unifier& theta) const;
  Maybe<Unifier, Clause> Unify(const Atom& cl_a, const Atom& ext_a) const;
  static Maybe<Clause> ResolveWrt(const Clause& c1, const Clause& c2,
                                  const Literal& l1, const Literal& l2);

  bool box_;
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
    return comp(c.ls_, c.box_, c.e_, d.ls_, d.box_, d.e_);
  }

 private:
  LexicographicComparator<SimpleClause::Comparator,
                          LessComparator<bool>,
                          Ewff::Comparator> comp;
};

class Clause::Set : public std::set<Clause, Comparator> {
 public:
  using std::set<Clause, Comparator>::set;

  const_iterator first_unit() const {
    auto it = begin();
    if (it != end() && it->is_empty()) {
      ++it;
    }
    if (it == end() || !it->is_unit()) {
      return end();
    }
    return it;
  }
};

std::ostream& operator<<(std::ostream& os, const SimpleClause& c);
std::ostream& operator<<(std::ostream& os, const Clause& c);
std::ostream& operator<<(std::ostream& os, const Clause::Set& cs);

}  // namespace esbl

#endif  // SRC_CLAUSE_H_

