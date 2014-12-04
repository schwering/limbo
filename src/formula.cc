// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <utility>
#include "./formula.h"
#include "./compar.h"

namespace esbl {

// {{{ Clausal Normal Form definition and conversion.

class Formula::Cnf {
 public:
  struct Comparator;
  class Disj;

  Cnf();
  explicit Cnf(const Disj& d);
  Cnf(const Cnf&);
  Cnf& operator=(const Cnf&);

  Cnf Substitute(const Unifier& theta) const;
  Cnf And(const Cnf& c) const;
  Cnf Or(const Cnf& c) const;

  bool operator==(const Cnf& c) const;

  void Minimize();

  bool ground() const;

  bool Eval(Setup* setup, split_level k) const;
  bool Eval(Setups* setups, split_level k) const;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

  const std::set<Disj, LessComparator<Disj>>& clauses() { return *ds_; }

  void Print(std::ostream* os) const;

 private:
  struct Equality;
  class KLiteral;
  class BLiteral;

  // Using unique_ptr prevents incomplete type errors.
  std::unique_ptr<std::set<Disj, LessComparator<Disj>>> ds_;
};

struct Formula::Cnf::Equality : public std::pair<Term, Term> {
 public:
  typedef std::set<Equality> Set;
  using std::pair<Term, Term>::pair;

  bool equal() const { return first == second; }
  bool ground() const { return first.ground() && second.ground(); }
};

class Formula::Cnf::KLiteral {
 public:
  struct Comparator;
  typedef std::set<KLiteral, Comparator> Set;

  KLiteral() = default;
  KLiteral(split_level k, const TermSeq& z, bool sign, const Cnf& phi)
      : k_(k), z_(z), sign_(sign), phi_(phi) {}
  KLiteral(const KLiteral&) = default;
  KLiteral& operator=(const KLiteral&) = default;

  bool operator==(const KLiteral& l) const {
    return z_ == l.z_ && phi_ == l.phi_ && sign_ == l.sign_ && k_ == l.k_;
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

  bool ground() const { return z_.ground() && phi_.ground(); }

 private:
  split_level k_;
  TermSeq z_;
  bool sign_;
  Cnf phi_;
};

class Formula::Cnf::BLiteral {
 public:
  struct Comparator;
  typedef std::set<BLiteral, Comparator> Set;

  BLiteral() = default;
  BLiteral(split_level k, const TermSeq& z, bool sign, const Cnf& phi,
           const Cnf& psi)
      : k_(k), z_(z), sign_(sign), phi_(phi), psi_(psi) {}
  BLiteral(const BLiteral&) = default;
  BLiteral& operator=(const BLiteral&) = default;

  bool operator==(const BLiteral& l) const {
    return z_ == l.z_ && phi_ == l.phi_ && psi_ == l.psi_ &&
        sign_ == l.sign_ && k_ == l.k_;
  }

  BLiteral Flip() const { return BLiteral(k_, z_, !sign_, phi_, psi_); }
  BLiteral Positive() const { return BLiteral(k_, z_, true, phi_, psi_); }
  BLiteral Negative() const { return BLiteral(k_, z_, false, phi_, psi_); }

  BLiteral Substitute(const Unifier& theta) const {
    return BLiteral(k_, z_.Substitute(theta), sign_,
                    phi_.Substitute(theta), psi_.Substitute(theta));
  }

  split_level k() const { return k_; }
  const TermSeq& z() const { return z_; }
  bool sign() const { return sign_; }
  const Cnf& phi() const { return phi_; }
  const Cnf& psi() const { return psi_; }

  bool ground() const {
    return z_.ground() && phi_.ground() && psi_.ground();
  }

 private:
  split_level k_;
  TermSeq z_;
  bool sign_;
  Cnf phi_;
  Cnf psi_;
};

struct Formula::Cnf::Comparator {
  typedef Cnf value_type;

  bool operator()(const Cnf& c, const Cnf& d) const;
};

struct Formula::Cnf::KLiteral::Comparator {
  typedef KLiteral value_type;

  bool operator()(const KLiteral& l1, const KLiteral& l2) const {
    return comp(l1.z_, l1.phi_, l1.sign_, l1.k_,
                l2.z_, l2.phi_, l2.sign_, l2.k_);
  }

 private:
  LexicographicComparator<LessComparator<TermSeq>,
                          Formula::Cnf::Comparator,
                          LessComparator<bool>,
                          LessComparator<split_level>> comp;
};

struct Formula::Cnf::BLiteral::Comparator {
  typedef BLiteral value_type;

  bool operator()(const BLiteral& l1, const BLiteral& l2) const {
    return comp(l1.z_, l1.phi_, l1.psi_, l1.sign_, l1.k_,
                l2.z_, l2.phi_, l2.psi_, l2.sign_, l2.k_);
  }

