// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <utility>
#include <tuple>
#include "./formula.h"

namespace esbl {

class Formula::Cnf {
 public:
  class Disj;

  Cnf();
  explicit Cnf(const Disj& d);
  Cnf(const Cnf&);
  Cnf& operator=(const Cnf&);

  Cnf Substitute(const Unifier& theta) const;
  Cnf And(const Cnf& c) const;
  Cnf Or(const Cnf& c) const;

  bool operator<(const Cnf& c) const;
  bool operator==(const Cnf& c) const;

  void Minimize();

  bool is_ground() const;

  bool EntailedBy(Setup* setup, split_level k) const;
  bool EntailedBy(Setups* setups, split_level k) const;

  void Print(std::ostream* os) const;

 private:
  class KLiteral;
  class BLiteral;

  // unique_ptr prevents incomplete type errors
  std::unique_ptr<std::set<Disj>> ds_;
};

class Formula::Cnf::KLiteral {
 public:
  KLiteral() = default;
  KLiteral(split_level k, const TermSeq& z, bool sign, const Cnf& phi)
      : k_(k), z_(z), sign_(sign), phi_(phi) {}
  KLiteral(const KLiteral&) = default;
  KLiteral& operator=(const KLiteral&) = default;

  bool operator<(const KLiteral& l) const {
    return std::tie(z_, phi_, sign_, k_) <
        std::tie(l.z_, l.phi_, l.sign_, l.k_);
  }

  bool operator==(const KLiteral& l) const {
    return std::tie(z_, phi_, sign_, k_) ==
        std::tie(l.z_, l.phi_, l.sign_, l.k_);
  }

  KLiteral Flip() const { return KLiteral(k_, z_, !sign_, phi_); }
  KLiteral Positive() const { return KLiteral(k_, z_, true, phi_); }
  KLiteral Negative() const { return KLiteral(k_, z_, false, phi_); }

  KLiteral Substitute(const Unifier& theta) const {
    return KLiteral(k_, z_.Substitute(theta), sign_, phi_.Substitute(theta));
  }

  split_level k() const { return k_; }
  const TermSeq& z() const { return z_; }
  bool sign() const { return sign_; }
  const Cnf& phi() const { return phi_; }

  bool is_ground() const { return z_.is_ground() && phi_.is_ground(); }

 private:
  split_level k_;
  TermSeq z_;
  bool sign_;
  Cnf phi_;
};

class Formula::Cnf::BLiteral {
 public:
  BLiteral() = default;
  BLiteral(split_level k, const TermSeq& z, bool sign, const Cnf& neg_phi,
           const Cnf& psi)
      : k_(k), z_(z), sign_(sign), neg_phi_(neg_phi), psi_(psi) {}
  BLiteral(const BLiteral&) = default;
  BLiteral& operator=(const BLiteral&) = default;

  bool operator<(const BLiteral& l) const {
    return std::tie(z_, neg_phi_, psi_, sign_, k_) <
        std::tie(l.z_, l.neg_phi_, l.psi_, l.sign_, l.k_);
  }

  bool operator==(const BLiteral& l) const {
    return std::tie(z_, neg_phi_, psi_, sign_, k_) ==
        std::tie(l.z_, l.neg_phi_, l.psi_, l.sign_, l.k_);
  }

  BLiteral Flip() const { return BLiteral(k_, z_, !sign_, neg_phi_, psi_); }
  BLiteral Positive() const { return BLiteral(k_, z_, true, neg_phi_, psi_); }
  BLiteral Negative() const { return BLiteral(k_, z_, false, neg_phi_, psi_); }

  BLiteral Substitute(const Unifier& theta) const {
    return BLiteral(k_, z_.Substitute(theta), sign_,
                    neg_phi_.Substitute(theta), psi_.Substitute(theta));
  }

  split_level k() const { return k_; }
  const TermSeq& z() const { return z_; }
  bool sign() const { return sign_; }
  const Cnf& neg_phi() const { return neg_phi_; }
  const Cnf& psi() const { return psi_; }

  bool is_ground() const {
    return z_.is_ground() && neg_phi_.is_ground() && psi_.is_ground();
  }

