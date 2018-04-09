// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014-2018 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.

#ifndef LIMBO_SOLVER_H_
#define LIMBO_SOLVER_H_

#include <algorithm>
#include <limits>
#include <queue>
#include <utility>
#include <vector>
#include <unordered_map>

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

  T Top() { return size() >= 1 ? heap_[1] : make_default_(); }

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
  static constexpr level_t kNullLevel = 0;
  static constexpr level_t kRootLevel = 1;
  static constexpr level_t kMaxLevel = std::numeric_limits<level_t>::max();

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
      Register(a);
      Enqueue(a, kNullRef);
    }
  }

  void AddClause(const std::vector<Literal>& lits) {
    if (lits.size() == 0) {
      empty_clause_ = true;
    } else if (lits.size() == 1) {
      AddLiteral(lits[0]);
    } else {
      Clause* c = Clause::New(lits);
      if (c->unsatisfiable()) {
        empty_clause_ = true;
      } else if (c->primitive() && !c->valid() && !satisfies(*c)) {
        for (Literal a : *c) {
          Register(a);
        }
        AddClause(c, kNullRef);
      }
    }
  }

  const DenseMap<Term, Term>& model() const { return model_; }

  bool Solve() {
    std::vector<Literal> learnt;
    for (;;) {
      const cref_t conflict = Propagate();
      //if (clauses_.size() > 684) { exit(0); }
      if (conflict != kNullRef) {
        if (current_level() == kRootLevel) {
          return false;
        }
        learnt.clear();
        level_t btlevel;
        Analyze(conflict, &learnt, &btlevel);
        //std::cout << "Learnt " << learnt << ", backtracking to " << btlevel << std::endl;
        assert(std::all_of(learnt.begin(), learnt.end(), [this](Literal a) { return falsifies(a); }));
        //if (learnt.size() > 1) {
        //  Clause* c = Clause::NewNormalized(learnt);
        //  if (!c->Normalized()) {
        //    std::cout << "CLAUSE: " << learnt << std::endl;
        //    std::cout << "CLAUSE: " << *c << std::endl;
        //    for (Literal a : *c) {
        //      std::cout << a << " from level " << (a == (*c)[0] ? level_of(a) : level_of_complementary(a)) << " " << (falsifies(a) ? "" : "not ") << "falsified" << std::endl;
        //    }
        //    for (int i = 0; i < trail_.size(); ++i) {
        //      const Literal a = trail_[i];
        //      std::cout << "Trail literal " << i << " " << a << " from level " << level_of(a) << " " << (data_[a.lhs()][a.rhs()].wanted ? "wanted" : "not wanted") << std::endl;
        //    }
        //    std::cout << "level: " << current_level() << std::endl;
        //    for (level_t l = 0; l <= current_level(); ++l) {
        //      std::cout << "falsified at level " << l << ": " << falsifies(*c, l) << std::endl;
        //    }
        //    abort();
        //  }
        //  Clause::Delete(c);
        //}
        Backtrack(btlevel);
        assert(!falsifies(learnt[0]));
        assert(std::all_of(learnt.begin() + 1, learnt.end(), [this](Literal a) { return falsifies(a); }));
        assert(!learnt.empty());
        if (learnt.size() == 1) {
          Enqueue(learnt[0], kNullRef);
        } else {
          Clause* c = Clause::New(learnt);
          assert((*c)[0] == learnt[0]);
          const cref_t cr = AddClause(c, conflict);
          if (cr != kNullRef) {
            Enqueue(learnt[0], cr);
          } else {
            Clause::Delete(c);
          }
        }
      } else {
        const Term f = order_.Top();
        if (f.null()) {
          return true;
        }
        const Term n = CandidateName(f);
        if (n.null()) {
          //std::cout << f << std::endl;
          //for (int i = 0; i < names_[f.sort()].size(); ++i) {
          //  const Term n = names_[f.sort()][i];
          //  if (!n.null()) {
          //    const Literal a = Literal::Eq(f, n);
          //    std::cout << a << ": " << satisfies(a) << std::endl;
          //    std::cout << a.flip() << ": " << satisfies(a.flip()) << std::endl;
          //  }
          //}
          return false;
        }
        NewLevel();
        Enqueue(Literal::Eq(f, n), kNullRef);
        //std::cout << current_level() << ": " << f << " = " << n << std::endl;
      }
    }
  }

 private:
  struct Data {  // meta data for a pair (f,n)
    Data() = default;
    Data(bool occurs) :
        seen_subsumed(0), wanted(0), occurs(occurs), model_neq(0), level(0), reason(0)  {}
    Data(bool model_neq, level_t l, cref_t r) :
        seen_subsumed(0), wanted(0), occurs(1), model_neq(model_neq), level(l), reason(r)  {}
    unsigned seen_subsumed :  1;  // auxiliary flag to keep track of seen trail literals
    unsigned wanted        :  1;  // auxiliary flag to keep track of seen trail literals
    unsigned occurs        :  1;  // true iff f occurs with n in added clauses or literals
    unsigned model_neq     :  1;  // true iff f != n was set or derived
    unsigned level         : 29;  // level at which f = n or f != n was set or derived
    cref_t reason;                // clause which derived f = n or f != n
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
    if (funcs_[s][f] != f && !order_.contains(f)) {
      order_.Insert(f);
    }
    funcs_[s][f] = f;
    names_[s][n] = n;
    names_[s][extra_n] = extra_n;
    data_[f][n].occurs = true;
    data_[f][extra_n].occurs = true;
  }

  cref_t AddClause(Clause* c, cref_t reason) {
    assert(!c->empty() && !c->unit());
    assert(reason != kNullRef || !satisfies(*c));
    assert(!c->valid());
    const cref_t cr = clauses_.size();
    clauses_.push_back(c);
    const Term f0 = (*c)[0].lhs();
    const Term f1 = (*c)[1].lhs();
    watchers_[f0].push_back(cr);
    if (f0 != f1) {
      watchers_[f1].push_back(cr);
    }
    return cr;
  }

  cref_t Propagate() {
    cref_t conflict = kNullRef;
    while (trail_head_ < trail_.size() && conflict == kNullRef) {
      Literal a = trail_[trail_head_++];
      conflict = Propagate(a);
      //std::cout << "Propagated " << a << std::endl;
    }
    //assert(conflict != kNullRef || std::all_of(clauses_.begin()+1, clauses_.end(), [this](Clause* c) { return std::any_of(watchers_[(*c)[0].lhs()].begin(), watchers_[(*c)[0].lhs()].end(), [this, c](cref_t cr) { return c == clauses_[cr]; }); }));
    //assert(conflict != kNullRef || std::all_of(clauses_.begin()+1, clauses_.end(), [this](Clause* c) { return std::any_of(watchers_[(*c)[1].lhs()].begin(), watchers_[(*c)[1].lhs()].end(), [this, c](cref_t cr) { return c == clauses_[cr]; }); }));
    assert(conflict != kNullRef || std::all_of(clauses_.begin()+1, clauses_.end(), [this](Clause* c) { return satisfies(*c) || !falsifies((*c)[0]) || std::all_of(c->begin()+2, c->end(), [this](Literal a) { return falsifies(a); }); }));
    assert(conflict != kNullRef || std::all_of(clauses_.begin()+1, clauses_.end(), [this](Clause* c) { return satisfies(*c) || !falsifies((*c)[1]) || std::all_of(c->begin()+2, c->end(), [this](Literal a) { return falsifies(a); }); }));
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
      const Term f1 = c[1].lhs();

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
        assert(std::all_of(c.begin(), c.end(), [this](Literal a) { return falsifies(a); }));
      } else if (w[0] || w[1]) {
        const Literal b = c[static_cast<int>(w[0])];
        //std::cout << current_level() << ": " << b << " by propagation from " << c << std::endl;
        Enqueue(b, cr);
        assert(std::all_of(c.begin(), c.end(), [this, b](Literal a) { return a == b ? satisfies(a) : falsifies(a); }));
      } else {
        assert(!falsifies(c[0]) && !falsifies(c[1]));
        //assert(std::any_of(c.begin(), c.end(), [this](Literal a) { return !falsifies(a); }));
        //assert(!falsifies(c[0]) || std::all_of(c.begin()+2, c.end(), [this](Literal a) { return !falsifies(a); }));
        //assert(!falsifies(c[1]) || std::all_of(c.begin()+2, c.end(), [this](Literal a) { return !falsifies(a); }));
      }
    }
    ws.resize(cr_ptr2 - ws.begin());
    //assert(conflict == kNullRef || std::all_of(clauses_[conflict]->begin(), clauses_[conflict]->end(), [this](Literal a) { return falsifies(a); }));
    return conflict;
  }

  void Analyze(cref_t conflict, std::vector<Literal>* learnt, level_t* btlevel) {
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) { return !d.seen_subsumed && !d.wanted; }); }));
    int depth = 0;
    Literal trail_lit = Literal();
    learnt->push_back(trail_lit);
    uref_t trail_index = trail_.size() - 1;

    // When a literal has been added to the conflict clause, every subsuming
    // literal would be redundant and should be skipped.
    // (1) f == n is only subsumed by f == n.
    // (2) f != n is only subsumed by f != n and f == n' for every n' != n.
    // Every literal we see has a complementary literal on the trail, and the
    // trail does not contain two mutually complementary literals.
    // In case (1), the trail only contains f != n or f == n', but not f == n.
    // Hence we will not see f != n. Therefore marking (f,n) where n !=
    // model_[f] uniquely f == n as seen and nothing else.
    // In case (2), the trail only contains f == n and perhaps f != n', but
    // not f != n or f == n'. Hence we might see f != n and f == n', but not
    // f == n or f != n'. Therefore marking (f,n) where n == model_[f]
    // uniquely identifies f != n and f == n' for all n' != n.
    auto see_subsuming = [this](const Literal a) {
      assert(falsifies(a));
      assert(a.pos() || !model_[a.lhs()].null());
      assert(model_[a.lhs()].null() || (a.pos() != (model_[a.lhs()] == a.rhs())));
      const Term f = a.lhs();
      const Term n = a.rhs();
      data_[f][n].seen_subsumed = 1;
    };
    // Some literal subsumed by f == n was seen iff f == n or f != n' was seen for some n'.
    // Some literal subsumed by f != n was seen iff f != n was seen.
    // If f == n was seen, then n != model_[f] and (f,n) is marked
    // If f != n was seen, then n == model_[f] and (f,model_[f]) is marked.
    auto seen_subsumed = [this](const Literal a) {
      assert(falsifies(a));
      assert(model_[a.lhs()].null() || (a.pos() != (model_[a.lhs()] == a.rhs())));
      const Term f = a.lhs();
      const Term n = a.rhs();
      const Term m = model_[f];
      return data_[f][n].seen_subsumed || (a.pos() && !m.null() && data_[f][m].seen_subsumed);
    };

    // When we want a complementary literal to f == n, we prefer f != n over
    // f == model_[f] because this will become f == n in the conflict clause.
    // This also means that we want exactly one literal, which eliminates the
    // need for traversing the whole level again to reset the wanted flag.
    auto want_complementary_on_level = [this](const Literal a, level_t l) {
      assert(falsifies(a));
      assert(data_[a.lhs()][a.rhs()].level <= l);
      const Term f = a.lhs();
      const Term n = a.rhs();
      const Term m = model_[f];
      if (!a.pos()) {
        assert(data_[f][n].level == l);
        data_[f][n].wanted = true;
      } else {
        if (data_[f][n].level == l) {
          assert(data_[f][n].model_neq);
          data_[f][n].wanted = true;
        } else {
          assert(!m.null());
          assert(data_[f][m].level == l);
          data_[f][m].wanted = true;
        }
      }
    };
    auto wanted_complementary_on_level = [this](const Literal a, level_t l) {
      assert(falsifies(a));
      assert(data_[a.lhs()][a.rhs()].level <= l);
      const Term f = a.lhs();
      const Term n = a.rhs();
      const Term m = model_[f];
      return !a.pos() ?
          data_[f][n].wanted :
          (data_[f][n].level == l && data_[f][n].wanted) || (!m.null() && data_[f][m].wanted);
    };
    // We un-want every trail literal after it has been traversed.
    auto wanted = [this](const Literal a) {
      assert(satisfies(a));
      const Term f = a.lhs();
      const Term n = a.rhs();
      return data_[f][n].wanted;
    };

    //std::cout << std::endl;
    //std::unordered_map<Literal, int> fup;
    do {
      //if (conflict == kNullRef) {
      //  for (Literal x : trail_) { std::cout << "Trail: " << level_of(x) << ": " << x << (reason_of(x) != kNullRef ? " caused by propagation" : "") << std::endl; }
      //}
      assert(conflict != kNullRef);
      //std::cout << "Conflict clause " << *clauses_[conflict] << " at level " << current_level() << std::endl;
      //bool found = false;
      for (Literal a : *clauses_[conflict]) {
        if (trail_lit == a) {
          continue;
        }
        assert(falsifies(a));
        const level_t l = level_of_complementary(a);
        assert(l <= current_level());
        if (l == kRootLevel || seen_subsumed(a) || wanted_complementary_on_level(a, l)) {
          continue;
        }
        if (l < current_level()) {
          //std::cout << "Adding " << a << " from " << l << std::endl;
          learnt->push_back(a);
          see_subsuming(a);
          Bump(a.lhs());
        } else if (l == current_level()) {
          //++fup[a];
          //std::cout << "Noding " << a << " from " << l << std::endl;
          ++depth;
          want_complementary_on_level(a, l);
          Bump(a.lhs());
        }
      }
      //if (depth == 0) {
      //  std::cout << "TRAIL:" << std::endl;
      //  for (int i = 0; i < trail_.size(); ++i) {
      //    const Literal a = trail_[i];
      //    std::cout << "Trail literal " << i << " " << a << " from level " << level_of(a) << " " << (data_[a.lhs()][a.rhs()].wanted ? "wanted" : "not wanted") << std::endl;
      //  }
      //  std::cout << "CLAUSE:" << std::endl;
      //  for (Literal a : *clauses_[conflict]) {
      //    std::cout << a << " from level " << (a == trail_lit ? level_of(a) : level_of_complementary(a)) << " " << (data_[a.lhs()][a.rhs()].seen_subsumed ? "seen_subsumed" : "not seen_subsumed") << " " << (falsifies(a, 39) ? "" : "not ") << "falsified at level 39" << std::endl;
      //  }
      //  std::cout << "TRAIL_LIT: " << trail_lit << std::endl;
      //  std::cout << "level: " << current_level() << std::endl;
      //  for (level_t l = 0; l <= current_level(); ++l) {
      //    std::cout << "falsified at level " << l << ": " << falsifies(*clauses_[conflict], l) << std::endl;
      //  }
      //}
      assert(depth > 0);
      //if (!found) {
      //  for (int i = 0; i < trail_.size(); ++i) {
      //    std::cout << "Trail literal " << i << " " << trail_[i] << " from level " << level_of(trail_[i]) << " " << (data_[trail_[i].lhs()][trail_[i].rhs()].wanted ? "wanted" : "not wanted") << std::endl;
      //  }
      //  for (Literal a : *clauses_[conflict]) {
      //    std::cout << a << " [" << (a == trail_lit ? level_of(a) : level_of_complementary(a)) << "] ";
      //  }
      //  std::cout << std::endl;
      //  std::cout << "trail_lit == " << trail_lit << std::endl;
      //  std::cout << "current_level() == " << current_level() << std::endl;
      //}
      //assert(found);

      //const int prev_trail_index = trail_index;
      while (!wanted(trail_[trail_index--])) {
        //if (trail_index < 0) {
        //  for (int i = 0; i < trail_.size(); ++i) {
        //    if (data_[trail_[i].lhs()][trail_[i].rhs()].wanted) {
        //      std::cout << "Trail literal " << i << " " << trail_[i] << " " << (data_[trail_[i].lhs()][trail_[i].rhs()].wanted ? "wanted" : "not wanted") << std::endl;
        //    }
        //  }
        //  std::cout << "Follow-ups with depth " << depth << " out of " << fup << std::endl;
        //  std::cout << "Previous one was " << prev_trail_index << " " << trail_[prev_trail_index] << std::endl;
        //}
        assert(trail_index >= 0);
      }
      trail_lit = trail_[trail_index + 1];
      data_[trail_lit.lhs()][trail_lit.rhs()].wanted = false;
      assert(!trail_lit.lhs().null());
      assert(!trail_lit.rhs().null());
      //for (const auto p : fup) {
      //  if (Literal::Complementary(p.first, trail_lit)) {
      //    fup[p.first]--;
      //    break;
      //  }
      //}
      --depth;
      //std::cout << "Follow-up " << trail_lit << " from level " << level_of(trail_lit) << " with depth " << depth << std::endl;
      conflict = reason_of(trail_lit);
    } while (depth > 0);
    (*learnt)[0] = trail_lit.flip();

    if (learnt->size() == 1) {
      *btlevel = kRootLevel;
    } else {
      uref_t max = 1;
      *btlevel = level_of_complementary((*learnt)[max]);
      for (uref_t i = 2, s = learnt->size(); i != s; ++i) {
        level_t l = level_of_complementary((*learnt)[i]);
        if (*btlevel < l) {
          max = i;
          *btlevel = l;
        }
      }
      std::swap((*learnt)[1], (*learnt)[max]);
    }

    for (const Literal a : *learnt) {
      //const Literal a = (*learnt)[i];
      //if (a.lhs().null() || a.rhs().null()) {
      //  std::cout << a << std::endl;
      //}
      data_[a.lhs()][a.rhs()].seen_subsumed = false;
    }
    assert(level_of(trail_lit) > *btlevel);
    assert(*btlevel >= kRootLevel);
    assert(std::all_of(learnt->begin(), learnt->end(), [this](Literal a) { return falsifies(a, current_level()); }));
    assert(learnt->size() < 1 || !falsifies((*learnt)[0], current_level() - 1));
    assert(learnt->size() < 2 || !falsifies((*learnt)[1], *btlevel - 1));
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) { return !d.seen_subsumed; }); }));
    assert(std::all_of(data_.begin(), data_.end(), [](const auto& ds) { return std::all_of(ds.begin(), ds.end(),
           [](const Data& d) { return !d.wanted; }); }));
  }

  void NewLevel() { level_size_.push_back(trail_.size()); }

  void Enqueue(Literal a, cref_t reason) {
    assert(a.primitive());
    assert(!falsifies(a));
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    if (m.null() && (a.pos() || !data_[f][n].model_neq)) {
      assert(!satisfies(a));
      trail_.push_back(a);
      if (a.pos()) {
        model_[f] = n;
      } else {
        BumpToFront(f);
      }
      data_[f][n] = Data(!a.pos(), current_level(), reason);
      if (a.pos() && order_.contains(f)) {
        order_.Erase(f);
      }
    }
  }

  void Backtrack(level_t l) {
    //std::cout << "backtracking to " << l << " from " << current_level() << std::endl;
    for (auto a = trail_.cbegin() + level_size_[l], e = trail_.cend(); a != e; ++a) {
      const Term f = a->lhs();
      const Term n = a->rhs();
      model_[f] = Term();
      data_[f][n] = Data(data_[f][n].occurs);
      if (a->pos() && !order_.contains(f)) {
        order_.Insert(f);
      }
    }
    trail_.resize(level_size_[l]);
    level_size_.resize(l);
    trail_head_ = trail_.size();
  }

  Term CandidateName(Term f) const {
    //if (!f.null() && !model_[f].null()) {
    //  const Term m = model_[f];
    //  std::cout << Literal::Eq(f,m) << " " << (satisfies(Literal::Eq(f,m)) ? "satisfied" : "not satisfied") << data_[f][m].level << std::endl;
    //}
    assert(!f.null() && model_[f].null());
    for (Term n : names_[f.sort()]) {
      if (!n.null() && data_[f][n].occurs && !data_[f][n].model_neq) {
        return n;
      }
    }
    return Term();
  }

  void BumpToFront(Term f) {
    for (const double& activity : activity_) {
      if (activity_[f] < activity) {
        activity_[f] = activity;
      }
    }
    activity_[f] += bump_step_;
    if (order_.contains(f)) {
      order_.Increase(f);
    }
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

  bool satisfies(const Literal a, const level_t l = kMaxLevel) const {
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    if (a.pos()) {
      return m == n && data_[f][m].level <= l;
    } else {
      return (!m.null() && m != n && data_[f][m].level <= l) || (data_[f][n].model_neq && data_[f][n].level <= l);
    }
  }

  bool falsifies(const Literal a, const level_t l = kMaxLevel) const { return satisfies(a.flip(), l); }

  bool satisfies(const Clause& c, const level_t l = kMaxLevel) const {
    return std::any_of(c.begin(), c.end(), [this, l](Literal a) { return satisfies(a, l); });
  }

  bool falsifies(const Clause& c, const level_t l = kMaxLevel) const {
    return std::all_of(c.begin(), c.end(), [this, l](Literal a) { return falsifies(a, l); });
  }

  level_t level_of(Literal a) const {
    assert(a.primitive());
    assert(satisfies(a));
    assert(!a.pos() || model_[a.lhs()] == a.rhs());
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    return !a.pos() && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level;
  }

  cref_t reason_of(Literal a) const {
    assert(a.primitive());
    assert(satisfies(a));
    assert(!a.pos() || model_[a.lhs()] == a.rhs());
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    return !a.pos() && data_[f][n].model_neq ? data_[f][n].reason : data_[f][m].reason;
  }

  level_t level_of_complementary(Literal a) const {
    assert(a.primitive());
    assert(falsifies(a));
    assert(a.pos() || model_[a.lhs()] == a.rhs());
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    return a.pos() && data_[f][n].model_neq ? data_[f][n].level : data_[f][m].level;
  }

  cref_t reason_of_complementary(Literal a) const {
    assert(a.primitive());
    assert(falsifies(a));
    assert(a.pos() || model_[a.lhs()] == a.rhs());
    const Term f = a.lhs();
    const Term n = a.rhs();
    const Term m = model_[f];
    return a.pos() && data_[f][n].model_neq ? data_[f][n].reason : data_[f][m].reason;
  }

  level_t current_level() const { return level_size_.size(); }

  // empty_clause_ is true iff the empty clause has been derived.
  bool empty_clause_ = false;

  // clauses_ is the sequence of clauses added initially or learnt.
  // extra_name_ stores an additional name for every sort.
  // funcs_ is the set of functions that occur in clauses.
  // names_ is the set of names that occur in clauses plus extra names.
  std::vector<Clause*> clauses_ = std::vector<Clause*>(1, nullptr);
  DenseMap<Symbol::Sort, Term> extra_name_;
  DenseMap<Symbol::Sort, DenseMap<Term, Term>> funcs_;
  DenseMap<Symbol::Sort, DenseMap<Term, Term>> names_;

  // watchers_ maps every function to a sequence of clauses that watch it.
  // Every clause watches two functions, and when a literal with this function
  // is propagated, the watching clauses are inspected.
  DenseMap<Term, std::vector<cref_t>> watchers_;

  // trail_ is a sequence of literals in the order they were derived.
  // level_size_ groups the literals of trail_ into chunks by their level at
  // which they were derived, where level_size_[l] determines the number of
  // literals set or derived up to level l.
  // trail_head_ is the index of the first literal of trail_ that hasn't been
  // propagated yet.
  std::vector<Literal> trail_;
  std::vector<uref_t>  level_size_ = std::vector<uref_t>(1, trail_.size());
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

