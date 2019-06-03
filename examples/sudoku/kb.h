// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2017 Christoph Schwering

#ifndef EXAMPLES_SUDOKU_KB_H_
#define EXAMPLES_SUDOKU_KB_H_

#include <iostream>
#include <sstream>
#include <vector>

#include <limbo/limsat.h>
#include <limbo/internal/maybe.h>
#include <limbo/io/output.h>

#include "game.h"
#include "timer.h"

class KnowledgeBase {
 public:
  template<typename T>
  using Maybe  = limbo::internal::Maybe<T>;

  explicit KnowledgeBase(int max_k) : max_k_(max_k) {
    sort_ = abc().CreateSort(false);
    f_.resize(9 * 9);
    n_.resize(9);
    fs_.resize(9 * 9);
    ns_.resize(9);
    for (int x = 1; x <= 9; ++x) {
      for (int y = 1; y <= 9; ++y) {
        const char f_str[] = {char('0' + x), char('0' + y), '\0'};
        cell(x, y) = abc().CreateFun(sort_, 0);
        LIMBO_REG_STR(cell(x, y), f_str);
        Formula ff = Formula::Fun(cell(x, y));
        ff.Strip();
        cellf(x, y) = ff.head().u.f_s;
      }
    }
    for (int i = 1; i <= 9; ++i) {
      const char n_str[] = {char('0' + i), '\0'};
      val(i) = abc().CreateName(sort_, 0);
      LIMBO_REG_STR(val(i), n_str);
      Formula ff = Formula::Name(val(i));
      ff.Strip();
      valn(i) = ff.head().u.n_s;
    }
    for (int x = 1; x <= 9; ++x) {
      for (int y = 1; y <= 9; ++y) {
        for (int yy = 1; yy <= 9; ++yy) {
          if (y != yy) {
            Add(neq(cell(x, y), cell(x, yy)));
          }
        }
      }
    }
    for (int x = 1; x <= 9; ++x) {
      for (int xx = 1; xx <= 9; ++xx) {
        for (int y = 1; y <= 9; ++y) {
          if (x != xx) {
            Add(neq(cell(x, y), cell(xx, y)));
          }
        }
      }
    }
    for (int i = 1; i <= 3; ++i) {
      for (int j = 1; j <= 3; ++j) {
        for (int x = 3*i-2; x <= 3*i; ++x) {
          for (int xx = 3*i-2; xx <= 3*i; ++xx) {
            for (int y = 3*j-2; y <= 3*j; ++y) {
              for (int yy = 3*j-2; yy <= 3*j; ++yy) {
                if (x != xx || y != yy) {
                  Add(neq(cell(x, y), cell(xx, yy)));
                }
              }
            }
          }
        }
      }
    }
    for (int x = 1; x <= 9; ++x) {
      for (int y = 1; y <= 9; ++y) {
        std::vector<Formula> as;
        for (int i = 1; i <= 9; ++i) {
          as.push_back(eq(cell(x, y), val(i)));
        }
        Add(Formula::Or(std::move(as)));
      }
    }
  }

  void InitGame(const Game* g) {
    for (int x = 1; x <= 9; ++x) {
      for (int y = 1; y <= 9; ++y) {
        int i = g->get(Point(x, y));
        if (i != 0) {
          Add(eq(cell(x, y), val(i)));
        }
      }
    }
  }

  int max_k() const { return max_k_; }

  void Add(Point p, int i) { Add(eq(cellf(p), valn(i))); }

  Maybe<int> Val(Point p, int k) {
    t_.start();
    //const Maybe<Name> r = lim_sat_.Determines(k, cell(p));
    //if (r) {
    //  assert(!r.val.null());
    //  for (int i = 1; i <= 9; ++i) {
    //    if (r.val.null() || abc().Unstrip(r.val) == val(i)) {
    //      return limbo::internal::Just(i);
    //    }
    //  }
    //}
    for (int i = 1; i <= 9; ++i) {
      printf("x = %d, y = %d, i = %d, size = %lu\n", p.x, p.y, i, lim_sat_.clauses().size());
      if (lim_sat_.Solve(k, eq(cellf(p), valn(i)).readable())) {
        return limbo::internal::Just(i);
      }
    }
    t_.stop();
    return limbo::internal::Nothing;
  }

  const Timer& timer() const { return t_; }
  void ResetTimer() { t_.reset(); }

