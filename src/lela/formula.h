// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering
//
// Basic first-order formulas without any syntactic sugar. The atomic entities
// here are clauses, and the connectives are negation, disjunction, and
// existential quantifier. Formulas are immutable.
//
// Formulas can be accessed through Readers, which gives access to Element
// objects, which is either a Clause or a logical operator, which in case of an
// existential operator is parameterized with a (variable) Term. Readers and
// Elements are immutable.
//
// Substitute() and NF() are only well-behaved when the formula is rectified,
// that is, every variable is bound exactly by one quantifier. Rectify() can be
// used to ensure this condition holds.
//
// Readers are glorified range objects; their behaviour is only defined while
// the owning Formula is alive. NF() implements a normal form similar to
// negation normal form which however preserves clauses (and it turns
// clauses that are not explicitly represented as such into ones).
//
// Internally it's stored in Polish notation as a list of Element objects.
// Element would be a union if C++ would allow non-trivial types in unions.

#ifndef LELA_FORMULA_H_
#define LELA_FORMULA_H_

#include <cassert>

#include <list>
#include <memory>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <lela/clause.h>
#include <lela/internal/iter.h>
#include <lela/internal/maybe.h>

namespace lela {

class Formula {
 public:
  typedef std::unique_ptr<Formula> Ref;
  typedef std::unordered_set<Term> TermSet;

  enum Type { kAtomic, kNot, kOr, kExists };

  class Atomic;
  class Not;
  class Or;
  class Exists;

  static Ref Atomic(const Clause& c);
  static Ref Not(Ref phi);
  static Ref Or(Ref lhs, Ref rhs);
  static Ref Exists(Term lhs, Ref rhs);

  Formula(const Formula&) = delete;
  Formula& operator=(const Formula&) = delete;
  Formula(Formula&&) = default;
  Formula& operator=(Formula&&) = default;

  virtual ~Formula() {}

  virtual bool operator==(const Formula&) const = 0;
  bool operator!=(const Formula& that) const { return !(*this == that); }

  virtual Ref Clone() const = 0;

  Type type() const { return type_; }
  bool is_atomic() const { return type_ == kAtomic; }
  bool is_not() const { return type_ == kNot; }
  bool is_or() const { return type_ == kOr; }
  bool is_exists() const { return type_ == kExists; }

  const class Atomic& as_atomic() const;
  const class Not& as_not() const;
  const class Or& as_or() const;
  const class Exists& as_exists() const;

  const TermSet& free_vars() const {
    if (!free_vars_) {
      free_vars_ = internal::Just(FreeVars());
    }
    return free_vars_.val;
  }

  const TermSet& sub_terms() const {
    if (!sub_terms_) {
      sub_terms_ = internal::Just(SubTerms());
    }
    return sub_terms_.val;
  }

  template<typename UnaryFunction>
  void SubstituteFree(UnaryFunction theta, Term::Factory* tf) {
    struct FreeSubstitution : public ISubstitution {
      FreeSubstitution(UnaryFunction func) : func_(func) {}
      internal::Maybe<Term> operator()(Term t) const override { return !bound(t) ? func_(t) : internal::Nothing; }
     private:
      UnaryFunction func_;
    };
    ISubstitute(FreeSubstitution(theta), tf);
  }

  template<typename UnaryFunction>
  void Traverse(UnaryFunction f) const {
    typedef typename internal::remove_const_ref<typename internal::arg<UnaryFunction>::template type<0>>::type arg_type;
    struct Traversal : public ITraversal<arg_type> {
      Traversal(UnaryFunction func) : func_(func) {}
      bool operator()(arg_type t) const override { return func_(t); }
     private:
      UnaryFunction func_;
    };
    ITraverse(Traversal(f));
  }

  Ref NF(Symbol::Factory* sf, Term::Factory* tf) const {
    Ref c = Clone();
    c->Rectify(sf, tf);
    c = c->Normalize();
    return c;
  }

