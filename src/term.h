// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <map>
#include <ostream>
#include <set>
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
  typedef uint64_t Sort;
  class Factory;

  Term() : kind_(DUMMY), id_(0) {}
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  bool operator==(const Term& t) const;
  bool operator!=(const Term& t) const;
  bool operator<(const Term& t) const;

  const Term& Substitute(const Unifier& theta) const;
  const Term& Ground(const Assignment& theta) const;
  static bool Unify(const Term& t1, const Term& t2, Unifier* theta);

  inline Id id() const { return id_; }
  inline bool sort() const { return sort_; }
  inline bool is_variable() const { return kind_ == VAR; }
  inline bool is_name() const { return kind_ == NAME; }
  inline bool is_ground() const { return kind_ != VAR; }

 protected:
  friend class Factory;

  enum Kind { DUMMY, VAR, NAME };

  Term(Kind kind, int id, Sort sort) : kind_(kind), id_(id), sort_(sort) {}

  Kind kind_;
  Id id_;
  Sort sort_;
};

class Variable : public Term {
 public:
  typedef std::map<Sort, std::set<Variable>> SortedSet;

  Variable();
  explicit Variable(const Term& t) : Term(t) { assert(is_variable()); }
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;
};

class StdName : public Term {
 public:
  typedef std::map<Sort, std::set<StdName>> SortedSet;

  StdName() : Term() {}
  explicit StdName(const Term& t) : Term(t) { assert(is_name()); }
  StdName(const StdName&) = default;
  StdName& operator=(const StdName&) = default;
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

std::ostream& operator<<(std::ostream& os, const Term& t);
std::ostream& operator<<(std::ostream& os, const Unifier& theta);
std::ostream& operator<<(std::ostream& os, const Assignment& theta);

}

#endif  // SRC_TERM_H_