  void PrintDimacs(std::ostream* os) {
    using namespace limbo;
    using namespace limbo::io;
    const std::set<std::vector<Lit>>& cs = lim_sat_.clauses();
    *os << "p fcnf 81 9 " << (cs.size() + 81) << std::endl;
    *os << "c Sudoku rules" << std::endl;
    for (const std::vector<Lit>& c : cs) {
      for (Lit a : c) {
        int i = 0;
        for (int x = 1; x <= 9; ++x) {
          for (int y = 1; y <= 9; ++y) {
            if (a.fun() == cellf(x, y)) {
              i = x + (y - 1) * 9;
            }
          }
        }
        int j = 0;
        for (int m = 1; m <= 9; ++m) {
          if (a.name() == valn(m)) {
            j = m;
          }
        }
        if (i == 0 || j == 0) {
          goto next;
        }
        *os << (i < 10 ? " " : "") << (a.pos() ? ' ' : '-') << i << '=' << j << ' ';
      }
      *os << '0' << std::endl;
      *os << "c " << c << std::endl;
next: {}
    }
  }

 private:
  using Abc     = limbo::Alphabet;
  using Fun     = limbo::Fun;
  using Name    = limbo::Name;
  using Lit     = limbo::Lit;
  using Formula = limbo::Formula;
  using LimSat  = limbo::LimSat;

  void Add(Formula&& f) {
    Abc::DenseMap<Abc::Sort, std::vector<Name>> subst;
    subst[sort_] = n_;
    //std::cout << f << std::endl;
    f.Normalize();
    //std::cout << f << std::endl;
    f.Ground(subst);
    //std::cout << f << std::endl;
    f.Strip();
    //std::cout << f << std::endl;
    Maybe<std::vector<std::vector<Lit>>> cs = f.readable().CnfClauses();
    if (!cs) {
      std::cerr << "No clauses extracted from " << f << std::endl;
    } else {
      //std::cout << cs.val.size() << " clauses" << std::endl;
      for (std::vector<Lit>& c : cs.val) {
        //std::cout << c << std::endl;
        Add(std::move(c));
      }
    }
    //std::cout << std::endl;
  }

  void Add(Lit a) { std::vector<Lit> c; c.push_back(a); Add(std::move(c)); }

  void Add(std::vector<Lit>&& c) { lim_sat_.AddClause(std::move(c)); }

  Abc& abc() const { return Abc::instance(); }

  Fun   cellf(Point p)      const { return cellf(p.x, p.y); }
  Fun   cellf(int x, int y) const { assert(1 <= x && x <= 9 && 1 <= y && y <= 9); return f_[(x-1) * 9 + y-1]; }
  Name  valn(int i)         const { assert(1 <= i && i <= 9); return n_[i-1]; }
  Fun&  cellf(int x, int y)       { assert(1 <= x && x <= 9 && 1 <= y && y <= 9); return f_[(x-1) * 9 + y-1]; }
  Name& valn(int i)               { assert(1 <= i && i <= 9); return n_[i-1]; }

  Abc::FunSymbol   cell(Point p)      const { return cell(p.x, p.y); }
  Abc::FunSymbol   cell(int x, int y) const { assert(1 <= x && x <= 9 && 1 <= y && y <= 9); return fs_[(x-1) * 9 + y-1]; }
  Abc::NameSymbol  val(int i)         const { assert(1 <= i && i <= 9); return ns_[i-1]; }
  Abc::FunSymbol&  cell(int x, int y)       { assert(1 <= x && x <= 9 && 1 <= y && y <= 9); return fs_[(x-1) * 9 + y-1]; }
  Abc::NameSymbol& val(int i)               { assert(1 <= i && i <= 9); return ns_[i-1]; }

  Formula eq(Fun f, Name n) const { return Formula::Lit(Lit::Eq(f, n)); }
  Formula neq(Fun f, Name n) const { return Formula::Lit(Lit::Neq(f, n)); }

  Formula eq(Abc::FunSymbol f, Abc::NameSymbol n) const { return Formula::Equals(Formula::Fun(f), Formula::Name(n)); }
  Formula eq(Abc::FunSymbol f, Abc::FunSymbol ff) const { return Formula::Equals(Formula::Fun(f), Formula::Fun(ff)); }
  Formula neq(Abc::FunSymbol f, Abc::NameSymbol n) const { return Formula::NotEquals(Formula::Fun(f), Formula::Name(n)); }
  Formula neq(Abc::FunSymbol f, Abc::FunSymbol ff) const { return Formula::NotEquals(Formula::Fun(f), Formula::Fun(ff)); }

  int    max_k_;
  LimSat lim_sat_;

  Abc::Sort sort_;

  std::vector<Fun>  f_;
  std::vector<Name> n_;

  std::vector<Abc::FunSymbol>  fs_;
  std::vector<Abc::NameSymbol> ns_;

  Timer t_;
};

#endif  // EXAMPLES_SUDOKU_KB_H_