 protected:
  struct ISubstitution {
    virtual ~ISubstitution() {}
    virtual internal::Maybe<Term> operator()(Term t) const = 0;
    void Bind(Term t) const { bound_.insert(t); }
    void Unbind(Term t) const { bound_.erase(t); }
    bool bound(Term t) const { return bound_.find(t) != bound_.end(); }
   private:
    mutable TermSet bound_;
  };

  template<typename T>
  struct ITraversal {
    virtual ~ITraversal() {}
    virtual bool operator()(T t) const = 0;
  };

  typedef std::unordered_map<Term, Term> TermMap;

  struct QuantifierPrefix {
    explicit QuantifierPrefix(const Formula& suffix) : suffix_(suffix) {}
    void PrependNot() { prefix_.push_front(Element{kNot}); }
    void AppendNot() { prefix_.push_back(Element{kNot}); }
    void PrependExists(Term x) { prefix_.push_front(Element{kExists, x}); }

    bool even() const { size_t n = 0; for (const auto& e : prefix_) { if (e.type == kNot) ++n; } return n % 2 == 0; }

    const Formula& suffix() const { return suffix_; }

    Ref PrependTo(Ref phi) const {
      for (auto it = prefix_.rbegin(); it != prefix_.rend(); ++it) {
        assert(it->type == kNot || it->type == kExists);
        if (it->type == kNot) {
          phi = Not(std::move(phi));
        } else {
          phi = Exists(it->x, std::move(phi));
        }
      }
      return phi;
    }

   private:
    struct Element { Type type; Term x; };
    std::list<Element> prefix_;
    const Formula& suffix_;
  };

  explicit Formula(Type type) : type_(type) {}

  virtual TermSet FreeVars() const = 0;
  virtual TermSet SubTerms() const = 0;

  virtual void ISubstitute(const ISubstitution& theta, Term::Factory* tf) = 0;
  virtual void ITraverse(const ITraversal<Term>& f)    const = 0;
  virtual void ITraverse(const ITraversal<Literal>& f) const = 0;

  void Rectify(Symbol::Factory* sf, Term::Factory* tf) { TermMap tm; Rectify(&tm, sf, tf); }
  virtual void Rectify(TermMap* tm, Symbol::Factory* sf, Term::Factory* tf) = 0;

  virtual QuantifierPrefix quantifier_prefix() const = 0;

  //virtual Ref Flatten(Term::Factory* tf) const = 0;
  //virtual Ref Simplify() const = 0;

  virtual Ref Normalize() const = 0;

 private:
  Type type_;
  mutable internal::Maybe<TermSet> free_vars_ = internal::Nothing;
  mutable internal::Maybe<TermSet> sub_terms_ = internal::Nothing;
};

class Formula::Atomic : public Formula {
 public:
  explicit Atomic(const Clause& c) : Formula(kAtomic), c_(c) {}

  bool operator==(const Formula& that) const override { return type() == that.type() && c_ == that.as_atomic().c_; }

  Ref Clone() const override { return Ref(new Atomic(c_)); }

  const Clause& arg() const { return c_; }

 protected:
  TermSet FreeVars() const override {
    TermSet ts;
    c_.Traverse([&ts](Term x) { if (x.variable()) ts.insert(x); return true; });
    return ts;
  }

  TermSet SubTerms() const override {
    TermSet ts;
    c_.Traverse([&ts](Term x) { ts.insert(x); return true; });
    return ts;
  }

  void ISubstitute(const ISubstitution& theta, Term::Factory* tf) override {
    c_ = c_.Substitute([&theta](Term t) { return theta(t); }, tf);
  }
  void ITraverse(const ITraversal<Term>& f)    const override { c_.Traverse([&f](Term t) { return f(t); }); }
  void ITraverse(const ITraversal<Literal>& f) const override { c_.Traverse([&f](Literal a) { return f(a); }); }

