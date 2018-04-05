// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_SOLVER_H_
#define LIMBO_SOLVER_H_

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include <limbo/literal.h>
#include <limbo/term.h>

#include <limbo/internal/ints.h>
#include <limbo/format/output.h>

#include "clause.h"

namespace limbo {

template<typename T, typename Index>
struct IndexOf { Index operator()(const T t) const { return t.index(); } };

template<typename T, typename Index>
struct FromIndex { T operator()(const Index i) const { return T(i); } };

template<typename T>
struct MakeDefault { T operator()() const { return T(); } };

template<typename Key, typename Val, typename Index = int,
         typename IndexOf = IndexOf<Key, Index>,
         typename MakeDefault = MakeDefault<Val>>
class DenseMap {
 public:
  using Vec = std::vector<Val>;
  using value_type = typename Vec::value_type;
  using reference = typename Vec::reference;
  using const_reference = typename Vec::const_reference;
  using iterator = typename Vec::iterator;
  using const_iterator = typename Vec::const_iterator;

  explicit DenseMap(Index n = 0) { vec_.reserve(n); }

  bool operator==(const DenseMap& m) const { return vec_ == m.vec_; }
  bool operator!=(const DenseMap& m) const { return !(*this == m); }

  Index size() const { return vec_.size(); }

  reference operator[](Index i) {
    if (i >= size()) {
      vec_.resize(i + 1, make_default_());
    }
    return vec_[i];
  }

  const_reference operator[](Index i) const { return const_cast<DenseMap&>(*this)[i]; }

  reference operator[](Key key) { return operator[](index_of_(key)); }
  const_reference operator[](Key key) const { return operator[](index_of_(key)); }

  iterator begin() { return vec_.begin(); }
  iterator end()   { return vec_.end(); }

  const_iterator begin() const { return vec_.begin(); }
  const_iterator end()   const { return vec_.end(); }

 protected:
  IndexOf index_of_;
  MakeDefault make_default_;
  Vec vec_;
};

template<typename T, typename Less, typename Index = int,
         typename IndexOf = IndexOf<T, Index>,
         typename MakeDefault = MakeDefault<T>>
class Heap {
 public:
  explicit Heap(Less less = Less()) : less_(less) { heap_.push_back(make_default_()); }

  bool size()  const { return heap_.size() - 1; }
  bool empty() const { return heap_.size() == 1; }

  bool contains(T x) const { return index_[x] != 0; }

  void Increase(T x) {
    assert(contains(x));
    SiftUp(index_[x]);
  }

  void Insert(T x) {
    assert(!contains(x));
    Index i = heap_.size();
    heap_.push_back(x);
    index_[x] = i;
    SiftUp(i);
  }

  void Erase(T x) {
    assert(contains(x));
    const Index i = index_[x];
    heap_[i] = heap_.back();
    index_[heap_[i]] = i;
    heap_.pop_back();
    index_[x] = 0;
    if (heap_.size() > i) {
      SiftDown(i);
    }
    assert(!contains(x));
  }

  T Top() { return heap_[1]; }

 private:
  static Index left(Index i)   { return 2 * i; }
  static Index right(Index i)  { return 2 * i + 1; }
  static Index parent(Index i) { return i / 2; }

