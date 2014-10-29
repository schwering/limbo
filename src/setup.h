// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <set>
#include "./clause.h"

namespace esbl {

class Setup {
 public:
  Setup() = default;
  Setup(std::initializer_list<Clause> cs) : cs_(cs) {}
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  std::set<Literal> Rel(const StdName::SortedSet& hplus,
                        const SimpleClause& c) const;
  std::set<Atom> Pel(const StdName::SortedSet& hplus,
                     const SimpleClause& c) const;

  const std:: set<Clause> clauses() const { return cs_; }
  Variable::SortedSet variables() const;
  StdName::SortedSet names() const;

 private:
  std::set<Clause> UnitClauses() const;
  bool ContainsEmptyClause() const;
  void PropagateUnits();
  void Minimize();

  std::set<Clause> cs_;
};

std::ostream& operator<<(std::ostream& os, const Setup& s);

}  // namespace esbl

#endif  // SRC_SETUP_H_