  void Rectify(TermMap* tm, Symbol::Factory* sf, Term::Factory* tf) override {
    c_ = c_.Substitute([tm](Term t) {
      TermMap::const_iterator it;
      return t.variable() && ((it = tm->find(t)) != tm->end()) ? internal::Just(it->second) : internal::Nothing;
    }, tf);
  }

  virtual QuantifierPrefix quantifier_prefix() const override { return QuantifierPrefix(*this); }

  Ref Normalize() const override { return Formula::Atomic(c_); }

 private:
  Clause c_;
};

class Formula::Not : public Formula {
 public:
  explicit Not(Ref phi) : Formula(kNot), phi_(std::move(phi)) {}

  bool operator==(const Formula& that) const override { return type() == that.type() && *phi_ == *that.as_not().phi_; }

  Ref Clone() const override { return Formula::Not(phi_->Clone()); }

  const Formula& arg() const { return *phi_; }

 protected:
  TermSet FreeVars() const override { return phi_->FreeVars(); }
  TermSet SubTerms() const override { return phi_->SubTerms(); }

  void ISubstitute(const ISubstitution& theta, Term::Factory* tf) override { phi_->ISubstitute(theta, tf); }
  void ITraverse(const ITraversal<Term>& f)    const override { phi_->ITraverse(f); }
  void ITraverse(const ITraversal<Literal>& f) const override { phi_->ITraverse(f); }

  void Rectify(TermMap* tm, Symbol::Factory* sf, Term::Factory* tf) override { phi_->Rectify(tm, sf, tf); }

  virtual QuantifierPrefix quantifier_prefix() const override {
    QuantifierPrefix p = phi_->quantifier_prefix();
    p.PrependNot();
    return p;
  }

  Ref Normalize() const override;

 private:
  Ref phi_;
};

class Formula::Or : public Formula {
 public:
  Or(Ref lhs, Ref rhs) : Formula(kOr), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  bool operator==(const Formula& that) const override {
    return type() == that.type() && *lhs_ == *that.as_or().lhs_ && *rhs_ == *that.as_or().rhs_;
  }

  Ref Clone() const override { return Formula::Or(lhs_->Clone(), rhs_->Clone()); }

  const Formula& lhs() const { return *lhs_; }
  const Formula& rhs() const { return *rhs_; }

 protected:
  TermSet FreeVars() const override {
    TermSet ts1 = lhs_->FreeVars();
    const TermSet ts2 = rhs_->FreeVars();
    ts1.insert(ts2.begin(), ts2.end());
    return ts1;
  }

  TermSet SubTerms() const override {
    TermSet ts1 = lhs_->SubTerms();
    const TermSet ts2 = rhs_->SubTerms();
    ts1.insert(ts2.begin(), ts2.end());
    return ts2;
  }

  void ISubstitute(const ISubstitution& theta, Term::Factory* tf) override {
    lhs_->ISubstitute(theta, tf);
    rhs_->ISubstitute(theta, tf);
  }
  void ITraverse(const ITraversal<Term>& f)    const override { lhs_->ITraverse(f); rhs_->ITraverse(f); }
  void ITraverse(const ITraversal<Literal>& f) const override { lhs_->ITraverse(f); rhs_->ITraverse(f); }

  void Rectify(TermMap* tm, Symbol::Factory* sf, Term::Factory* tf) override {
    lhs_->Rectify(tm, sf, tf);
    rhs_->Rectify(tm, sf, tf);
  }

  virtual QuantifierPrefix quantifier_prefix() const override { return QuantifierPrefix(*this); }

  Ref Normalize() const override;

 private:
  Ref lhs_;
  Ref rhs_;
};

class Formula::Exists : public Formula {
 public:
  Exists(Term x, Ref phi) : Formula(kExists), x_(x), phi_(std::move(phi)) {}

