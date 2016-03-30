// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <algorithm>
#include <forward_list>
#include <map>
#include <vector>
#include "./clause.h"

namespace lela {

class Setup {
 public:
  typedef size_t SplitLevel;

  Setup() = default;
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void Minimize() {
    for (auto it = clauses_.begin(); it != clauses_.end(); ++it) {
      for (auto jt = clauses_.begin(), prev = jt; jt != clauses_.end(); prev = jt++) {
        if (it != jt && it->Subsumes(*jt)) {
          assert(std::next(prev) == jt);
          RemoveClause(prev, *jt);
        }
      }
    }
  }

  bool PossiblyInconsistent() const {
    std::vector<Literal> ls;
    for (auto kv : mentions_) {
      ls.clear();
      Term t = kv.first;
      for (const Clause& c : kv.second) {
        for (Literal a : c) {
          if (t == a.lhs() || t == a.rhs()) {
            ls.push_back(a);
          }
        }
      }
      for (auto it = ls.begin(); it != ls.end(); ++it) {
        for (auto jt = std::next(it); jt != ls.end(); ++jt) {
          assert(Literal::Complementary(*it, *jt) == Literal::Complementary(*jt, *it));
          if (Literal::Complementary(*it, *jt)) {
            return true;
          }
        }
      }
    }
    return false;
  }

  bool Implies(const Clause& c) const {
    for (const Clause& d : clauses_) {
      if (d.Subsumes(c)) {
        return true;
      }
    }
    return false;
  }

 private:
  typedef std::forward_list<Clause> List;
  typedef std::forward_list<List::const_reference> RefList;

  void AddClause(const Clause& c) {
    clauses_.push_front(c);
    for (Literal a : c) {
      if (a.lhs().function()) {
        auto& cs = mentions_[a.lhs()];
        if (cs.empty() || c == cs.front()) {
          cs.push_front(clauses_.front());
        }
      }
    }
  }

  void RemoveClause(List::const_iterator before, List::const_reference c) {
    for (Literal a : c) {
      if (a.lhs().function()) {
        mentions_[a.lhs()].remove_if([c](const Clause& d) { return &c == &d; });
      }
      if (a.rhs().function()) {
        mentions_[a.rhs()].remove_if([c](const Clause& d) { return &c == &d; });
      }
    }
    assert(*std::next(before) == c);
    clauses_.erase_after(before);
  }

  List clauses_;
  std::map<Term, List> mentions_;
};

}  // namespace lela

#endif  // SRC_SETUP_H_

