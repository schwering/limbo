// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <map>
#include <ostream>
#include <set>
#include <utility>
#include <vector>
#include "./maybe.h"
#include "./range.h"

namespace esbl {

class Term;
class Variable;
class StdName;
class TermSeq;

typedef std::map<Variable, Term> Unifier;
typedef std::map<Variable, StdName> Assignment;

class Term {
 public:
  typedef std::set<Term> Set;
  typedef int Id;
  typedef int Sort;
  class Factory;

  Term() = default;
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  bool operator==(const Term& t) const {
    return kind_ == t.kind_ && id_ == t.id_ && sort_ == t.sort_;
  }
  bool operator<(const Term& t) const {
    return std::tie(kind_, id_, sort_) < std::tie(t.kind_, t.id_, t.sort_);
  }

  Term Substitute(const Unifier& theta) const;
  Term Ground(const Assignment& theta) const;
  bool Matches(const Term& t, Unifier* theta) const;
  static bool Unify(const Term& t1, const Term& t2, Unifier* theta);

  Id id() const { return id_; }
  bool sort() const { return sort_; }
  bool ground() const { return kind_ != VAR; }
  bool is_variable() const { return kind_ == VAR; }
  bool is_name() const { return kind_ == NAME; }

 protected:
  friend class Factory;
  friend class Variable;
  friend class StdName;

  enum Kind { DUMMY, VAR, NAME };

  Term(Kind kind, Id id, Sort sort) : kind_(kind), id_(id), sort_(sort) {}

  Kind kind_ = DUMMY;
  Id id_ = 0;
  Sort sort_ = 0;
};

class Variable {
 public:
  typedef std::set<Variable> Set;
  typedef std::map<Term::Sort, std::set<Variable>> SortedSet;

  static const Variable MIN;
  static const Variable MAX;

  Variable() = default;
  explicit Variable(const Term& t) : t_(t) { assert(t_.is_variable()); }
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;

  bool operator==(const Term& t) const { return t_ == t; }
  bool operator<(const Term& t) const { return t_ < t; }

  operator Term&() { return t_; }
  operator const Term&() const { return t_; }

  Term Substitute(const Unifier& theta) const { return t_.Substitute(theta); }
  Term Ground(const Assignment& theta) const { return t_.Ground(theta); }

  Term::Id id() const { return t_.id_; }
  bool sort() const { return t_.sort_; }
  bool ground() const { return t_.ground(); }
  bool is_variable() const { return t_.is_variable(); }
  bool is_name() const { return t_.is_name(); }

 private:
  Term t_;
};

class StdName {
 public:
  class Set;
  class SortedSet;

  static const StdName MIN_NORMAL;
  static const StdName MIN;
  static const StdName MAX;

  StdName() = default;
  explicit StdName(const Term& t) : t_(t) { assert(t_.is_name()); }
  StdName(const StdName&) = default;
  StdName& operator=(const StdName&) = default;

  bool operator==(const Term& t) const { return t_ == t; }
  bool operator<(const Term& t) const { return t_ < t; }

  operator Term&() { return t_; }
  operator const Term&() const { return t_; }

  Term Substitute(const Unifier& theta) const { return t_.Substitute(theta); }
  Term Ground(const Assignment& theta) const { return t_.Ground(theta); }

  Term::Id id() const { return t_.id_; }
  bool sort() const { return t_.sort_; }
  bool ground() const { return t_.ground(); }
  bool is_variable() const { return t_.is_variable(); }
  bool is_name() const { return t_.is_name(); }
  bool is_placeholder() const { return t_.id_ < MIN_NORMAL.t_.id_; }

 private:
  StdName next_placeholder() const;

  Term t_;
};

class StdName::Set : public std::set<StdName> {
 public:
  using std::set<StdName>::set;

  size_t n_placeholders() const;
};

class StdName::SortedSet : public std::map<Term::Sort, StdName::Set> {
 public:
  using std::map<Term::Sort, StdName::Set>::map;

  Range<StdName::Set::const_iterator> lookup(Term::Sort sort) const;
  StdName AddNewPlaceholder(Term::Sort sort);
  SortedSet WithoutPlaceholders() const;
};

class Term::Factory {
 public:
  Factory() : var_counter_(0) {}
  Factory(const Factory&) = delete;
  Factory& operator=(const Factory&) = delete;

  Variable CreateVariable(Term::Sort sort);
  StdName CreateStdName(Term::Id id, Term::Sort sort);

  const StdName::SortedSet& sorted_names() const { return names_; }

 private:
  Term::Id var_counter_;
  StdName::SortedSet names_;
};

class TermSeq : public std::vector<Term> {
 public:
  typedef std::set<TermSeq> Set;

  using std::vector<Term>::vector;

  Maybe<Term, TermSeq> SplitHead() const;
  Maybe<TermSeq, Term> SplitLast() const;
  Maybe<TermSeq> WithoutLast(const size_t n) const;
  TermSeq Substitute(const Unifier& theta) const;
  bool Matches(const TermSeq& z, Unifier* theta) const;
  static bool Unify(const TermSeq& z1, const TermSeq& z2, Unifier* theta);

  void CollectVariables(Variable::Set* vs) const;
  void CollectVariables(Variable::SortedSet* vs) const;
  void CollectNames(StdName::SortedSet* ns) const;

  bool ground() const;
};

std::ostream& operator<<(std::ostream& os, const Term& t);
std::ostream& operator<<(std::ostream& os, const TermSeq& z);
std::ostream& operator<<(std::ostream& os, const Unifier& theta);
std::ostream& operator<<(std::ostream& os, const Assignment& theta);
std::ostream& operator<<(std::ostream& os, const StdName::SortedSet& ns);

}  // namespace esbl

#endif  // SRC_TERM_H_

