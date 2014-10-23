// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <map>
#include <set>

class Term {
 public:
  typedef uint64_t Id;
  typedef uint64_t Sort;

  class Var;
  class Name;

  typedef std::set<Var> VarSet;
  typedef std::set<Name> NameSet;
  typedef std::map<Var, Term> Unifier;
  typedef std::map<Var, Name> Assignment;

  static Var CreateVariable(Sort sort);
  static Name CreateStdName(Id id, Sort sort);

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
  inline bool is_ground() const { return type_ == VAR; }

 protected:
  enum Type { DUMMY, VAR, NAME };

  static Id var_id_;

  Term(Type type, int id, Sort sort);

  Type type_;
  int id_;
  Sort sort_;
};

class Term::Var : public Term {
 public:
  Var();
  explicit Var(const Term& t) : Term(t) { assert(is_variable()); }
  Var(const Var&) = default;
  Var& operator=(const Var&) = default;
};

class Term::Name : public Term {
 public:
  Name() : Term() {}
  explicit Name(const Term& t) : Term(t) { assert(is_name()); }
  Name(const Name&) = default;
  Name& operator=(const Name&) = default;
};

#endif  // SRC_TERM_H_

