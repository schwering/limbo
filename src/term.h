// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <map>
#include <set>

class Term;
class Variable;
class StdName;

typedef std::map<Variable, Term> Unifier;
typedef std::map<Variable, StdName> Assignment;

class Term {
 public:
  typedef uint64_t Id;
  typedef uint64_t Sort;

  static Variable CreateVariable(Sort sort);
  static StdName CreateStdName(Id id, Sort sort);

  Term();
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  bool operator==(const Term& t) const;
  bool operator!=(const Term& t) const;
  bool operator<(const Term& t) const;

  const Term& Substitute(const Unifier& theta) const;

  inline bool get_sort() const { return sort_; }
  inline bool is_variable() const { return type_ == VAR; }
  inline bool is_name() const { return type_ == NAME; }
  inline bool is_ground() const { return type_ != VAR; }

 protected:
  enum Type { DUMMY, VAR, NAME };

  static Id var_id_;

  Term(Type type, int id, Sort sort);

  Type type_;
  int id_;
  Sort sort_;
};

class Variable : public Term {
 public:
  typedef std::set<Variable> Set;

  Variable();
  explicit Variable(const Term& t) : Term(t) { assert(is_variable()); }
  Variable(const Variable&) = default;
  Variable& operator=(const Variable&) = default;
};

class StdName : public Term {
 public:
  typedef std::set<StdName> Set;

  StdName() : Term() {}
  explicit StdName(const Term& t) : Term(t) { assert(is_name()); }
  StdName(const StdName&) = default;
  StdName& operator=(const StdName&) = default;
};

#endif  // SRC_TERM_H_