  void SiftUp(Index i) {
    assert(i > 0 && i < heap_.size());
    T x = heap_[i];
    Index p;
    while ((p = parent(i)) != 0 && less_(x, heap_[p])) {
      heap_[i] = heap_[p];
      index_[heap_[i]] = i;
      i = p;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  void SiftDown(Index i) {
    assert(i > 0 && i < heap_.size());
    T x = heap_[i];
    while (left(i) < heap_.size()) {
      const Index min_child = right(i) < heap_.size() && less_(heap_[right(i)], heap_[left(i)]) ? right(i) : left(i);
      if (!less_(heap_[min_child], x)) {
        break;
      }
      heap_[i] = heap_[min_child];
      index_[heap_[i]] = i;
      i = min_child;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  Less less_;
  MakeDefault make_default_;
  std::vector<T> heap_;
  DenseMap<T, Index, Index, IndexOf> index_;
};

class Solver {
 public:
  using uref_t = int;
  using cref_t = int;
  using level_t = int;

  static constexpr cref_t kNullRef = 0;
  static constexpr cref_t kNullLevel = 0;
  static constexpr level_t kRootLevel = 1;

  Solver() = default;
  ~Solver() { for (Clause* c : clauses_) { Clause::Delete(c); } }

  void add_extra_name(Term n) {
    assert(extra_name_[n.sort()].null());
    extra_name_[n.sort()] = n;
  }

  void AddLiteral(Literal a) {
    if (a.unsatisfiable()) {
      empty_clause_ = true;
    } else if (a.primitive() && !a.valid() && !satisfies(a)) {
      assert(!a.valid());
      Enqueue(a, kNullRef);
      Register(a);
    }
  }

  void AddClause(const std::vector<Literal>& lits) {
    Clause* c = Clause::New(lits);
    if (c->unsatisfiable()) {
      empty_clause_ = true;
    } else if (c->primitive() && !c->valid() && !satisfies(*c)) {
      AddClause(c, kNullRef);
      for (Literal a : *c) {
        Register(a);
      }
    }
  }

  const DenseMap<Term, Term>& model() const { return model_; }

  bool Solve() {
    std::vector<Literal> learnt;
    for (;;) {
      const cref_t conflict = Propagate();
      if (conflict != kNullRef) {
        if (current_level() == kRootLevel) {
          return false;
        }
        learnt.clear();
        level_t btlevel;
        Analyze(conflict, &learnt, &btlevel);
        Backtrack(btlevel);
        assert(!learnt.empty());
        if (learnt.size() == 0) {
          Enqueue(learnt.front(), kNullRef);
        } else {
          Clause* c = Clause::New(learnt);
          const cref_t cr = AddClause(c, conflict);
          if (cr != kNullRef) {
            Enqueue(learnt[0], cr);
          } else {
            Clause::Delete(c);
          }
        }
      } else if (!order_.empty()) {
        const Term f = order_.Top();
        const Term n = CandidateName(f);
        NewLevel();
        Enqueue(Literal::Eq(f, n), kNullRef);
        std::cout << current_level() << ": " << f << " = " << n << std::endl;
      } else {
        return true;
      }
    }
  }

 private:
  struct Data {  // meta data for a pair (f,n)
    Data() = default;
    Data(bool occurs) : occurs(occurs), neq(0), seen(0), level(0), reason(0)  {}
    Data(bool neq, level_t l, cref_t r) : seen(0), occurs(1), neq(neq), level(l), reason(r)  {}
    unsigned occurs :  1;  // true iff f occurs with n in added clauses or literals
    unsigned neq    :  1;  // true iff f != n has been assigned
    unsigned seen   :  1;  // flag used in Analyze() to keep track of seen trail literals
    unsigned level  : 29;  // level at which f = n or f != n was assigned
    cref_t reason;  // clause which derived f = n or f != n
  };

  struct ActivityCompare {
    explicit ActivityCompare(const DenseMap<Term, double>* a) : activity_(a) {}
    bool operator()(Term t1, Term t2) const { return (*activity_)[t1] > (*activity_)[t2]; }
   private:
    const DenseMap<Term, double>* activity_;
  };

  void Register(Literal a) {
    const Symbol::Sort s = a.lhs().sort();
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term extra_n = extra_name_[s];
    assert(!extra_n.null());
    if (!order_.contains(f)) {
      order_.Insert(f);
    }
    names_[s][n] = n;
    names_[s][extra_n] = extra_n;
    set_occurs(f, n, true);
    set_occurs(f, extra_n, true);
  }

  cref_t AddClause(Clause* c, cref_t reason) {
    assert(reason != kNullRef || !satisfies(*c));
    assert(!c->valid());
    if (c->unit()) {
      Enqueue((*c)[0], reason);
      return kNullRef;
    } else {
      const cref_t cr = clauses_.size();
      clauses_.push_back(c);
      watchers_[(*c)[0].lhs()].push_back(cr);
      watchers_[(*c)[1].lhs()].push_back(cr);
      return cr;
    }
  }

  cref_t Propagate() {
    cref_t conflict = kNullRef;
    while (trail_head_ < trail_.size()) {
      Literal a = trail_[trail_head_++];
      conflict = Propagate(a);
    }
    return conflict;
  }

  cref_t Propagate(Literal a) {
    assert(a.primitive());
    cref_t conflict = kNullRef;
    const Term f = a.lhs();
    std::vector<cref_t>& ws = watchers_[f];
    auto cr_ptr1 = ws.begin();
    auto cr_ptr2 = cr_ptr1;
    while (cr_ptr1 != ws.end()) {
      const cref_t cr = *cr_ptr1;
      Clause& c = *clauses_[cr];
      const Term f0 = c[0].lhs();
      const Term f1 = c[0].lhs();

      // remove outdated watcher, skip
      if (f0 != f && f1 != f) {
        ++cr_ptr1;
        continue;
      }
      assert(c[0].lhs() == f || c[1].lhs() == f);

      // skip if no watched literal is false or one of them clause is true
      bool w[2] = { falsifies(c[0]), falsifies(c[1]) };
      if ((!w[0] && !w[1]) || satisfies(c[0]) || satisfies(c[1])) {
        *cr_ptr2++ = *cr_ptr1++;
        continue;
      }
      assert(falsifies(c[0]) || falsifies(c[1]));

      // find new watched literals if necessary
      for (int k = 2, s = c.size(); (w[0] || w[1]) && k < s; ++k) {
        if (!falsifies(c[k])) {
          const int l = 1 - static_cast<int>(w[0]);
          if (c[k].lhs() != f && c[k].lhs() != f0 && c[k].lhs() != f1) {
            watchers_[c[k].lhs()].push_back(cr);
          }
          std::swap(c[l], c[k]);
          w[l] = false;
        }
      }
      if (c[0].lhs() != f && c[1].lhs() != f) {
        ++cr_ptr1;
      }
      assert(w[0] == falsifies(c[0]));
      assert(w[1] == falsifies(c[1]));

      // handle conflicts and/or propagated unit clauses
      if (w[0] && w[1]) {
        while (cr_ptr1 != ws.end()) {
          *cr_ptr2++ = *cr_ptr1++;
        }
        trail_head_ = trail_.size();
        conflict = cr;
      } else if (w[0] || w[1]) {
        const Literal b = c[static_cast<int>(w[0])];
        //std::cout << current_level() << ": " << b.lhs() << " = " << b.rhs() << " by propagation from " << c << std::endl;
        Enqueue(b, cr);
      }
    }
    ws.resize(cr_ptr2 - ws.begin());
    return conflict;
  }

  void Analyze(cref_t conflict, std::vector<Literal>* learnt, level_t* btlevel) {
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) { return !d.seen; }); }));
    int depth = 0;
    Literal a = Literal();
    learnt->push_back(a);
    uref_t i = trail_.size() - 1;

