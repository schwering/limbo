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

namespace esbl {

class Term;
class Variable;
class StdName;

typedef std::vector<Term> TermSeq;
typedef std::map<Variable, Term> Unifier;
typedef std::map<Variable, StdName> Assignment;

class Term {
 public:
  typedef uint64_t Id;
  typedef unsigned int Sort;
  class Factory;

  Term() = default;
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  bool operator==(const Term& t) const;
  bool operator!=(const Term& t) const;
  bool operator<(const Term& t) const;

  const Term& Substitute(const Unifier& theta) const;
  Term Ground(const Assignment& theta) const;
  static bool Unify(const Term& t1, const Term& t2, Unifier* theta);
  static bool UnifySeq(const TermSeq& z1, const TermSeq& z2, Unifier* theta);

  Id id() const { return id_; }
  bool sort() const { return sort_; }
  bool is_variable() const { return kind_ == VAR; }
  bool is_name() const { return kind_ == NAME; }
  bool is_ground() const { return kind_ != VAR; }

 protected:
  friend class Factory;
  friend class Variable;
  friend class StdName;

  enum Kind { DUMMY, VAR, NAME };

  Term(Kind kind, int id, Sort sort) : kind_(kind), id_(id), sort_(sort) {}

  Kind kind_ = DUMMY;
  Id id_ = 0;
  Sort sort_ = 0;
};

class Variable {
 public:
  typedef std::map<Term::Sort, std::set<Variable>> SortedSet;

  Variable() = default;
  explicit Variable(const Term& t) : t_(t) { assert(t_.is_variable()); }
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;

  bool operator==(const Term& t) const { return t_ == t; }
  bool operator!=(const Term& t) const { return t_ != t; }
  bool operator<(const Term& t) const { return t_ < t; }

  operator Term&() { return t_; }
  operator const Term&() const { return t_; }

  const Term& Substitute(const Unifier& theta) const {
    return t_.Substitute(theta);
  }
  Term Ground(const Assignment& theta) const {
    return t_.Ground(theta);
  }

  inline Term::Id id() const { return t_.id_; }
  inline bool sort() const { return t_.sort_; }
  inline bool is_variable() const { return t_.kind_ == Term::VAR; }
  inline bool is_name() const { return t_.kind_ == Term::NAME; }
  inline bool is_ground() const { return t_.kind_ != Term::VAR; }

 private:
  friend class Term;

  Term t_;
};

class StdName {
 public:
  typedef std::map<Term::Sort, std::set<StdName>> SortedSet;

  StdName() = default;
  explicit StdName(const Term& t) : t_(t) { assert(t_.is_name()); }
  StdName(const StdName&) = default;
  StdName& operator=(const StdName&) = default;

  bool operator==(const Term& t) const { return t_ == t; }
  bool operator!=(const Term& t) const { return t_ != t; }
  bool operator<(const Term& t) const { return t_ < t; }

  operator Term&() { return t_; }
  operator const Term&() const { return t_; }

  const Term& Substitute(const Unifier& theta) const {
    return t_.Substitute(theta);
  }
  Term Ground(const Assignment& theta) const {
    return t_.Ground(theta);
  }

  inline Term::Id id() const { return t_.id_; }
  inline bool sort() const { return t_.sort_; }
  inline bool is_variable() const { return t_.kind_ == Term::VAR; }
  inline bool is_name() const { return t_.kind_ == Term::NAME; }
  inline bool is_ground() const { return t_.kind_ != Term::VAR; }

 private:
  friend class Term;

  Term t_;
};

class Term::Factory {
 public:
  Factory() : var_counter_(0) {}
  Factory(const Factory&) = default;
  Factory& operator=(const Factory&) = default;

  Variable CreateVariable(Term::Sort sort);
  StdName CreateStdName(Term::Id id, Term::Sort sort);

  const StdName::SortedSet& sorted_names() const { return names_; }

 private:
  Term::Id var_counter_;
  StdName::SortedSet names_;
};

template<typename T1>
constexpr std::pair<bool, T1> failed() {
  return std::make_pair(false, T1());
}

template<typename T1, typename T2>
constexpr std::tuple<bool, T1, T2> failed() {
  return std::make_tuple(false, T1(), T2());
}

std::ostream& operator<<(std::ostream& os, const TermSeq& z);
std::ostream& operator<<(std::ostream& os, const Term& t);
std::ostream& operator<<(std::ostream& os, const Unifier& theta);
std::ostream& operator<<(std::ostream& os, const Assignment& theta);

}  // namespace esbl

#endif  // SRC_TERM_H_

