// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014, 2015, 2016 schwering@kbsg.rwth-aachen.de

#ifndef SRC_TERM_H_
#define SRC_TERM_H_

#include <cassert>
#include <map>
#include <set>
#include <vector>
#include "./compar.h"

namespace lela {

class Symbol {
 public:
  typedef int Id;
  typedef int Sort;
  struct Comparator;

  static Symbol CreateFunction(Id id, Sort sort, int arity) {
    assert(id > 0);
    id = -1 * (2 * id + 1);
    return Symbol(id, sort, arity);
  }

  static Symbol CreateName(Id id, Sort sort) {
    assert(id > 0);
    return Symbol(id, sort, 0);
  }

  static Symbol CreateVariable(Id id, Sort sort) {
    id = -1 * (2 * id);
    return Symbol(id, sort, 0);
  }

  bool function() const { return id_ < 0 && ((-id_) % 2) != 0; }
  bool name() const { return id_ > 0; }
  bool variable() const { return id_ < 0 && ((-id_) % 2) == 0; }

  int arity() const { return arity_; }

 private:
  Symbol(Id id, Sort sort, int arity) : id_(id), sort_(sort), arity_(arity) {}

  Id id_;
  Sort sort_;
  int arity_;
};

struct Symbol::Comparator {
  typedef Symbol value_type;

  bool operator()(const value_type& a, const value_type& b) const {
    return comp(a.id_, a.sort_, a.arity_,
                b.id_, b.sort_, b.arity_);
  }

 private:
  LexicographicComparator<LessComparator<Id>,
                          LessComparator<Sort>,
                          LessComparator<int>> comp;
};

class Term {
 public:
  struct Comparator;
  typedef std::vector<Term> Vector;

  Term() = default;
  Term(const Term&) = default;
  Term& operator=(const Term&) = default;

  static Term Create(Symbol symbol);
  static Term Create(Symbol symbol, const Vector& args);

  bool operator==(Term t) const { return data_ == t.data_; }

  Term Substitute(Term pre, Term post) const {
    if (*this == pre) {
      return post;
    }
    if (arity() > 0) {
      Vector args(data_->args_.size());
      for (Term arg : data_->args_) {
        args.push_back(arg.Substitute(pre, post));
      }
      if (args != data_->args_) {
        return Create(data_->symbol_, args);
      }
    }
    return *this;
  }

  const Symbol& symbol() const { return data_->symbol_; }
  const Vector& args() const { return data_->args_; }

  bool function() const { return data_->symbol_.function(); }
  bool name() const { return data_->symbol_.name(); }
  bool variable() const { return data_->symbol_.variable(); }
  int arity() const { return data_->symbol_.arity(); }

  bool null() const { return !data_; }

 private:
  struct Data {
    struct PtrComparator;
    Data(const Symbol& symbol, const Vector& args) : symbol_(symbol), args_(args) {}

    Symbol symbol_;
    Vector args_;
  };

  explicit Term(Data* data) : data_(data) {}

  static std::set<Data*, Data::PtrComparator> memory_;

  Data* data_;
};

struct Term::Comparator {
  typedef Term value_type;

  bool operator()(const value_type& a, const value_type& b) const {
    return comp(a.data_, b.data_);
  }

 private:
  LessComparator<Data*> comp;
};

struct Term::Data::PtrComparator {
  typedef Term::Data* value_type;

  bool operator()(const value_type& a, const value_type& b) const {
    return comp(a->symbol_, a->args_,
                b->symbol_, b->args_);
  }

 private:
  LexicographicComparator<Symbol::Comparator,
                          LexicographicContainerComparator<Vector, Term::Comparator>> comp;
};

}  // namespace lela

#endif  // SRC_TERM_H_

