// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./term.h"
#include <cassert>
#include <limits>
#include <tuple>

namespace esbl {

Variable Term::Factory::CreateVariable(Term::Sort sort) {
  return Variable(Term(Term::VAR, var_counter_++, sort));
}

StdName Term::Factory::CreateStdName(Term::Id id, Term::Sort sort) {
  assert(id >= 0);
  const StdName n(Term(Term::NAME, id, sort));
  names_[n.sort()].insert(n);
  assert(n.is_name());
  return n;
}

Term Term::Substitute(const Unifier& theta) const {
  if (is_variable()) {
    auto it = theta.find(Variable(*this));
    assert(it == theta.end() || !(it->second == *this));
    return it != theta.end() ? it->second.Substitute(theta) : *this;
  } else {
    return *this;
  }
}

Term Term::Ground(const Assignment& theta) const {
  if (is_variable()) {
    auto it = theta.find(Variable(*this));
    return it != theta.end() ? it->second : *this;
  } else {
    return *this;
  }
}

namespace {
inline bool Update(Unifier* theta, const Variable& x, const Term& t) {
  assert(x.sort() == t.sort());
  if (x == t) {
    return true;
  }
  auto p = theta->insert(std::make_pair(x, t));
  if (p.second) {
    return true;
  }
  const Term t_old = p.first->second;
  if (t.ground() && t_old.ground()) {
    return t == t_old;
  }
  return Update(theta, Variable(t_old), t);
}
}  // namespace

bool Term::Matches(const Term& t, Unifier* theta) const {
  const Term tt = t.Substitute(*theta);
  if (tt.is_variable()) {
    return Update(theta, Variable(tt), *this);
  } else {
    return tt == *this;
  }
}

bool Term::Unify(const Term& t1, const Term& t2, Unifier* theta) {
  const Term tt1 = t1.Substitute(*theta);
  const Term tt2 = t2.Substitute(*theta);
  if (tt1.is_variable() && (!tt2.is_variable() || tt1 < tt2)) {
    return Update(theta, Variable(tt1), tt2);
  } else if (tt2.is_variable() && (!tt1.is_variable() || tt2 < tt1)) {
    return Update(theta, Variable(tt2), tt1);
  } else {
    return tt1 == tt2;
  }
}

const Variable Variable::MIN(Term(Term::VAR,
                                  std::numeric_limits<Term::Id>::min(),
                                  std::numeric_limits<Term::Sort>::min()));
const Variable Variable::MAX(Term(Term::VAR,
                                  std::numeric_limits<Term::Id>::max(),
                                  std::numeric_limits<Term::Sort>::max()));

const StdName StdName::MIN_NORMAL(Term(Term::NAME, 0,
                                       std::numeric_limits<Term::Sort>::min()));
const StdName StdName::MIN(Term(Term::NAME,
                                std::numeric_limits<Term::Id>::min(),
                                std::numeric_limits<Term::Sort>::min()));
const StdName StdName::MAX(Term(Term::NAME,
                                std::numeric_limits<Term::Id>::max(),
                                std::numeric_limits<Term::Sort>::max()));

StdName StdName::next_placeholder() const {
  assert(is_placeholder());
  assert(id() < 0);
  Term::Id next_id = id() - 1;
  assert(next_id < 0);
  return StdName(Term(Term::NAME, next_id, sort()));
}

size_t StdName::Set::n_placeholders() const {
  size_t n;
  auto it = begin();
  if (it == end() || !it->is_placeholder()) {
    n = 0;
  } else {
    n = -1 * it->id();
  }
  assert(n == static_cast<size_t>(
          std::distance(begin(), lower_bound(StdName::MIN_NORMAL))));
  return n;
}

Range<StdName::Set::const_iterator>
StdName::SortedSet::lookup(Term::Sort sort) const {
  auto it = find(sort);
  if (it == end()) {
    return EmptyRange;
  }
  return MakeRange(it->second.begin(), it->second.end());
}

StdName StdName::SortedSet::AddNewPlaceholder(Term::Sort sort) {
  StdName::Set& ns = (*this)[sort];
  auto it = ns.begin();
  const StdName n =
      it == ns.end() || !it->is_placeholder()
      ? StdName(Term(Term::NAME, -1, sort))
      : it->next_placeholder();
  const auto p = ns.insert(n);
  assert(p.second);
  return n;
}

StdName::SortedSet StdName::SortedSet::WithoutPlaceholders() const {
  SortedSet ss = *this;
  for (auto& p : ss) {
    for (auto it = p.second.begin(); it != p.second.end(); ) {
      if (it->is_placeholder()) {
        it = p.second.erase(it);
      } else {
        ++it;
      }
    }
  }
  return ss;
}

Maybe<Term, TermSeq> TermSeq::SplitHead() const {
  if (empty()) {
    return Nothing;
  }
  const Term head(*begin());
  const TermSeq tail(std::next(begin()), end());
  return Just(head, tail);
}

Maybe<TermSeq, Term> TermSeq::SplitLast() const {
  if (empty()) {
    return Nothing;
  }
  const TermSeq init(begin(), std::prev(end()));
  const Term last(*std::prev(end()));
  return Just(init, last);
}

Maybe<TermSeq> TermSeq::WithoutLast(const size_t n) const {
  if (n > size()) {
    return Nothing;
  }
  const TermSeq prefix(begin(), end() - n);
  return Just(prefix);
}

TermSeq TermSeq::Substitute(const Unifier& theta) const {
  TermSeq ts = *this;
  for (Term& t : ts) {
    t = t.Substitute(theta);
  }
  return ts;
}

bool TermSeq::Matches(const TermSeq& z, Unifier* theta) const {
  if (size() != z.size()) {
    return false;
  }
  for (auto i = begin(), j = z.begin();
       i != end() && j != z.end();
       ++i, ++j) {
    const Term& t1 = *i;
    const Term& t2 = *j;
    const bool r = t1.Matches(t2, theta);
    if (!r) {
      return false;
    }
  }
  return true;
}

bool TermSeq::Unify(const TermSeq& z1, const TermSeq& z2, Unifier* theta) {
  if (z1.size() != z2.size()) {
    return false;
  }
  for (auto i = z1.begin(), j = z2.begin();
       i != z1.end() && j != z2.end();
       ++i, ++j) {
    const Term& t1 = *i;
    const Term& t2 = *j;
    const bool r = Term::Unify(t1, t2, theta);
    if (!r) {
      return false;
    }
  }
  return true;
}

void TermSeq::CollectVariables(Variable::Set* vs) const {
  for (const Term& t : *this) {
    if (t.is_variable()) {
      (*vs).insert(Variable(t));
    }
  }
}

void TermSeq::CollectVariables(Variable::SortedSet* vs) const {
  for (const Term& t : *this) {
    if (t.is_variable()) {
      (*vs)[t.sort()].insert(Variable(t));
    }
  }
}

void TermSeq::CollectNames(StdName::SortedSet* ns) const {
  for (const Term& t : *this) {
    if (t.is_name()) {
      (*ns)[t.sort()].insert(StdName(t));
    }
  }
}

bool TermSeq::ground() const {
  for (const Term& t : *this) {
    if (!t.ground()) {
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& os, const Term& t) {
  char c;
  if (t.is_variable()) {
    c = 'X';
  } else if (t.is_name()) {
    c = '#';
  } else {
    c = '?';
  }
  return os << c << t.id();
}

std::ostream& operator<<(std::ostream& os, const TermSeq& z) {
  for (auto it = z.begin(); it != z.end(); ++it) {
    if (it != z.begin()) {
      os << ", ";
    }
    os << *it;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Unifier& theta) {
  os << '{';
  for (auto it = theta.begin(); it != theta.end(); ++it) {
    if (it != theta.begin()) {
      os << ',' << ' ';
    }
    os << it->first << " : " << it->second;
  }
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Assignment& theta) {
  os << '{';
  for (auto it = theta.begin(); it != theta.end(); ++it) {
    if (it != theta.begin()) {
      os << ',' << ' ';
    }
    os << it->first << " : " << it->second;
  }
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, const StdName::SortedSet& ns) {
  for (const auto& p : ns) {
    os << p.first << ": ";
    for (const auto& q : p.second) {
      os << q << " ";
    }
    os << std::endl;
  }
  return os;
}

}  // namespace esbl