 private:
  LexicographicComparator<LessComparator<TermSeq>,
                          Formula::Cnf::Comparator,
                          Formula::Cnf::Comparator,
                          LessComparator<bool>,
                          LessComparator<split_level>> comp;
};

class Formula::Cnf::Disj {
 public:
  struct Comparator;
  typedef std::set<Disj, LessComparator<Disj>> Set;

  Disj() = default;
  Disj(const Disj&) = default;
  Disj& operator=(const Disj&) = default;

  static Disj Concat(const Disj& c1, const Disj& c2);
  static Maybe<Disj> Resolve(const Disj& d1, const Disj& d2);
  Disj Substitute(const Unifier& theta) const;

  bool Subsumes(const Disj& d) const;
  bool Tautologous() const;

  // Just forwards to Comparator::operator(). The only purpose is that this
  // allows Cnf to declare a set of Disj.
  bool operator<(const Formula::Cnf::Disj& d) const;

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

  void AddNested(split_level k, const TermSeq& z, bool sign, const Cnf& phi,
                 const Cnf& psi) {
    bs_.insert(BLiteral(k, z, sign, phi, psi));
  }

  bool ground() const;

  bool Eval(Setup* setup, split_level k) const;
  bool Eval(Setups* setups, split_level k) const;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

  void Print(std::ostream* os) const;

 private:
  Equality::Set eqs_;
  Equality::Set neqs_;
  SimpleClause c_;
  KLiteral::Set ks_;
  BLiteral::Set bs_;
};

struct Formula::Cnf::Disj::Comparator {
  typedef Disj value_type;

  bool operator()(const Disj& c, const Disj& d) const {
    const size_t n1 = c.eqs_.size() + c.neqs_.size() + c.c_.size() +
        c.ks_.size() + c.bs_.size();
    const size_t n2 = d.eqs_.size() + d.neqs_.size() + d.c_.size() +
        d.ks_.size() + d.bs_.size();
    return comp(n1, c.eqs_, c.neqs_, c.c_, c.ks_, c.bs_,
                n2, d.eqs_, d.neqs_, d.c_, d.ks_, d.bs_);
  }

 private:
  LexicographicComparator<LessComparator<size_t>,
                          LessComparator<Equality::Set>,
                          LessComparator<Equality::Set>,
                          SimpleClause::Comparator,
                          LexicographicContainerComparator<KLiteral::Set>,
                          LexicographicContainerComparator<BLiteral::Set>> comp;
};

bool Formula::Cnf::Comparator::operator()(const Cnf& c, const Cnf& d) const {
  LexicographicContainerComparator<Disj::Set> compar;
  return compar(*c.ds_, *d.ds_);
}

Formula::Cnf::Cnf() : ds_(new Disj::Set()) {}

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

bool Formula::Cnf::operator==(const Formula::Cnf& c) const {
  return *ds_ == *c.ds_;
}

void Formula::Cnf::Minimize() {
  Disj::Set new_ds;
  for (const Disj& d : *ds_) {
    assert(d.ground());
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

bool Formula::Cnf::ground() const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [](const Disj& d) { return d.ground(); });
}

void Formula::Cnf::AddToSetup(Setup* setup) const {
  for (const Disj& d : *ds_) {
    d.AddToSetup(setup);
  }
}

void Formula::Cnf::AddToSetups(Setups* setups) const {
  for (const Disj& d : *ds_) {
    d.AddToSetups(setups);
  }
}

bool Formula::Cnf::Eval(Setup* s, split_level k) const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [s, k](const Disj& d) { return d.Eval(s, k); });
}

bool Formula::Cnf::Eval(Setups* s, split_level k) const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [s, k](const Disj& d) { return d.Eval(s, k); });
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
  assert(d1.ground());
  assert(d2.ground());
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
  Comparator comp;
  return comp(*this, d);
}

bool Formula::Cnf::Disj::operator==(const Formula::Cnf::Disj& d) const {
  return eqs_ == d.eqs_ && neqs_ == d.neqs_ && c_ == d.c_ && ks_ == d.ks_ &&
      bs_ == d.bs_;
}