  bool operator==(const Formula& that) const override {
    return type() == that.type() && x_ == that.as_exists().x_ && *phi_ == *that.as_exists().phi_;
  }

  Ref Clone() const override { return Formula::Exists(x_, phi_->Clone()); }

  Term x() const { return x_; }
  const Formula& arg() const { return *phi_; }

 protected:
  TermSet FreeVars() const override { TermSet ts = phi_->FreeVars(); ts.erase(x_); return ts; }
  TermSet SubTerms() const override { return phi_->SubTerms(); }

  void ISubstitute(const ISubstitution& theta, Term::Factory* tf) override {
    theta.Bind(x_);
    phi_->ISubstitute(theta, tf);
    theta.Unbind(x_);
  }
  void ITraverse(const ITraversal<Term>& f)    const override { phi_->ITraverse(f); }
  void ITraverse(const ITraversal<Literal>& f) const override { phi_->ITraverse(f); }

  void Rectify(TermMap* tm, Symbol::Factory* sf, Term::Factory* tf) override {
    Term old_x = x_;
    Term new_x = tf->CreateTerm(sf->CreateVariable(old_x.sort()));
    tm->insert(std::make_pair(old_x, new_x));
    x_ = new_x;
    phi_->Rectify(tm, sf, tf);
    tm->erase(old_x);
  }

  virtual QuantifierPrefix quantifier_prefix() const override {
    QuantifierPrefix p = phi_->quantifier_prefix();
    p.PrependExists(x_);
    return p;
  }

  Ref Normalize() const override { return Formula::Exists(x_, phi_->Normalize()); }

 private:
  Term x_;
  Ref phi_;
};

Formula::Ref Formula::Atomic(const Clause& c) { return Ref(new class Atomic(c)); }
Formula::Ref Formula::Not(Ref phi) { return Ref(new class Not(std::move(phi))); }
Formula::Ref Formula::Or(Ref lhs, Ref rhs) { return Ref(new class Or(std::move(lhs), std::move(rhs))); }
Formula::Ref Formula::Exists(Term x, Ref phi) { return Ref(new class Exists(x, std::move(phi))); }

const class Formula::Atomic& Formula::as_atomic() const {
  assert(type_ == kAtomic);
  return *dynamic_cast<const class Atomic*>(this);
}
const class Formula::Not& Formula::as_not() const {
  assert(type_ == kNot);
  return *dynamic_cast<const class Not*>(this);
}
const class Formula::Or& Formula::as_or() const {
  assert(type_ == kOr);
  return *dynamic_cast<const class Or*>(this);
}
const class Formula::Exists& Formula::as_exists() const {
  assert(type_ == kExists);
  return *dynamic_cast<const class Exists*>(this);
}


Formula::Ref Formula::Not::Normalize() const {
  switch (phi_->type()) {
    case kAtomic: {
      const Clause& c = arg().as_atomic().arg();
      if (c.unit()) {
        return Formula::Atomic(Clause({c.get(0).flip()}));
      } else {
        return Formula::Atomic(c);
      }
    }
    case kNot:
      return arg().as_not().arg().Normalize();
    case kOr:
      return Formula::Not(arg().Normalize());
    case kExists: {
      Term x = arg().as_exists().x();
      const Formula& phi = arg().as_exists().arg();
      return Formula::Not(Formula::Exists(x, phi.Normalize()));
    }
  }
}

Formula::Ref Formula::Or::Normalize() const {
  Ref l = lhs_->Normalize();
  Ref r = rhs_->Normalize();
  QuantifierPrefix lq = l->quantifier_prefix();
  QuantifierPrefix rq = r->quantifier_prefix();
  const Formula& ls = lq.suffix();
  const Formula& rs = rq.suffix();
  if (ls.type() == kAtomic && (lq.even() || ls.as_atomic().arg().unit()) &&
      rs.type() == kAtomic && (rq.even() || rs.as_atomic().arg().unit())) {
    Clause lc = ls.as_atomic().arg();
    Clause rc = rs.as_atomic().arg();
    if (!lq.even()) {
      lq.AppendNot();
      lc = Clause({lc.get(0).flip()});
    }
    if (!rq.even()) {
      rq.AppendNot();
      rc = Clause({rc.get(0).flip()});
    }
    const auto lits = internal::join_ranges(lc.begin(), lc.end(), rc.begin(), rc.end());
    return lq.PrependTo(rq.PrependTo(Atomic(Clause(lits.begin(), lits.end()))));
  } else {
    return Formula::Or(std::move(l), std::move(r));
  }
}

#if 0
class Formula {
 public:
  class Element {
   public:
    enum Type { kClause, kNot, kOr, kExists };

