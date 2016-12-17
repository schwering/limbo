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
    c = c->Flatten(sf, tf);
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
    void prepend_not() { prefix_.push_front(Element{kNot}); }
    void append_not() { prefix_.push_back(Element{kNot}); }
    void prepend_exists(Term x) { prefix_.push_front(Element{kExists, x}); }

    size_t size() const { return prefix_.size(); }
    bool even() const { size_t n = 0; for (const auto& e : prefix_) { if (e.type == kNot) ++n; } return n % 2 == 0; }

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
  };

  explicit Formula(Type type) : type_(type) {}

  virtual TermSet FreeVars() const = 0;
  virtual TermSet SubTerms() const = 0;

  virtual void ISubstitute(const ISubstitution&, Term::Factory*) = 0;
  virtual void ITraverse(const ITraversal<Term>&)    const = 0;
  virtual void ITraverse(const ITraversal<Literal>&) const = 0;

  void Rectify(Symbol::Factory* sf, Term::Factory* tf) {
    TermMap tm;
    for (Term x : free_vars()) {
      tm.insert(std::make_pair(x, x));
    }
    // Rectify() renames every bound variable that also occurs free globally
    // somewhere in the formula or is bound by another quantifier to the left
    // of the current position.
    Rectify(&tm, sf, tf);
  }
  virtual void Rectify(TermMap*, Symbol::Factory*, Term::Factory*) = 0;

  virtual std::pair<QuantifierPrefix, const Formula*> quantifier_prefix() const = 0;

  virtual Ref Normalize() const = 0;

  //virtual Ref Simplify() const = 0;

  virtual Ref Flatten(Symbol::Factory*, Term::Factory*) const = 0;

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
      return t.variable() && ((it = tm->find(t)) != tm->end() && it->first != it->second)
          ? internal::Just(it->second) : internal::Nothing;
    }, tf);
  }

  virtual std::pair<QuantifierPrefix, const Formula*> quantifier_prefix() const override {
    return std::make_pair(QuantifierPrefix(), this);
  }

  Ref Normalize() const override { return Formula::Atomic(c_); }

  Ref Flatten(Symbol::Factory*, Term::Factory*) const override;

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

  virtual std::pair<QuantifierPrefix, const Formula*> quantifier_prefix() const override {
    auto p = phi_->quantifier_prefix();
    p.first.prepend_not();
    return p;
  }

  Ref Normalize() const override;

  Ref Flatten(Symbol::Factory* sf, Term::Factory* tf) const override { return Formula::Not(phi_->Flatten(sf, tf)); }

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

  virtual std::pair<QuantifierPrefix, const Formula*> quantifier_prefix() const override {
    return std::make_pair(QuantifierPrefix(), this);
  }

  Ref Normalize() const override;

  Ref Flatten(Symbol::Factory* sf, Term::Factory* tf) const override {
    return Formula::Or(lhs_->Flatten(sf, tf), rhs_->Flatten(sf, tf));
  }

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
    TermMap::const_iterator it = tm->find(x_);
    if (it != tm->end()) {
      const Term old_x = x_;
      const Term new_x = tf->CreateTerm(sf->CreateVariable(old_x.sort()));
      tm->insert(it, std::make_pair(old_x, new_x));
      x_ = new_x;
      phi_->Rectify(tm, sf, tf);
    }
  }

  virtual std::pair<QuantifierPrefix, const Formula*> quantifier_prefix() const override {
    auto p = phi_->quantifier_prefix();
    p.first.prepend_exists(x_);
    return p;
  }

  Ref Normalize() const override { return Formula::Exists(x_, phi_->Normalize()); }

  Ref Flatten(Symbol::Factory* sf, Term::Factory* tf) const override {
    return Formula::Exists(x_, phi_->Flatten(sf, tf));
  }

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
  QuantifierPrefix lp;
  QuantifierPrefix rp;
  const Formula* ls;
  const Formula* rs;
  std::tie(lp, ls) = l->quantifier_prefix();
  std::tie(rp, rs) = r->quantifier_prefix();
  if (ls->type() == kAtomic && (lp.even() || ls->as_atomic().arg().unit()) &&
      rs->type() == kAtomic && (rp.even() || rs->as_atomic().arg().unit())) {
    Clause lc = ls->as_atomic().arg();
    Clause rc = rs->as_atomic().arg();
    if (!lp.even()) {
      lp.append_not();
      lc = Clause({lc.get(0).flip()});
    }
    if (!rp.even()) {
      rp.append_not();
      rc = Clause({rc.get(0).flip()});
    }
    const auto lits = internal::join_ranges(lc.begin(), lc.end(), rc.begin(), rc.end());
    return lp.PrependTo(rp.PrependTo(Atomic(Clause(lits.begin(), lits.end()))));
  } else {
    return Formula::Or(std::move(l), std::move(r));
  }
}

Formula::Ref Formula::Atomic::Flatten(Symbol::Factory* sf, Term::Factory* tf) const {
  typedef std::unordered_set<Literal> LiteralSet;
  LiteralSet queue(arg().begin(), arg().end());
  TermMap term_to_var;
  for (Literal a : queue) {
    if (a.quasiprimitive() && a.rhs().variable()) {
      assert(a.lhs().function());
      term_to_var[a.rhs()] = a.lhs();
    }
  }
  LiteralSet lits;
  QuantifierPrefix vars;
  while (!queue.empty()) {
    auto it = queue.begin();
    Literal a = *it;
    queue.erase(it);
    if (a.quasiprimitive() || (!a.lhs().function() && !a.rhs().function())) {
      lits.insert(a);
    } else if (a.rhs().function()) {
      assert(a.lhs().function());
      Term old_rhs = a.rhs();
      Term new_rhs;
      TermMap::const_iterator it = term_to_var.find(old_rhs);
      if (it != term_to_var.end()) {
        new_rhs = it->second;
      } else {
        new_rhs = tf->CreateTerm(sf->CreateVariable(old_rhs.sort()));
        term_to_var[old_rhs] = new_rhs;
        vars.prepend_exists(new_rhs);
      }
      Literal new_a = a.Substitute(Term::SingleSubstitution(old_rhs, new_rhs), tf);
      Literal new_b = Literal::Neq(new_rhs, old_rhs);
      queue.insert(new_a);
      queue.insert(new_b);
    } else {
      assert(!a.lhs().quasiprimitive());
      for (Term arg : a.lhs().args()) {
        if (arg.function()) {
          Term old_arg = arg;
          Term new_arg;
          TermMap::const_iterator it = term_to_var.find(old_arg);
          if (it != term_to_var.end()) {
            new_arg = it->second;
          } else {
            new_arg = tf->CreateTerm(sf->CreateVariable(old_arg.sort()));
            term_to_var[old_arg] = new_arg;
            vars.prepend_exists(new_arg);
          }
          Literal new_a = a.Substitute(Term::SingleSubstitution(old_arg, new_arg), tf);
          Literal new_b = Literal::Neq(new_arg, old_arg);
          queue.insert(new_a);
          queue.insert(new_b);
        }
      }
    }
  }
  assert(lits.size() >= arg().size());
  assert(std::all_of(lits.begin(), lits.end(), [](Literal a) { return a.quasiprimitive(); }));
  if (vars.size() > 0) {
    vars.prepend_not();
    vars.append_not();
  }
  return vars.PrependTo(Formula::Atomic(Clause(lits.begin(), lits.end())));
}

}  // namespace lela

#endif  // LELA_FORMULA_H_

