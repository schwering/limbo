// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_QUERY_H_
#define SRC_QUERY_H_

#include <memory>
#include "./setup.h"
#include "./term.h"

namespace esbl {

class Query {
 public:
  Query(const Query&) = delete;
  Query& operator=(const Query&) = delete;
  virtual ~Query() {}

  static std::unique_ptr<Query> Eq(const Term& t1, const Term& t2);
  static std::unique_ptr<Query> Neq(const Term& t1, const Term& t2);
  static std::unique_ptr<Query> Lit(const Literal& l);
  static std::unique_ptr<Query> Or(std::unique_ptr<Query> q1,
                                   std::unique_ptr<Query> q2);
  static std::unique_ptr<Query> And(std::unique_ptr<Query> q1,
                                    std::unique_ptr<Query> q2);
  static std::unique_ptr<Query> Neg(std::unique_ptr<Query> q);
  static std::unique_ptr<Query> Act(const Term& a, std::unique_ptr<Query> q);
  static std::unique_ptr<Query> Exists(const Variable& x,
                                       std::unique_ptr<Query> q);
  static std::unique_ptr<Query> Forall(const Variable& x,
                                       std::unique_ptr<Query> q);

  std::vector<SimpleClause> Clauses(const StdName::SortedSet& hplus) const;

  template<class T>
  bool Entailed(T* setup, typename T::split_level k) const;

 private:
  struct Equal;
  struct Lit;
  struct Junction;
  struct Action;
  struct Quantifier;

  struct Cnf {
   public:
    struct C {
      static C Concat(const C& c1, const C& c2);
      C Substitute(const Unifier& theta) const;
      bool VacuouslyTrue() const;

      std::vector<std::pair<Term, Term>> eqs;
      std::vector<std::pair<Term, Term>> neqs;
      SimpleClause clause;
    };

    Cnf() = default;
    Cnf(const Cnf&) = default;
    Cnf& operator=(const Cnf&) = default;

    Cnf Substitute(const Unifier& theta) const;
    Cnf And(const Cnf& c) const;
    Cnf Or(const Cnf& c) const;
    std::vector<SimpleClause> UnsatisfiedClauses() const;

    std::vector<C> cs;
    int n_vars = 0;
  };

  Query() = default;

  virtual Cnf MakeCnf(const StdName::SortedSet& hplus) const = 0;
  virtual void Negate() = 0;
};

template<class T>
bool Query::Entailed(T* setup, typename T::split_level k) const {
  for (SimpleClause c : Clauses(setup->hplus())) {
    if (!setup->Entailed(c, k)) {
      return false;
    }
  }
  return true;
}

}  // namespace esbl

#endif  // SRC_QUERY_H_