 private:
  split_level k_;
  TermSeq z_;
  bool sign_;
  Cnf neg_phi_;
  Cnf psi_;
};

class Formula::Cnf::Disj {
 public:
  Disj() = default;
  Disj(const Disj&) = default;
  Disj& operator=(const Disj&) = default;

  static Disj Concat(const Disj& c1, const Disj& c2);
  static Maybe<Disj> Resolve(const Disj& d1, const Disj& d2);
  Disj Substitute(const Unifier& theta) const;

  bool Subsumes(const Disj& d) const;
  bool Tautologous() const;

  bool operator<(const Disj& d) const;
  bool operator==(const Disj& d) const;

  void AddEq(const Term& t1, const Term& t2) {
    eqs_.insert(std::make_pair(t1, t2));
  }

  void AddNeq(const Term& t1, const Term& t2) {
    neqs_.insert(std::make_pair(t1, t2));
  }

  void ClearEqs() { eqs_.clear(); }
  void ClearNeqs() { neqs_.clear(); }

  void AddLiteral(const Literal& l) {
    c_.insert(l);
  }

  void AddNested(split_level k, const TermSeq& z, bool sign, const Cnf& phi) {
    ks_.insert(KLiteral(k, z, sign, phi));
  }

  void AddNested(split_level k, const TermSeq& z, bool sign, const Cnf& neg_phi,
                 const Cnf& psi) {
    bs_.insert(BLiteral(k, z, sign, neg_phi, psi));
  }

  bool is_ground() const;

  bool EntailedBy(Setup* setup, split_level k) const;
  bool EntailedBy(Setups* setups, split_level k) const;

  void Print(std::ostream* os) const;

 private:
  std::set<std::pair<Term, Term>> eqs_;
  std::set<std::pair<Term, Term>> neqs_;
  SimpleClause c_;
  std::set<KLiteral> ks_;
  std::set<BLiteral> bs_;
};

struct Formula::Equal : public Formula {
  bool sign;
  Term t1;
  Term t2;

  Equal(const Term& t1, const Term& t2) : Equal(true, t1, t2) {}
  Equal(bool sign, const Term& t1, const Term& t2)
      : sign(sign), t1(t1), t2(t2) {}

  Ptr Copy() const override { return Ptr(new Equal(sign, t1, t2)); }

  void Negate() override { sign = !sign; }

  void PrependActions(const TermSeq&) override {}

  void SubstituteInPlace(const Unifier& theta) override {
    t1 = t1.Substitute(theta);
    t2 = t2.Substitute(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    if (t1.is_variable()) {
      (*vs)[t1.sort()].insert(Variable(t1));
    }
    if (t2.is_variable()) {
      (*vs)[t2.sort()].insert(Variable(t2));
    }
  }

  Cnf MakeCnf(StdName::SortedSet*) const override {
    Cnf::Disj d;
    if (sign) {
      d.AddEq(t1, t2);
    } else {
      d.AddNeq(t1, t2);
    }
    return Cnf(d);
  }

  Maybe<Ptr> Regress(Term::Factory*, const DynamicAxioms&) const override {
    return Just(Copy());
  }

  void Print(std::ostream* os) const override {
    const char* s = sign ? "=" : "!=";
    *os << '(' << t1 << ' ' << s << ' ' << t2 << ')';
  }
};

struct Formula::Lit : public Formula {
  Literal l;

  explicit Lit(const Literal& l) : l(l) {}

  Ptr Copy() const override { return Ptr(new Lit(l)); }

  void Negate() override { l = l.Flip(); }

  void PrependActions(const TermSeq& z) override { l = l.PrependActions(z); }

  void SubstituteInPlace(const Unifier& theta) override {
    l = l.Substitute(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    l.CollectVariables(vs);
  }

  Cnf MakeCnf(StdName::SortedSet*) const override {
    Cnf::Disj d;
    d.AddLiteral(l);
    return Cnf(d);
  }

  Maybe<Ptr> Regress(Term::Factory* tf,
                     const DynamicAxioms& axioms) const override {
    Maybe<Ptr> phi = axioms.RegressOneStep(tf, l);
    if (!phi) {
      return Just(Copy());
    }
    if (!l.sign()) {
      phi.val->Negate();
    }
    return phi.val->Regress(tf, axioms);
  }

  void Print(std::ostream* os) const override {
    *os << l;
  }
};

struct Formula::Junction : public Formula {
  enum Type { DISJUNCTION, CONJUNCTION };

