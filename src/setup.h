// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de
//
// FullPel(), Rel(), Pel(), ... depend on names().
//
// There are const versions of Inconsistent() and Entails() which just perform a
// const_cast. Their behavior is not-const for two reasons: Firstly, they update
// the inconsistency cache, which is hidden from the outside, though. Secondly,
// they add names to names().
//
// Is a setup for a minimal names() inconsistent iff it is inconsistent for any
// names()?

#ifndef SRC_SETUP_H_
#define SRC_SETUP_H_

#include <set>
#include <vector>
#include "./clause.h"

namespace lela {

class Setup {
 public:
  typedef size_t split_level;

  Setup() = default;
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void AddClause(const Clause& c);
  void AddClauseWithoutConsistencyCheck(const Clause& c);
  void GuaranteeConsistency(split_level k);

  bool Inconsistent(split_level k);
  bool Models(const Clause& c, split_level k);

  bool Inconsistent(split_level k) const {
    return const_cast<Setup*>(this)->Inconsistent(k);
  }
  bool Models(const Clause& c, split_level k) const {
    return const_cast<Setup*>(this)->Models(c, k);
  }


 private:
  typedef std::vector<std::vector<Clause>> BucketMap;

  class BitMap : public std::vector<bool> {
   public:
    using std::vector<bool>::vector;
    reference operator[](size_type pos) {
      if (pos >= size()) {
        resize(pos+1, false);
      }
      return std::vector<bool>::operator[](pos);
    }
    const_reference operator[](size_type pos) const {
      return pos < size() ? std::vector<bool>::operator[](pos) : false;
    }
  };

  BucketMap cs_;
  BitMap incons_;
};

#if 0
class Setup {
 public:
  typedef size_t split_level;

  Setup() = default;
  Setup(const Setup&) = default;
  Setup& operator=(const Setup&) = default;

  void AddClause(const Clause& c);
  void AddClauseWithoutConsistencyCheck(const Clause& c);
  void GuaranteeConsistency(split_level k);

  bool Inconsistent(split_level k);
  bool Entails(const SimpleClause& c, split_level k);

  bool Inconsistent(split_level k) const {
    return const_cast<Setup*>(this)->Inconsistent(k);
  }
  bool Entails(const SimpleClause& c, split_level k) const {
    return const_cast<Setup*>(this)->Entails(c, k);
  }

  const Clause::Set& clauses() const { return cs_; }
  const StdName::SortedSet& names() const { return names_; }

 private:
  class BitMap : public std::vector<bool> {
   public:
    using std::vector<bool>::vector;

    reference operator[](size_type pos) {
      if (pos >= size()) {
        resize(pos+1, false);
      }
      return std::vector<bool>::operator[](pos);
    }

    const_reference operator[](size_type pos) const {
      return pos < size() ? std::vector<bool>::operator[](pos) : false;
    }
  };

  void UpdateHPlusFor(const StdName::SortedSet& ns);
  void UpdateHPlusFor(const Variable::SortedSet& vs);
  void UpdateHPlusFor(const SimpleClause& c);
  void UpdateHPlusFor(const Clause& c);
  Atom::Set FullPel() const;
  Literal::Set Rel(const SimpleClause& c) const;
  Atom::Set Pel(const SimpleClause& c) const;
  bool ContainsEmptyClause() const;
  size_t MinimizeWrt(Clause::Set::iterator c);
  void Minimize();
  void PropagateUnits();
  bool Subsumes(const Clause& c);
  bool SubsumesWithSplits(Atom::Set pel, const SimpleClause& c, split_level k);
  bool SplitRelevant(const Atom& a, const Clause& c, split_level k);

  Clause::Set cs_;
  Clause::Set boxes_;
  TermSeq::Set grounded_;
  BitMap incons_;
  StdName::SortedSet names_;
};

std::ostream& operator<<(std::ostream& os, const Setup& s);
#endif

}  // namespace lela

#endif  // SRC_SETUP_H_