    static Element Clause(const lela::Clause& c) { return Element(kClause, c); }
    static Element Not() { return Element(kNot); }
    static Element Or() { return Element(kOr); }
    static Element Exists(Term var) { assert(var.variable()); return Element(kExists, var); }

    bool operator==(const Element& e) const {
      return type_ == e.type_ &&
            (type_ != kClause || clause_ == e.clause_) &&
            (type_ != kExists || var_ == e.var_);
    }
    bool operator!=(const Element& e) const { return !(*this == e); }

    Type type() const { return type_; }
    const internal::Maybe<lela::Clause>& clause() const { return clause_; }
    internal::Maybe<Term> var() const { return var_; }

   private:
    explicit Element(Type type) : type_(type) {}
    Element(Type type, Term var) : type_(type), var_(internal::Just(var)) {}
    Element(Type type, const lela::Clause& c) : type_(type), clause_(internal::Just(c)) {}

    Type type_;
    internal::Maybe<lela::Clause> clause_ = internal::Nothing;
    internal::Maybe<Term> var_ = internal::Nothing;
  };

  template<typename ForwardIt = std::list<Element>::const_iterator>
  class Reader {
   public:
    ForwardIt begin() const { return begin_; }
    ForwardIt end() const { return end_; }

    typename ForwardIt::reference head() const { return *begin(); }

    Reader arg() const {
      assert(head().type() == Element::kNot || head().type() == Element::kExists);
      return Reader(std::next(begin()));
    }

    Reader left() const {
      assert(head().type() == Element::kOr);
      return Reader(std::next(begin()));
    }

    Reader right() const {
      assert(head().type() == Element::kOr);
      return Reader(left().end());
    }

    Formula Build() const { return Formula(*this); }

    Formula Rectify(Symbol::Factory* sf, Term::Factory* tf) const;
    Formula Flatten(Term::Factory* tf) const;
    Formula Simplify() const;
    Formula NF() const;

    template<typename UnaryFunction>
    struct SubstituteElement {
      SubstituteElement() = default;
      SubstituteElement(UnaryFunction theta, Term::Factory* tf) : theta_(theta), tf_(tf) {}

      Element operator()(ForwardIt it) const {
        switch (it->type()) {
          case Element::kClause: return Element::Clause(it->clause().val.Substitute(theta_, tf_));
          case Element::kNot:    return *it;
          case Element::kOr:     return *it;
          case Element::kExists: assert(it->var().val.Substitute(theta_, tf_) == it->var().val); return *it;
        }
      }

     private:
      UnaryFunction theta_;
      Term::Factory* tf_;
    };

    template<typename UnaryFunction>
    Reader<internal::transform_iterator<ForwardIt, SubstituteElement<UnaryFunction>>>
    Substitute(UnaryFunction theta, Term::Factory* tf) const {
      typedef internal::transform_iterator<ForwardIt, SubstituteElement<UnaryFunction>> iterator;
      iterator it = iterator(begin(), SubstituteElement<UnaryFunction>(theta, tf));
      return Reader<iterator>(it);
    }