template<class T>
bool TautologousLiterals(const T& ls) {
  for (auto it = ls.begin(); it != ls.end(); ) {
    assert(ls.key_comp()(it->Negative(), it->Positive()));
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
  assert(ground());
  assert(d.ground());
  return std::includes(d.eqs_.begin(), d.eqs_.end(),
                       eqs_.begin(), eqs_.end(), neqs_.key_comp()) &&
      std::includes(d.neqs_.begin(), d.neqs_.end(),
                    neqs_.begin(), neqs_.end(), neqs_.key_comp()) &&
      std::includes(d.c_.begin(), d.c_.end(),
                    c_.begin(), c_.end(), c_.key_comp()) &&
      std::includes(d.ks_.begin(), d.ks_.end(),
                    ks_.begin(), ks_.end(), ks_.key_comp()) &&
      std::includes(d.bs_.begin(), d.bs_.end(),
                    bs_.begin(), bs_.end(), bs_.key_comp());
}

bool Formula::Cnf::Disj::Tautologous() const {
  assert(ground());
  return std::any_of(eqs_.begin(), eqs_.end(),
                     [](const Equality& e) { return e.equal(); }) ||
      std::any_of(neqs_.begin(), neqs_.end(),
                  [](const Equality& e) { return !e.equal(); }) ||
      TautologousLiterals(c_) ||
      TautologousLiterals(ks_) ||
      TautologousLiterals(bs_);
}

bool Formula::Cnf::Disj::ground() const {
  return std::all_of(eqs_.begin(), eqs_.end(),
                     [](const Equality& e) { return e.ground(); }) &&
      std::all_of(neqs_.begin(), neqs_.end(),
                  [](const Equality& e) { return e.ground(); }) &&
      c_.ground() &&
      std::all_of(ks_.begin(), ks_.end(),
                  [](const KLiteral& l) { return l.ground(); }) &&
      std::all_of(bs_.begin(), bs_.end(),
                  [](const BLiteral& l) { return l.ground(); });
}

void Formula::Cnf::Disj::AddToSetup(Setup* setup) const {
  assert(eqs_.empty() && neqs_.empty());
  assert(ks_.empty() && bs_.empty());
  setup->AddClause(Clause(Ewff::TRUE, c_));
}

void Formula::Cnf::Disj::AddToSetups(Setups* setups) const {
  assert(eqs_.empty() && neqs_.empty());
  assert(ks_.empty() && bs_.empty());
  setups->AddClause(Clause(Ewff::TRUE, c_));
}

bool Formula::Cnf::Disj::Eval(Setup* s, split_level k) const {
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
    if (l.phi().Eval(s, l.k())) {
      return true;
    }
  }
  return false;
}