    std::vector<Literal> unsee;
    //std::cout << std::endl;
    do {
      //if (conflict == kNullRef) {
      //  for (Literal x : trail_) { std::cout << "Trail: " << level_of(x) << ": " << x << (reason_of(x) != kNullRef ? " caused by propagation" : "") << std::endl; }
      //}
      assert(conflict != kNullRef);
      //std::cout << "Conflict clause " << *clauses_[conflict] << std::endl;
      for (Literal b : *clauses_[conflict]) {
        level_t l;
        if (a != b && !seen(b) && (l = level_of(b)) > kRootLevel) {
          Bump(b.lhs());
          set_seen(b, true);
          unsee.push_back(b);
          if (l >= current_level()) {
            //std::cout << "Not adding " << b << std::endl;
            ++depth;
          } else {
            //std::cout << "Adding " << b << " from " << l << std::endl;
            learnt->push_back(b);
          }
        }
      }

      while (i > 0 && !seen(trail_[i])) {
        --i;
      }
      a = trail_[i];
      //std::cout << "Follow-up " << a << " depth " << depth << std::endl;
      conflict = reason_of(a);
      //set_seen(a, false);
    } while (--depth > 0);
    (*learnt)[0] = a.flip();

    if (learnt->size() == 1) {
      *btlevel = kRootLevel;
    } else {
      uref_t max = 1;
      *btlevel = level_of((*learnt)[max]);
      for (uref_t i = 2, s = learnt->size(); i != s; ++i) {
        level_t l = level_of((*learnt)[i]);
        if (*btlevel < l) {
          max = i;
          *btlevel = l;
        }
      }
      std::swap((*learnt)[1], (*learnt)[max]);
    }