    template<typename UnaryFunction>
    void Traverse(UnaryFunction f) const {
      for (const Element& e : *this) {
        switch (e.type()) {
          case Element::kClause: e.clause().val.Traverse(f); break;
          case Element::kNot:    break;
          case Element::kOr:     break;
          case Element::kExists: break;
        }
      }
    }

    struct QuantifierPrefix {
      QuantifierPrefix(ForwardIt begin, ForwardIt end) : begin_(begin), end_(end) {}

      ForwardIt begin() const { return begin_; }
      ForwardIt end()   const { return end_; }

      bool even() const {
        return std::count_if(begin(), end(), [](const Element& e) { return e.type() == Element::kNot; }) % 2 == 0;
      }

      Reader<ForwardIt> suffix() const { return Reader(end_); }

     private:
      ForwardIt begin_;
      ForwardIt end_;
    };

    QuantifierPrefix quantifiers() const {
      auto last = std::find_if_not(begin(), end(), [](const Element& e) {
        return e.type() == Element::kNot || e.type() == Element::kExists;
      });
      return QuantifierPrefix(begin(), last);
    }

   private:
    friend class Formula;

    explicit Reader(ForwardIt begin) : begin_(begin), end_(begin) {
      for (int n = 1; n > 0; --n, ++end_) {
        switch ((*end_).type()) {
          case Element::kClause: n += 0; break;
          case Element::kNot:    n += 1; break;
          case Element::kOr:     n += 2; break;
          case Element::kExists: n += 1; break;
        }
      }
    }

    ForwardIt begin_;
    ForwardIt end_;
  };

  template<typename ForwardIt>
  explicit Formula(const Reader<ForwardIt>& r) : es_(r.begin(), r.end()) {}

  static Formula Clause(const lela::Clause& c) { return Atomic(Element::Clause(c)); }
  static Formula Not(const Formula& phi) { return Unary(Element::Not(), phi); }
  static Formula Or(const Formula& phi, const Formula& psi) { return Binary(Element::Or(), phi, psi); }
  static Formula Exists(Term var, const Formula& phi) { return Unary(Element::Exists(var), phi); }

  bool operator==(const Formula& phi) const { return es_ == phi.es_; }
  bool operator!=(const Formula& phi) const { return !(*this == phi); }

  Reader<> reader() const { return Reader<>(es_.begin()); }

 private:
  static Formula Atomic(Element op) {
    assert(op.type() == Element::kClause);
    Formula r;
    r.es_.push_front(op);
    return r;
  }

  static Formula Unary(Element op, Formula s) {
    assert(op.type() == Element::kNot || op.type() == Element::kExists);
    s.es_.push_front(op);
    return s;
  }

  static Formula Binary(Element op, Formula s, Formula r) {
    assert(op.type() == Element::kOr);
    r.es_.splice(r.es_.begin(), s.es_);
    r.es_.push_front(op);
    return r;
  }

  Formula() = default;