bool Formula::Cnf::Disj::Eval(Setups* s, split_level k) const {
  if (Tautologous()) {
    return true;
  }
  if (s->Entails(c_, k)) {
    return true;
  }
  for (const KLiteral& l : ks_) {
    if (l.phi().Eval(s, l.k())) {
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
    *os << s << '[' << it->z() << ']' << "B_" << it->k() << "(";
    it->phi().Print(os);
    *os << " => ";
    it->psi().Print(os);
    *os << ')';
  }
  *os << ')';
}

// }}}
// {{{ Specializations of Formula, particularly MakeCnf() and Regress().

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

  void GroundInPlace(const Assignment& theta) override {
    t1 = t1.Ground(theta);
    t2 = t2.Ground(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    if (t1.is_variable()) {
      vs->insert(Variable(t1));
    }
    if (t2.is_variable()) {
      vs->insert(Variable(t2));
    }
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    if (t1.is_name()) {
      (*ns)[t1.sort()].insert(StdName(t1));
    }
    if (t2.is_name()) {
      (*ns)[t2.sort()].insert(StdName(t2));
    }
  }

  Ptr Reduce(Setup*, const StdName::SortedSet&) const override {
    return Copy();
  }

  Ptr Reduce(Setups*, const StdName::SortedSet&) const override {
    return Copy();
  }

  std::pair<Truth, Ptr> Simplify() const override {
    if ((t1.ground() && t2.ground()) || t1 == t2) {
      const Truth t = (t1 == t2) == sign ? TRIVIALLY_TRUE : TRIVIALLY_FALSE;
      return std::make_pair(t, Ptr());
    }
    return std::make_pair(NONTRIVIAL, Copy());
  }

  Cnf MakeCnf(const StdName::SortedSet&, StdName::SortedSet*) const override {
    Cnf::Disj d;
    if (sign) {
      d.AddEq(t1, t2);
    } else {
      d.AddNeq(t1, t2);
    }
    return Cnf(d);
  }

  Ptr Regress(Term::Factory*, const DynamicAxioms&) const override {
    return Copy();
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

  void GroundInPlace(const Assignment& theta) override {
    l = l.Ground(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    l.CollectVariables(vs);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    l.CollectNames(ns);
  }

  Ptr Reduce(Setup*, const StdName::SortedSet&) const override {
    return Copy();
  }

  Ptr Reduce(Setups*, const StdName::SortedSet&) const override {
    return Copy();
  }

  std::pair<Truth, Ptr> Simplify() const override {
    return std::make_pair(NONTRIVIAL, Copy());
  }

  Cnf MakeCnf(const StdName::SortedSet&, StdName::SortedSet*) const override {
    Cnf::Disj d;
    d.AddLiteral(l);
    return Cnf(d);
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Maybe<Ptr> phi = axioms.RegressOneStep(tf, static_cast<const Atom&>(l));
    if (!phi) {
      return Copy();
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
      : type(type), l(std::move(l)), r(std::move(r)) {
    assert(this->l);
    assert(this->r);
  }

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

  void GroundInPlace(const Assignment& theta) override {
    l->GroundInPlace(theta);
    r->GroundInPlace(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    // We assume formulas to be rectified, so that's OK. Otherwise, if x
    // occurred freely in l but bound in r, we need to take care not to delete
    // it with the second call.
    l->CollectFreeVariables(vs);
    r->CollectFreeVariables(vs);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    l->CollectNames(ns);
    r->CollectNames(ns);
  }

  Ptr Reduce(Setup* setup,
             const StdName::SortedSet& kb_and_query_ns) const override {
    return Ptr(new Junction(type,
                            l->Reduce(setup, kb_and_query_ns),
                            r->Reduce(setup, kb_and_query_ns)));
  }

  Ptr Reduce(Setups* setups,
             const StdName::SortedSet& kb_and_query_ns) const override {
    return Ptr(new Junction(type,
                            l->Reduce(setups, kb_and_query_ns),
                            r->Reduce(setups, kb_and_query_ns)));
  }

  std::pair<Truth, Ptr> Simplify() const override {
    auto p1 = l->Simplify();
    auto p2 = r->Simplify();
    if (type == DISJUNCTION) {
      if (p1.first == TRIVIALLY_TRUE || p2.first == TRIVIALLY_TRUE) {
        return std::make_pair(TRIVIALLY_TRUE, Ptr());
      }
      if (p1.first == TRIVIALLY_FALSE) {
        return p2;
      }
      if (p2.first == TRIVIALLY_FALSE) {
        return p1;
      }
    }
    if (type == CONJUNCTION) {
      if (p1.first == TRIVIALLY_FALSE || p2.first == TRIVIALLY_FALSE) {
        return std::make_pair(TRIVIALLY_FALSE, Ptr());
      }
      if (p1.first == TRIVIALLY_TRUE) {
        return p2;
      }
      if (p2.first == TRIVIALLY_TRUE) {
        return p1;
      }
    }
    assert(p1.first == NONTRIVIAL && p2.first == NONTRIVIAL);
    assert(p1.second);
    assert(p2.second);
    Ptr psi = Ptr(new Junction(type,
                               std::move(p1.second),
                               std::move(p2.second)));
    return std::make_pair(NONTRIVIAL, std::move(psi));
  }

  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
              StdName::SortedSet* placeholders) const override {
    const Cnf cnf_l = l->MakeCnf(kb_and_query_ns, placeholders);
    const Cnf cnf_r = r->MakeCnf(kb_and_query_ns, placeholders);
    if (type == DISJUNCTION) {
      return cnf_l.Or(cnf_r);
    } else {
      return cnf_l.And(cnf_r);
    }
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Ptr ll = l->Regress(tf, axioms);
    Ptr rr = r->Regress(tf, axioms);
    return Ptr(new Junction(type, std::move(ll), std::move(rr)));
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
      : type(type), x(x), phi(std::move(phi)) {
    assert(this->phi);
  }

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
    Unifier new_theta = theta;
    new_theta.erase(x);
    phi->SubstituteInPlace(theta);
  }

  void GroundInPlace(const Assignment& theta) override {
    Assignment new_theta = theta;
    new_theta.erase(x);
    phi->GroundInPlace(new_theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    phi->CollectFreeVariables(vs);
    vs->erase(x);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    phi->CollectNames(ns);
  }

  Ptr Reduce(Setup* setup,
             const StdName::SortedSet& kb_and_query_ns) const override {
    return Ptr(new Quantifier(type, x, phi->Reduce(setup, kb_and_query_ns)));
  }

  Ptr Reduce(Setups* setups,
             const StdName::SortedSet& kb_and_query_ns) const override {
    return Ptr(new Quantifier(type, x, phi->Reduce(setups, kb_and_query_ns)));
  }

  std::pair<Truth, Ptr> Simplify() const override {
    auto p = phi->Simplify();
    if (type == EXISTENTIAL && p.first == TRIVIALLY_TRUE) {
      return std::make_pair(TRIVIALLY_TRUE, Ptr());
    }
    if (type == UNIVERSAL && p.first == TRIVIALLY_FALSE) {
      return std::make_pair(TRIVIALLY_FALSE, Ptr());
    }
    assert(p.first == NONTRIVIAL);
    Ptr psi = Ptr(new Quantifier(type, x, std::move(p.second)));
    return std::make_pair(NONTRIVIAL, std::move(psi));
  }

  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
              StdName::SortedSet* placeholders) const override {
    placeholders->AddNewPlaceholder(x.sort());
    // Remember the names because the recursive call to MakeCnf() might add
    // additional names which must not be considered for this quantifier.
    auto new_ns = (*placeholders)[x.sort()];
    const auto it = kb_and_query_ns.find(x.sort());
    if (it != kb_and_query_ns.end()) {
      new_ns.insert(it->second.begin(), it->second.end());
    }
    const Cnf c = phi->MakeCnf(kb_and_query_ns, placeholders);
    bool init = false;
    Cnf r;
    for (const StdName& n : new_ns) {
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

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Ptr psi = phi->Regress(tf, axioms);
    const Variable y = tf->CreateVariable(x.sort());
    psi->SubstituteInPlace({{x, y}});
    return Ptr(new Quantifier(type, y, std::move(psi)));
  }

  void Print(std::ostream* os) const override {
    const char* s = type == EXISTENTIAL ? "E " : "";
    *os << '(' << s << x << ". " << *phi << ')';
  }
};

namespace {
void GenerateAssignments(const Variable::Set::const_iterator first,
                         const Variable::Set::const_iterator last,
                         const StdName::SortedSet& hplus,
                         StdName::SortedSet additional_names,
                         const Assignment& base_theta,
                         std::list<Assignment>* rs) {
  if (first == last) {
    rs->push_back(base_theta);
    return;
  }
  Variable::Set::const_iterator next = std::next(first);
  const Variable& x = *first;
  for (const StdName& n : hplus.lookup(x.sort())) {
    Assignment theta = base_theta;
    theta.insert(std::make_pair(x, n));
    GenerateAssignments(next, last, hplus, additional_names, theta, rs);
  }
  const StdName n_prime = additional_names.AddNewPlaceholder(x.sort());
  Assignment theta = base_theta;
  theta.insert(std::make_pair(x, n_prime));
  GenerateAssignments(next, last, hplus, additional_names, theta, rs);
}

std::list<Assignment> ReducedAssignments(const Variable::Set& vs,
                                         const StdName::SortedSet& ns) {
  std::list<Assignment> rs;
  StdName::SortedSet additional_names;
  GenerateAssignments(vs.begin(), vs.end(), ns, StdName::SortedSet(),
                      Assignment(), &rs);
  assert(!rs.empty());
  return rs;
}

Formula::Ptr ReducedFormula(const Assignment& theta) {
  std::vector<Formula::Ptr> conjs;
  for (const auto& p : theta) {
    const Variable& x = p.first;
    const StdName& m = p.second;
    if (!m.is_placeholder()) {
      conjs.push_back(Formula::Eq(x, m));
    } else {
      for (const auto& q : theta) {
        const Variable& y = q.first;
        const StdName& n = q.second;
        if (!n.is_placeholder()) {
          conjs.push_back(Formula::Neq(x, n));
        } else if (m == n) {
          conjs.push_back(Formula::Eq(x, y));
        }
      }
    }
  }
  Formula::Ptr phi = Formula::True();
  for (Formula::Ptr& psi : conjs) {
    phi = Formula::And(std::move(phi), std::move(psi));
  }
  return std::move(phi);
}
}

struct Formula::Knowledge : public Formula {
  split_level k;
  TermSeq z;
  bool sign;
  Ptr phi;

  Knowledge(split_level k, const TermSeq z, bool sign, Ptr phi)
      : k(k), z(z), sign(sign), phi(std::move(phi)) {
    assert(this->phi);
  }

  Ptr Copy() const override {
    return Ptr(new Knowledge(k, z, sign, phi->Copy()));
  }

  void Negate() override { sign = !sign; }

  void PrependActions(const TermSeq& prefix) override {
    z.insert(z.begin(), prefix.begin(), prefix.end());
  }

  void SubstituteInPlace(const Unifier& theta) override {
    phi->SubstituteInPlace(theta);
  }

  void GroundInPlace(const Assignment& theta) override {
    phi->GroundInPlace(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    phi->CollectFreeVariables(vs);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    phi->CollectNames(ns);
  }

  Ptr Reduce(Setup* setup,
             const StdName::SortedSet& kb_and_query_ns) const override {
    assert(z.empty());
    Variable::Set vs;
    CollectFreeVariables(&vs);
    std::vector<Ptr> disjs;
    for (const Assignment& theta : ReducedAssignments(vs, kb_and_query_ns)) {
      Ptr e = ReducedFormula(theta);
      assert(e);
      Ptr phi_copy = phi->Copy();
      phi_copy->GroundInPlace(theta);
      Truth truth;
      std::tie(truth, phi_copy) = phi_copy->Simplify();
      bool holds;
      switch (truth) {
        case TRIVIALLY_TRUE:
          holds = true;
          break;
        case TRIVIALLY_FALSE:
          holds = setup->Inconsistent(k);
          break;
        case NONTRIVIAL:
          Cnf cnf = phi_copy->MakeCnf(kb_and_query_ns);
          cnf.Minimize();
          holds = std::all_of(cnf.clauses().begin(), cnf.clauses().end(),
                              [&setup, this](const Cnf::Disj& c) {
                                return c.Eval(setup, k);
                              });
          break;
      }
      assert(e);
      disjs.push_back(And(std::move(e), holds ? True() : False()));
    }
    Formula::Ptr r;
    for (Formula::Ptr& disj : disjs) {
      r = !r ? std::move(disj) : Formula::Or(std::move(r), std::move(disj));
    }
    if (!sign) {
      r->Negate();
    }
    return std::move(r);
  }

  Ptr Reduce(Setups* setups,
             const StdName::SortedSet& kb_and_query_ns) const override {
    return Reduce(&setups->last_setup(), kb_and_query_ns);
  }

  std::pair<Truth, Ptr> Simplify() const override {
    auto p = phi->Simplify();
    if (p.first == TRIVIALLY_TRUE) {
      return std::make_pair(sign ? TRIVIALLY_TRUE : TRIVIALLY_FALSE, Ptr());
    } else if (p.first == TRIVIALLY_FALSE) {
      p.second = False();
    }
    assert(p.first == NONTRIVIAL);
    Ptr know = Ptr(new Knowledge(k, z, sign, std::move(p.second)));
    return std::make_pair(NONTRIVIAL, std::move(know));
  }

  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
              StdName::SortedSet* placeholders) const override {
    Cnf::Disj d;
    d.AddNested(k, z, sign, phi->MakeCnf(kb_and_query_ns, placeholders));
    return Cnf(d);
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Maybe<TermSeq, Term> zt = z.SplitLast();
    if (zt) {
      Ptr pos(new struct Lit(SfLiteral(zt.val1, zt.val2, true)));
      Ptr neg(new struct Lit(SfLiteral(zt.val1, zt.val2, false)));
      Ptr beta(Act(zt.val1,
        Or(And(pos->Copy(),
               Know(k, OnlyIf(pos->Copy(), Act(zt.val2, phi->Copy())))),
           And(neg->Copy(),
               Know(k, OnlyIf(neg->Copy(), Act(zt.val2, phi->Copy())))))));
      if (!sign) {
        beta->Negate();
      }
      return beta->Regress(tf, axioms);
    } else {
      assert(z.empty());
      return Ptr(new Knowledge(k, z, sign, phi->Regress(tf, axioms)));
    }
  }

  void Print(std::ostream* os) const override {
    const char* s = sign ? "" : "~";
    *os << '[' << z << ']' << s << "K_" << k << '(' << *phi << ')';
  }
};

struct Formula::Belief : public Formula {
  split_level k;
  TermSeq z;
  bool sign;
  Ptr phi;
  Ptr psi;

  Belief(split_level k, const TermSeq& z, bool sign, Ptr phi, Ptr psi)
      : k(k), z(z), sign(sign), phi(std::move(phi)),
        psi(std::move(psi)) {
    assert(this->phi);
    assert(this->psi);
  }

  Ptr Copy() const override {
    return Ptr(new Belief(k, z, sign, phi->Copy(), psi->Copy()));
  }

  void Negate() override { sign = !sign; }

  void PrependActions(const TermSeq& prefix) override {
    z.insert(z.begin(), prefix.begin(), prefix.end());
  }

  void SubstituteInPlace(const Unifier& theta) override {
    phi->SubstituteInPlace(theta);
    psi->SubstituteInPlace(theta);
  }

  void GroundInPlace(const Assignment& theta) override {
    phi->GroundInPlace(theta);
    psi->GroundInPlace(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    phi->CollectFreeVariables(vs);
    psi->CollectFreeVariables(vs);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    phi->CollectNames(ns);
    psi->CollectNames(ns);
  }

  Ptr Reduce(Setup*,
             const StdName::SortedSet&) const override {
    assert(false);
    return Ptr();
  }

  Ptr Reduce(Setups* setups,
             const StdName::SortedSet& kb_and_query_ns) const override {
    assert(z.empty());
    Variable::Set vs;
    CollectFreeVariables(&vs);
    std::vector<Ptr> disjs;
    for (const Assignment& theta : ReducedAssignments(vs, kb_and_query_ns)) {
      Ptr e = ReducedFormula(theta);
      assert(e);
      Ptr phi_copy = phi->Copy();
      Ptr psi_copy = psi->Copy();
      phi_copy->GroundInPlace(theta);
      psi_copy->GroundInPlace(theta);
      Truth phi_truth;
      Truth psi_truth;
      std::tie(phi_truth, phi_copy) = phi_copy->Simplify();
      std::tie(psi_truth, psi_copy) = psi_copy->Simplify();
      bool holds;
      switch (psi_truth) {
        case TRIVIALLY_TRUE:
          holds = true;
          break;
        case TRIVIALLY_FALSE:
          holds = setups->Inconsistent(k);
          break;
        case NONTRIVIAL:
          switch (phi_truth) {
            case TRIVIALLY_TRUE: {
              Cnf psi_cnf = psi_copy->MakeCnf(kb_and_query_ns);
              psi_cnf.Minimize();
              // To show True => psi, we look for the first consistent setup.
              for (size_t i = 0; i < setups->n_setups(); ++i) {
                Setup* s = &setups->setup(i);
                if (!s->Inconsistent(k)) {
                  holds = std::all_of(psi_cnf.clauses().begin(),
                                      psi_cnf.clauses().end(),
                                      [&s, this](const Cnf::Disj& c) {
                                        return c.Eval(s, k);
                                      });
                  break;
                }
              }
              break;
            }
            case TRIVIALLY_FALSE:
              holds = true;
              break;
            case NONTRIVIAL: {
              Ptr neg_phi = std::move(phi_copy);
              neg_phi->Negate();
              Cnf neg_phi_cnf = neg_phi->MakeCnf(kb_and_query_ns);
              Cnf psi_cnf = psi_copy->MakeCnf(kb_and_query_ns);
              neg_phi_cnf.Minimize();
              psi_cnf.Minimize();
              holds = true;
              // To show phi => psi, we look for the first setup which is
              // consistent with phi and then test whether it also entails psi.
              // A setup is consistent with phi iff it does not entail neg_phi.
              // Since neg_phi_cnf is a conjunction of clauses, this is true if
              // any of these clauses is not entailed.
              for (size_t i = 0; i < setups->n_setups(); ++i) {
                Setup* s = &setups->setup(i);
                const bool consistent_with_phi =
                    std::any_of(neg_phi_cnf.clauses().begin(),
                                 neg_phi_cnf.clauses().end(),
                                 [&s, this](const Cnf::Disj& c) {
                                   return !c.Eval(s, k);
                                 });
                if (consistent_with_phi) {
                  holds = std::all_of(psi_cnf.clauses().begin(),
                                      psi_cnf.clauses().end(),
                                      [&s, this](const Cnf::Disj& c) {
                                        return c.Eval(s, k);
                                      });
                  break;
                }
              }
              break;
            }
          }
          break;
      }
      assert(e);
      disjs.push_back(And(std::move(e), holds ? True() : False()));
    }
    Formula::Ptr r;
    for (Formula::Ptr& disj : disjs) {
      r = !r ? std::move(disj) : Formula::Or(std::move(r), std::move(disj));
    }
    if (!sign) {
      r->Negate();
    }
    return std::move(r);
  }


  std::pair<Truth, Ptr> Simplify() const override {
    auto p1 = phi->Simplify();
    auto p2 = psi->Simplify();
    if (p1.first == TRIVIALLY_TRUE) {
      p1.second = True();
    } else if (p1.first == TRIVIALLY_FALSE) {
      p1.second = False();
    }
    if (p2.first == TRIVIALLY_TRUE) {
      return std::make_pair(sign ? TRIVIALLY_TRUE : TRIVIALLY_FALSE, Ptr());
    } else if (p2.first == TRIVIALLY_FALSE) {
      p2.second = False();
    }
    assert(p1.second);
    assert(p2.second);
    Ptr b = Ptr(new Belief(k, z, sign, std::move(p1.second),
                           std::move(p2.second)));
    return std::make_pair(NONTRIVIAL, std::move(b));
  }

  Cnf MakeCnf(const StdName::SortedSet& kb_and_query_ns,
              StdName::SortedSet* placeholders) const override {
    Cnf::Disj d;
    d.AddNested(k, z, sign, phi->MakeCnf(kb_and_query_ns, placeholders),
                psi->MakeCnf(kb_and_query_ns, placeholders));
    return Cnf(d);
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Maybe<TermSeq, Term> zt = z.SplitLast();
    if (zt) {
      Ptr pos(new struct Lit(SfLiteral(zt.val1, zt.val2, true)));
      Ptr neg(new struct Lit(SfLiteral(zt.val1, zt.val2, false)));
      Ptr beta(Act(zt.val1,
        Or(And(pos->Copy(),
               Believe(k,
                       And(pos->Copy(), Act(zt.val2, phi->Copy())),
                       Act(zt.val2, psi->Copy()))),
           And(neg->Copy(),
               Believe(k,
                       And(neg->Copy(), Act(zt.val2, phi->Copy())),
                       Act(zt.val2, psi->Copy()))))));
      if (!sign) {
        beta->Negate();
      }
      return beta->Regress(tf, axioms);
    } else {
      assert(z.empty());
      return Ptr(new Belief(k, z, sign,
                            phi->Regress(tf, axioms),
                            psi->Regress(tf, axioms)));
    }
  }

  void Print(std::ostream* os) const override {
    const char* s = sign ? "" : "~";
    *os << '[' << z << ']' << s <<
        "B_" << k << '(' << *phi << " => " << *psi << ')';
  }
};

// }}}
// {{{ Formula members.

Formula::Ptr Formula::True() {
  const StdName n = Term::Factory::CreatePlaceholderStdName(0, 0);
  return Eq(n, n);
}

Formula::Ptr Formula::False() {
  return Neg(True());
}

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

Formula::Ptr Formula::Know(split_level k, Ptr phi) {
  return Ptr(new Knowledge(k, {}, true, std::move(phi)));
}

Formula::Ptr Formula::Believe(split_level k, Ptr psi) {
  return Believe(k, True(), std::move(psi));
}

Formula::Ptr Formula::Believe(split_level k, Ptr phi, Ptr psi) {
  return Ptr(new Belief(k, {}, true, std::move(phi), std::move(psi)));
}

Formula::Cnf Formula::MakeCnf(const StdName::SortedSet& kb_and_query_ns) const {
  StdName::SortedSet placeholders;
  return MakeCnf(kb_and_query_ns, &placeholders);
}

void Formula::AddToSetup(Setup* setup) const {
  StdName::SortedSet hplus = setup->hplus().WithoutPlaceholders();
  CollectNames(&hplus);
  std::pair<Truth, Ptr> p = Simplify();
  if (p.first == TRIVIALLY_TRUE) {
    return;
  }
  if (p.first == TRIVIALLY_FALSE) {
    setup->AddClause(Clause::EMPTY);
    return;
  }
  Ptr phi = std::move(p.second);
  assert(phi);
  Cnf cnf = phi->MakeCnf(hplus);
  cnf.Minimize();
  cnf.AddToSetup(setup);
}

void Formula::AddToSetups(Setups* setups) const {
  StdName::SortedSet hplus = setups->hplus().WithoutPlaceholders();
  CollectNames(&hplus);
  std::pair<Truth, Ptr> p = Simplify();
  if (p.first == TRIVIALLY_TRUE) {
    return;
  }
  if (p.first == TRIVIALLY_FALSE) {
    setups->AddClause(Clause::EMPTY);
    return;
  }
  Ptr phi = std::move(p.second);
  assert(phi);
  Cnf cnf = phi->MakeCnf(hplus);
  cnf.Minimize();
  cnf.AddToSetups(setups);
}

bool Formula::Eval(Setup* setup) const {
  StdName::SortedSet ns = setup->hplus().WithoutPlaceholders();
  CollectNames(&ns);
  Ptr phi = Reduce(setup, ns);
  Truth truth;
  std::tie(truth, phi) = phi->Simplify();
  if (truth == TRIVIALLY_TRUE) {
    return true;
  }
  if (truth == TRIVIALLY_FALSE) {
    return setup->Inconsistent(0);
  }
  assert(phi);
  Cnf cnf = phi->MakeCnf(ns);
  cnf.Minimize();
  return std::all_of(cnf.clauses().begin(), cnf.clauses().end(),
                     [&setup](const Cnf::Disj& c) {
                       return c.Eval(setup, 0);
                     });
}

bool Formula::Eval(Setups* setups) const {
  StdName::SortedSet ns = setups->hplus().WithoutPlaceholders();
  CollectNames(&ns);
  Truth truth;
  Ptr phi;
  std::tie(truth, phi) = Simplify();
  if (truth == TRIVIALLY_TRUE) {
    return true;
  }
  if (truth == TRIVIALLY_FALSE) {
    return setups->Inconsistent(0);
  }
  phi = Reduce(setups, ns);
  std::tie(truth, phi) = phi->Simplify();
  if (truth == TRIVIALLY_TRUE) {
    return true;
  }
  if (truth == TRIVIALLY_FALSE) {
    return setups->Inconsistent(0);
  }
  assert(phi);
  Cnf cnf = phi->MakeCnf(ns);
  cnf.Minimize();
  return std::all_of(cnf.clauses().begin(), cnf.clauses().end(),
                     [&setups](const Cnf::Disj& c) {
                       return c.Eval(&setups->last_setup(), 0);
                     });
}

std::ostream& operator<<(std::ostream& os, const Formula& phi) {
  phi.Print(&os);
  return os;
}

// }}}

}  // namespace esbl