    for (Literal a : unsee) {
      set_seen(a, false);
    }
    for (Literal& a : *learnt) {
      //set_seen(a, false);
      //a = a.flip();
    }
    assert(*btlevel >= kRootLevel);
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) { return !d.seen; }); }));
  }

  void NewLevel() { level_begin_.push_back(trail_.size()); }

  void Enqueue(Literal a, cref_t reason) {
    assert(a.primitive());
    assert(!falsifies(a));
    const Term f = a.lhs();
    const Term n = a.rhs();
    if (model_[f].null()) {  // f != n1 is not enqueued if f = n2
      trail_.push_back(a);
      if (a.pos()) {
        model_[f] = n;
      }
      data_[f][n] = Data(!a.pos(), current_level(), reason);
      if (a.pos() && order_.contains(f)) {
        order_.Erase(f);
      }
    }
  }

  void Backtrack(level_t l) {
    for (auto a = trail_.cbegin() + level_begin_[l], e = trail_.cend(); a < e; ++a) {
      const Term f = a->lhs();
      const Term n = a->rhs();
      model_[f] = Term();
      data_[f][n] = Data(data_[f][n].occurs);
      if (a->pos() && !order_.contains(f)) {
        order_.Insert(f);
      }
    }
    trail_.resize(level_begin_[l]);
    level_begin_.resize(l);
    trail_head_ = trail_.size();
  }

  Term CandidateName(Term f) const {
    assert(!f.null() && model_[f].null());
    for (Term n : names_[f.sort()]) {
      if (!n.null() && !data_[f][n].neq) {
        return n;
      }
    }
    return Term();
  }

  void Bump(Term f) {
    activity_[f] += bump_step_;
    if (activity_[f] > 1e100) {
      for (double& activity : activity_) {
        activity *= 1e-100;
      }
      bump_step_ *= 1e-100;
    }
    if (order_.contains(f)) {
      order_.Increase(f);
    }
  }

  bool satisfies(Literal a) const {
    const Term f = a.lhs();
    const Term n = a.rhs();
    if (a.pos()) {
      return model_[f] == n;
    } else {
      return (!model_[f].null() && model_[f] != n) || data_[f][n].neq;
    }
  }

  bool falsifies(Literal a) const { return satisfies(a.flip()); }

  bool satisfies(const Clause& c) const {
    return std::any_of(c.begin(), c.end(), [this](Literal a) { return satisfies(a); });
  }

  bool falsifies(const Clause& c) const {
    return std::all_of(c.begin(), c.end(), [this](Literal a) { return falsifies(a); });
  }

  level_t level_of(Literal a) const {
    assert(a.primitive());
    const Term f = a.lhs();
    Term n = a.rhs();
    const level_t l = data_[f][n].level;
    if (l != 0) {
      return l;
    }
    n = model_[f];
    return !n.null() ? data_[f][n].level : kNullRef;
  }

  cref_t reason_of(Literal a) const {
    assert(a.primitive());
    const Term f = a.lhs();
    Term n = a.rhs();
    const cref_t r = data_[f][n].reason;
    if (r != 0) {
      return r;
    }
    n = model_[f];
    return !n.null() ? data_[f][n].reason : kNullRef;
  }

  bool seen(Literal a) const {
    assert(a.primitive());
    const Term f = a.lhs();
    const Term n = a.rhs();
    return data_[f][n].seen || (!model_[f].null() && data_[f][model_[f]].seen);
  }

  void set_seen(Literal a, bool s) {
    assert(a.primitive());
    const Term f = a.lhs();
    const Term n = !model_[f].null() ? model_[f] : a.rhs();
    data_[f][n].seen = s;
  }

  void set_occurs(Term f, Term n, bool yes) { data_[f][n].occurs = yes; }
  bool occurs(Term f, Term n) const { return data_[f][n].occurs; }

  level_t current_level() const { return level_begin_.size(); }

  // empty_clause_ is true iff the empty clause has been derived.
  bool empty_clause_ = false;

  // clauses_ is the sequence of clauses added initially or learnt.
  // extra_name_ stores an additional name for every sort.
  // names_ is the set of names that occur in clauses plus extra names.
  std::vector<Clause*> clauses_ = std::vector<Clause*>(1, nullptr);
  DenseMap<Symbol::Sort, Term> extra_name_;
  DenseMap<Symbol::Sort, DenseMap<Term, Term>> names_;

  // watchers_ maps every function to a sequence of clauses that watch it.
  // Every clause watches two functions, and when a literal with this function
  // is propagated, the watching clauses are inspected.
  DenseMap<Term, std::vector<cref_t>> watchers_;

  // trail_ is a sequence of literals in the order they were derived.
  // level_begin_ groups the literals of trail_ into chunks by their level at
  // which they were derived, where level_begin_[l] determines the index of the
  // first literal of level l.
  // trail_head_ is the index of the first literal of trail_ that hasn't been
  // propagated yet.
  std::vector<Literal> trail_;
  std::vector<uref_t>  level_begin_ = std::vector<uref_t>(1, trail_.size());
  uref_t               trail_head_ = 0;

  // model_ is an assignment of functions to names, i.e., positive literals.
  // data_ is meta data for every function and name pair (cf. Data).
  DenseMap<Term, Term>                 model_;
  DenseMap<Term, DenseMap<Term, Data>> data_;

  // order_ is a heap that ranks functions by their activity.
  // activity_ stores the activity of each activity.
  // bump_step_ is the current increase factor.
  Heap<Term, ActivityCompare> order_ = Heap<Term, ActivityCompare>(ActivityCompare(&activity_));
  DenseMap<Term, double>      activity_;
  double                      bump_step_ = 1.0; 
};

}  // namespace limbo

#endif  // LIMBO_SOLVER_H_