  std::list<Element> es_;
};

template<typename ForwardIt>
Formula Formula::Reader<ForwardIt>::Rectify(Symbol::Factory* sf, Term::Factory* tf) const {
  std::unordered_map<Term, Term> var_map;
  std::list<Element> es;
  for (auto e_it = begin(), last = end(); e_it != last; ++e_it) {
    const Element& e = *e_it;
    switch (e.type()) {
      case Element::kClause: {
        auto substitute = [&var_map](Term t) {
          auto it = var_map.find(t);
          return it != var_map.end() && t != it->second ? internal::Just(it->second) : internal::Nothing;
        };
        es.push_back(Element::Clause(e.clause().val.Substitute(substitute, tf)));
        break;
      }
      case Element::kNot: {
        es.push_back(e);
        break;
      }
      case Element::kOr: {
        es.push_back(e);
        break;
      }
      case Element::kExists: {
        const Term x = e.var().val;
        auto it = var_map.find(x);
        Term new_x;
        if (it == var_map.end()) {
          var_map.insert(std::make_pair(x, x));
          new_x = x;
        } else if (it != var_map.end()) {
          new_x = tf->CreateTerm(sf->CreateVariable(x.sort()));
          var_map.insert(std::make_pair(x, new_x));
        }
        es.push_back(Element::Exists(new_x));
        break;
      }
    }
  }
  assert(es.size() == Build().es_.size());
  Formula r;
  r.es_.splice(r.es_.begin(), es);
  return r;
}

template<typename ForwardIt>
Formula Formula::Reader<ForwardIt>::Flatten(Symbol::Factory* sf, Term::Factory* tf) const {
  Formula rect;
  std::unordered_map<Term, Term> var_map;
  std::list<Element> es;
  for (auto e_it = begin(), last = end(); e_it != last; ++e_it) {
    const Element& e = *e_it;
    switch (e.type()) {
      case Element::kClause: {
        std::pair<std::vector<Term>, Clause> 
        es.push_back(Element::Clause(e.clause().val.Substitute(substitute, tf)));
        break;
      }
      case Element::kNot: {
        es.push_back(e);
        break;
      }
      case Element::kOr: {
        es.push_back(e);
        break;
      }
      case Element::kExists: {
        es.push_back(e);
        break;
      }
    }
  }
  assert(es.size() == Build().es_.size());
  Formula r;
  r.es_.splice(r.es_.begin(), es);
  return r;
}

template<typename ForwardIt>
Formula Formula::Reader<ForwardIt>::NF() const {
  switch (head().type()) {
    case Element::kClause: {
      return Build();
    }
    case Element::kNot: {
      switch (arg().head().type()) {
        case Element::kClause: {
          const lela::Clause c = arg().head().clause().val;
          if (c.unit()) {
            return Clause(lela::Clause({c.begin()->flip()}));
          } else {
            return Clause(c);
          }
        }
        case Element::kOr:
          return Not(arg().NF());
        case Element::kNot:
          return arg().arg().NF();
        case Element::kExists:
          return Not(Exists(arg().head().var().val, arg().arg().NF()));
      }
    }
    case Element::kOr: {
      Formula phi = left().NF();
      Formula psi = right().NF();
      auto phi_quant = phi.reader().quantifiers();
      auto psi_quant = psi.reader().quantifiers();
      Reader phi_r = phi_quant.suffix();
      Reader psi_r = psi_quant.suffix();
      if (phi_r.head().type() == Element::kClause && (phi_quant.even() || phi_r.head().clause().val.unit()) &&
          psi_r.head().type() == Element::kClause && (psi_quant.even() || psi_r.head().clause().val.unit())) {
        std::list<Element> phi_p(phi_quant.begin(), phi_quant.end());
        std::list<Element> psi_p(psi_quant.begin(), psi_quant.end());
        lela::Clause phi_c = phi_r.head().clause().val;
        lela::Clause psi_c = psi_r.head().clause().val;
        if (!phi_quant.even()) {
          assert(phi_c.unit());
          phi_p.push_back(Element::Not());
          phi_c = lela::Clause({phi_c.begin()->flip()});
        }
        if (!psi_quant.even()) {
          assert(psi_c.unit());
          psi_p.push_back(Element::Not());
          psi_c = lela::Clause({psi_c.begin()->flip()});
        }
        auto ls = internal::join_ranges(phi_c.begin(), phi_c.end(), psi_c.begin(), psi_c.end());
        const lela::Clause c(ls.begin(), ls.end());
        Formula r;
        r.es_.push_front(Element::Clause(c));
        r.es_.splice(r.es_.begin(), psi_p);
        r.es_.splice(r.es_.begin(), phi_p);
        return r;
      } else {
        return Or(phi, psi);
      }
    }
    case Element::kExists: {
      return Exists(head().var().val, arg().NF());
    }
  }
}
#endif

}  // namespace lela

#endif  // LELA_FORMULA_H_