  Type type;
  Ptr l;
  Ptr r;

  Junction(Type type, Ptr l, Ptr r)
      : type(type), l(std::move(l)), r(std::move(r)) {}

  Ptr Copy() const override {
    return Ptr(new Junction(type, l->Copy(), r->Copy()));
  }

  void Negate() override {
    type = type == DISJUNCTION ? CONJUNCTION : DISJUNCTION;
    l->Negate();
    r->Negate();
  }

  void PrependActions(const TermSeq& z) override {
    l->PrependActions(z);
    r->PrependActions(z);
  }

  void SubstituteInPlace(const Unifier& theta) override {
    l->SubstituteInPlace(theta);
    r->SubstituteInPlace(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    // We assume formulas to be rectified, so that's OK. Otherwise, if x
    // occurred freely in l but bound in r, we need to take care not to delete
    // it with the second call.
    l->CollectFreeVariables(vs);
    r->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(StdName::SortedSet* hplus) const override {
    const Cnf cnf_l = l->MakeCnf(hplus);
    const Cnf cnf_r = r->MakeCnf(hplus);
    if (type == DISJUNCTION) {
      return cnf_l.Or(cnf_r);
    } else {
      return cnf_l.And(cnf_r);
    }
  }

  Maybe<Ptr> Regress(Term::Factory* tf,
                     const DynamicAxioms& axioms) const override {
    Maybe<Ptr> ll = l->Regress(tf, axioms);
    Maybe<Ptr> rr = r->Regress(tf, axioms);
    if (!ll || !rr) {
      return Nothing;
    }
    return Just(Ptr(new Junction(type, std::move(ll.val), std::move(rr.val))));
  }

  void Print(std::ostream* os) const override {
    const char c = type == DISJUNCTION ? 'v' : '^';
    *os << '(' << *l << ' ' << c << ' ' << *r << ')';
  }
};

struct Formula::Quantifier : public Formula {
  enum Type { EXISTENTIAL, UNIVERSAL };

  Type type;
  Variable x;
  Ptr phi;

  Quantifier(Type type, const Variable& x, Ptr phi)
      : type(type), x(x), phi(std::move(phi)) {}

  Ptr Copy() const override {
    return Ptr(new Quantifier(type, x, phi->Copy()));
  }

  void Negate() override {
    type = type == EXISTENTIAL ? UNIVERSAL : EXISTENTIAL;
    phi->Negate();
  }

  void PrependActions(const TermSeq& z) override {
    assert(std::find(z.begin(), z.end(), x) == z.end());
    phi->PrependActions(z);
  }

  void SubstituteInPlace(const Unifier& theta) override {
    x = Variable(x.Substitute(theta));
    phi->SubstituteInPlace(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    phi->CollectFreeVariables(vs);
    (*vs)[x.sort()].erase(x);
  }

  Cnf MakeCnf(StdName::SortedSet* hplus) const override {
    std::set<StdName>& new_ns = (*hplus)[x.sort()];
    for (Term::Id id = 0; ; ++id) {
      assert(id <= static_cast<int>(new_ns.size()));
      const StdName n = Term::Factory::CreatePlaceholderStdName(id, x.sort());
      const auto p = new_ns.insert(n);
      if (p.second) {
        break;
      }
    }
    // Memorize names for this x because the recursive call might add additional
    // names which must not be substituted for this x.
    const std::set<StdName> this_ns = new_ns;
    const Cnf c = phi->MakeCnf(hplus);
    bool init = false;
    Cnf r;
    for (const StdName& n : this_ns) {
      const Cnf d = c.Substitute({{x, n}});
      if (!init) {
        r = d;
        init = true;
      } else if (type == EXISTENTIAL) {
        r = r.Or(d);
      } else {
        r = r.And(d);
      }
    }
    return r;
  }

  Maybe<Ptr> Regress(Term::Factory* tf,
                     const DynamicAxioms& axioms) const override {
    Maybe<Ptr> psi = phi->Regress(tf, axioms);
    if (!psi) {
      return Nothing;
    }
    const Variable y = tf->CreateVariable(x.sort());
    psi.val->SubstituteInPlace({{x, y}});
    return Just(Ptr(new Quantifier(type, y, std::move(psi.val))));
  }

  void Print(std::ostream* os) const override {
    const char* s = type == EXISTENTIAL ? "E " : "";
    *os << '(' << s << x << ". " << *phi << ')';
  }
};

struct Formula::Knowledge : public Formula {
  split_level k;
  TermSeq z;
  bool sign;
  Ptr phi;

  Knowledge(split_level k, Ptr phi) : k(k), sign(true), phi(std::move(phi)) {}

  Ptr Copy() const override { return Ptr(new Knowledge(k, phi->Copy())); }

  void Negate() override { sign = !sign; }

  void PrependActions(const TermSeq& prefix) override {
    z.insert(z.begin(), prefix.begin(), prefix.end());
  }

  void SubstituteInPlace(const Unifier& theta) override {
    phi->SubstituteInPlace(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    phi->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(StdName::SortedSet* hplus) const override {
    Cnf::Disj d;
    d.AddNested(k, z, sign, phi->MakeCnf(hplus));
    return Cnf(d);
  }

  Maybe<Ptr> Regress(Term::Factory*, const DynamicAxioms&) const override {
    return Nothing;
  }

  void Print(std::ostream* os) const override {
    *os << "K_" << k << '(' << *phi << ')';
  }
};

struct Formula::Belief : public Formula {
  split_level k;
  TermSeq z;
  bool sign;
  Ptr neg_phi;
  Ptr psi;

  Belief(split_level k, Ptr neg_phi, Ptr psi)
      : k(k), neg_phi(std::move(neg_phi)), psi(std::move(psi)) {}

  Ptr Copy() const override {
    return Ptr(new Belief(k, neg_phi->Copy(), psi->Copy()));
  }

  void Negate() override { sign = !sign; }

  void PrependActions(const TermSeq& prefix) override {
    z.insert(z.begin(), prefix.begin(), prefix.end());
  }

  void SubstituteInPlace(const Unifier& theta) override {
    neg_phi->SubstituteInPlace(theta);
    psi->SubstituteInPlace(theta);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    neg_phi->CollectFreeVariables(vs);
    psi->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(StdName::SortedSet* hplus) const override {
    Cnf::Disj d;
    d.AddNested(k, z, sign, neg_phi->MakeCnf(hplus), psi->MakeCnf(hplus));
    return Cnf(d);
  }

  Maybe<Ptr> Regress(Term::Factory*, const DynamicAxioms&) const override {
    return Nothing;
  }

  void Print(std::ostream* os) const override {
    *os << "K_" << k << '(' << '~' << *neg_phi << " => " << *psi << ')';
  }
};

Formula::Ptr Formula::Eq(const Term& t1, const Term& t2) {
  return Ptr(new Equal(t1, t2));
}

Formula::Ptr Formula::Neq(const Term& t1, const Term& t2) {
  Ptr eq(std::move(Eq(t1, t2)));
  eq->Negate();
  return std::move(eq);
}

Formula::Ptr Formula::Lit(const Literal& l) {
  return Ptr(new struct Lit(l));
}

Formula::Ptr Formula::Or(Ptr phi1, Ptr phi2) {
  return Ptr(new Junction(Junction::DISJUNCTION,
                          std::move(phi1),
                          std::move(phi2)));
}

Formula::Ptr Formula::And(Ptr phi1, Ptr phi2) {
  return Ptr(new Junction(Junction::CONJUNCTION,
                          std::move(phi1),
                          std::move(phi2)));
}

Formula::Ptr Formula::OnlyIf(Ptr phi1, Ptr phi2) {
  return Or(Neg(std::move(phi1)), std::move(phi2));
}

Formula::Ptr Formula::If(Ptr phi1, Ptr phi2) {
  return Or(Neg(std::move(phi2)), std::move(phi1));
}

Formula::Ptr Formula::Iff(Ptr phi1, Ptr phi2) {
  return And(If(std::move(phi1->Copy()), std::move(phi2->Copy())),
             OnlyIf(std::move(phi1), std::move(phi2)));
}

Formula::Ptr Formula::Neg(Ptr phi) {
  phi->Negate();
  return std::move(phi);
}

Formula::Ptr Formula::Act(const Term& t, Ptr phi) {
  return Act(TermSeq{t}, std::move(phi));
}

Formula::Ptr Formula::Act(const TermSeq& z, Ptr phi) {
  phi->PrependActions(z);
  return std::move(phi);
}

Formula::Ptr Formula::Exists(const Variable& x, Ptr phi) {
  return Ptr(new Quantifier(Quantifier::EXISTENTIAL, x, std::move(phi)));
}

Formula::Ptr Formula::Forall(const Variable& x, Ptr phi) {
  return Ptr(new Quantifier(Quantifier::UNIVERSAL, x, std::move(phi)));
}

bool Formula::EntailedBy(Term::Factory* tf, Setup* setup, split_level k) const {
  StdName::SortedSet hplus = tf->sorted_names();
  Cnf cnf = MakeCnf(&hplus);
  cnf.Minimize();
  return cnf.EntailedBy(setup, k);
}

bool Formula::EntailedBy(Term::Factory* tf, Setups* setups,
                         split_level k) const {
  StdName::SortedSet hplus = tf->sorted_names();
  Cnf cnf = MakeCnf(&hplus);
  cnf.Minimize();
  return cnf.EntailedBy(setups, k);
}

Formula::Cnf::Cnf() : ds_(new std::set<Formula::Cnf::Disj>()) {}

Formula::Cnf::Cnf(const Formula::Cnf::Disj& d) : Cnf() {
  ds_->insert(d);
}

Formula::Cnf::Cnf(const Cnf& c) : Cnf() {
  *ds_ = *c.ds_;
}

Formula::Cnf& Formula::Cnf::operator=(const Formula::Cnf& c) {
  *ds_ = *c.ds_;
  return *this;
}

Formula::Cnf Formula::Cnf::Substitute(const Unifier& theta) const {
  Cnf c;
  for (const Disj& d : *ds_) {
    c.ds_->insert(d.Substitute(theta));
  }
  return c;
}

Formula::Cnf Formula::Cnf::And(const Cnf& c) const {
  Cnf r = *this;
  r.ds_->insert(c.ds_->begin(), c.ds_->end());
  assert(r.ds_->size() <= ds_->size() + c.ds_->size());
  return r;
}

Formula::Cnf Formula::Cnf::Or(const Cnf& c) const {
  Cnf r;
  for (const Disj& d1 : *ds_) {
    for (const Disj& d2 : *c.ds_) {
      r.ds_->insert(Cnf::Disj::Concat(d1, d2));
    }
  }
  assert(r.ds_->size() <= ds_->size() * c.ds_->size());
  return r;
}

bool Formula::Cnf::operator<(const Formula::Cnf& c) const {
  return *ds_ < *c.ds_;
}

bool Formula::Cnf::operator==(const Formula::Cnf& c) const {
  return *ds_ == *c.ds_;
}

void Formula::Cnf::Minimize() {
  std::set<Disj> new_ds;
  for (const Disj& d : *ds_) {
    assert(d.is_ground());
    if (!d.Tautologous()) {
      Disj dd = d;
      dd.ClearEqs();
      dd.ClearNeqs();
      new_ds.insert(new_ds.end(), dd);
    }
  }
  std::swap(*ds_, new_ds);
  do {
    new_ds.clear();
    for (auto it = ds_->begin(); it != ds_->end(); ++it) {
      // Disj::operator< orders by clause length first, so subsumed clauses
      // are greater than the subsuming.
      for (auto jt = std::next(it); jt != ds_->end(); ) {
        if (it->Subsumes(*jt)) {
          jt = ds_->erase(jt);
        } else {
          Maybe<Disj> d = Disj::Resolve(*it, *jt);
          if (d) {
            new_ds.insert(d.val);
          }
          ++jt;
        }
      }
    }
    ds_->insert(new_ds.begin(), new_ds.end());
  } while (!new_ds.empty());
}

bool Formula::Cnf::is_ground() const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [](const Disj& d) { return d.is_ground(); });
}

bool Formula::Cnf::EntailedBy(Setup* s, split_level k) const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [s, k](const Disj& d) { return d.EntailedBy(s, k); });
}

bool Formula::Cnf::EntailedBy(Setups* s, split_level k) const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [s, k](const Disj& d) { return d.EntailedBy(s, k); });
}

void Formula::Cnf::Print(std::ostream* os) const {
  *os << '(';
  for (auto it = ds_->begin(); it != ds_->end(); ++it) {
    if (it != ds_->begin()) {
      *os << " ^ ";
    }
    it->Print(os);
  }
  *os << ')';
}

Formula::Cnf::Disj Formula::Cnf::Disj::Concat(const Disj& d1, const Disj& d2) {
  Disj d = d1;
  d.eqs_.insert(d2.eqs_.begin(), d2.eqs_.end());
  d.neqs_.insert(d2.neqs_.begin(), d2.neqs_.end());
  d.c_.insert(d2.c_.begin(), d2.c_.end());
  d.ks_.insert(d2.ks_.begin(), d2.ks_.end());
  d.bs_.insert(d2.bs_.begin(), d2.bs_.end());
  return d;
}

template<class T>
bool ResolveLiterals(T* lhs, const T& rhs) {
  for (const auto& l : rhs) {
    const auto it = lhs->find(l.Flip());
    if (it != lhs->end()) {
      lhs->erase(it);
      return true;
    }
  }
  return false;
}

Maybe<Formula::Cnf::Disj> Formula::Cnf::Disj::Resolve(const Disj& d1,
                                                      const Disj& d2) {
  assert(d1.eqs_.empty() && d1.neqs_.empty());
  assert(d2.eqs_.empty() && d2.neqs_.empty());
  assert(d1.is_ground());
  assert(d2.is_ground());
  if (d1.c_.size() + d1.ks_.size() + d1.bs_.size() >
      d2.c_.size() + d2.ks_.size() + d2.bs_.size()) {
    return Resolve(d2, d1);
  }
  Disj r = d2;
  if (ResolveLiterals(&r.c_, d1.c_) ||
      ResolveLiterals(&r.ks_, d1.ks_) ||
      ResolveLiterals(&r.bs_, d1.bs_)) {
    return Perhaps(!r.Tautologous(), r);
  }
  return Nothing;
}

Formula::Cnf::Disj Formula::Cnf::Disj::Substitute(const Unifier& theta) const {
  Disj d;
  for (const auto& p : eqs_) {
    d.eqs_.insert(std::make_pair(p.first.Substitute(theta),
                                 p.second.Substitute(theta)));
  }
  for (const auto& p : neqs_) {
    d.eqs_.insert(std::make_pair(p.first.Substitute(theta),
                                 p.second.Substitute(theta)));
  }
  d.c_ = c_.Substitute(theta);
  for (const auto& k : ks_) {
    d.ks_.insert(k.Substitute(theta));
  }
  for (const auto& b : bs_) {
    d.bs_.insert(b.Substitute(theta));
  }
  return d;
}

bool Formula::Cnf::Disj::operator<(const Formula::Cnf::Disj& d) const {
  // shortest disjunctions first
  const size_t n = eqs_.size() + neqs_.size() + c_.size() +
      ks_.size() + bs_.size();
  const size_t dn = d.eqs_.size() + d.neqs_.size() + d.c_.size() +
      d.ks_.size() + d.bs_.size();
  return std::tie(n, eqs_, neqs_, c_, ks_, bs_) <
      std::tie(dn, d.eqs_, d.neqs_, d.c_, d.ks_, d.bs_);
}

bool Formula::Cnf::Disj::operator==(const Formula::Cnf::Disj& d) const {
  return std::tie(eqs_, neqs_, c_, ks_, bs_) ==
      std::tie(d.eqs_, d.neqs_, d.c_, d.ks_, d.bs_);
}

template<class T>
bool TautologousLiterals(const T& ls) {
  for (auto it = ls.begin(); it != ls.end(); ) {
    assert(it->Negative() < it->Positive());
    const auto jt = std::next(it);
    assert(ls.find(it->Flip()) == ls.end() || ls.find(it->Flip()) == jt);
    if (jt != ls.end() && !it->sign() && *it == jt->Flip()) {
      return true;
    }
    it = jt;
  }
  return false;
}

bool Formula::Cnf::Disj::Subsumes(const Disj& d) const {
  assert(is_ground());
  assert(d.is_ground());
  return std::includes(d.eqs_.begin(), d.eqs_.end(),
                       eqs_.begin(), eqs_.end()) &&
      std::includes(d.neqs_.begin(), d.neqs_.end(),
                       neqs_.begin(), neqs_.end()) &&
      std::includes(d.c_.begin(), d.c_.end(),
                       c_.begin(), c_.end()) &&
      std::includes(d.ks_.begin(), d.ks_.end(),
                       ks_.begin(), ks_.end()) &&
      std::includes(d.bs_.begin(), d.bs_.end(),
                       bs_.begin(), bs_.end());
}

bool Formula::Cnf::Disj::Tautologous() const {
  assert(is_ground());
  auto eq = [](const std::pair<Term, Term>& p) { return p.first == p.second; };
  auto neq = [](const std::pair<Term, Term>& p) { return p.first != p.second; };
  return std::any_of(eqs_.begin(), eqs_.end(), eq) ||
      std::any_of(neqs_.begin(), neqs_.end(), neq) ||
      TautologousLiterals(c_) ||
      TautologousLiterals(ks_) ||
      TautologousLiterals(bs_);
}

bool Formula::Cnf::Disj::is_ground() const {
  const auto is_ground = [](const std::pair<Term, Term>& p) {
    return p.first.is_ground() && p.second.is_ground();
  };
  return std::all_of(eqs_.begin(), eqs_.end(), is_ground) &&
      std::all_of(neqs_.begin(), neqs_.end(), is_ground) &&
      c_.is_ground() &&
      std::all_of(ks_.begin(), ks_.end(),
                  [](const KLiteral& l) { return l.is_ground(); }) &&
      std::all_of(bs_.begin(), bs_.end(),
                  [](const BLiteral& l) { return l.is_ground(); });
}

bool Formula::Cnf::Disj::EntailedBy(Setup* s, split_level k) const {
  assert(bs_.empty());
  if (Tautologous()) {
    return true;
  }
  if (s->Entails(c_, k)) {
    return true;
  }
  for (const KLiteral& l : ks_) {
    // TODO(chs) (1) The negation of d.c_ should be added to the setup.
    // TODO(chs) (2) Or representation theorem instead of (1)?
    // That way, at least the SSA of knowledge/belief should be come out
    // correctly.
    if (l.phi().EntailedBy(s, l.k())) {
      return true;
    }
  }
  return false;
}

bool Formula::Cnf::Disj::EntailedBy(Setups* s, split_level k) const {
  if (Tautologous()) {
    return true;
  }
  if (s->Entails(c_, k)) {
    return true;
  }
  for (const KLiteral& l : ks_) {
    if (l.phi().EntailedBy(s, l.k())) {
      return true;
    }
  }
  // TODO(chs) ~neg_phi => psi, c.f. TODOs for Knowledge class; API changes
  // needed to account for ~neg_phi and psi at once.
  assert(bs_.empty());
  return false;
}

void Formula::Cnf::Disj::Print(std::ostream* os) const {
  *os << '(';
  for (auto it = eqs_.begin(); it != eqs_.end(); ++it) {
    if (it != eqs_.begin()) {
      *os << " v ";
    }
    *os << it->first << " = " << it->second;
  }
  for (auto it = neqs_.begin(); it != neqs_.end(); ++it) {
    if (!eqs_.empty() || it != neqs_.begin()) {
      *os << " v ";
    }
    *os << it->first << " != " << it->second;
  }
  for (auto it = c_.begin(); it != c_.end(); ++it) {
    if (!eqs_.empty() || !neqs_.empty() || it != c_.begin()) {
      *os << " v ";
    }
    *os << *it;
  }
  for (auto it = ks_.begin(); it != ks_.end(); ++it) {
    if (!eqs_.empty() || !neqs_.empty() || !c_.empty() ||
        it != ks_.begin()) {
      *os << " v ";
    }
    const char* s = it->sign() ? "" : "~";
    *os << s << "K_" << it->k() << '(';
    it->phi().Print(os);
    *os << ')';
  }
  for (auto it = bs_.begin(); it != bs_.end(); ++it) {
    if (!eqs_.empty() || !neqs_.empty() || !c_.empty() ||
        !ks_.empty() || it != bs_.begin()) {
      *os << " v ";
    }
    const char* s = it->sign() ? "" : "~";
    *os << s << '[' << it->z() << ']' << "B_" << it->k() << "(~";
    it->neg_phi().Print(os);
    *os << " => ";
    it->psi().Print(os);
    *os << ')';
  }
  *os << ')';
}

}  // namespace esbl

