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

  bool Eval(const Setup& setup, split_level k) const;
  bool Eval(const Setups& setups, split_level k) const;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

  const std::set<Disj, LessComparator<Disj>>& clauses() { return *ds_; }

  void Print(std::ostream* os) const;

 private:
  struct Equality;

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

struct Formula::Cnf::Comparator {
  typedef Cnf value_type;

  bool operator()(const Cnf& c, const Cnf& d) const;
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

  bool ground() const;

  bool Eval(const Setup& setup, split_level k) const;
  bool Eval(const Setups& setups, split_level k) const;

  void AddToSetup(Setup* setup) const;
  void AddToSetups(Setups* setups) const;

  void Print(std::ostream* os) const;

 private:
  Equality::Set eqs_;
  Equality::Set neqs_;
  SimpleClause c_;
};

struct Formula::Cnf::Disj::Comparator {
  typedef Disj value_type;

  bool operator()(const Disj& c, const Disj& d) const {
    const size_t n1 = c.eqs_.size() + c.neqs_.size() + c.c_.size();
    const size_t n2 = d.eqs_.size() + d.neqs_.size() + d.c_.size();
    return comp(n1, c.eqs_, c.neqs_, c.c_,
                n2, d.eqs_, d.neqs_, d.c_);
  }

 private:
  LexicographicComparator<LessComparator<size_t>,
                          LessComparator<Equality::Set>,
                          LessComparator<Equality::Set>,
                          SimpleClause::Comparator> comp;
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
  // The bool says that the prime implicate is essential.
  std::map<Disj, bool, LessComparator<Disj>> pis;
  for (const Disj& d : *ds_) {
    assert(d.ground());
    if (!d.Tautologous()) {
      Disj dd = d;
      dd.ClearEqs();
      dd.ClearNeqs();
      pis.insert(pis.end(), std::make_pair(dd, true));
    }
  }
  bool repeat;
  do {
    repeat = false;
    for (auto it = pis.begin(); it != pis.end(); ++it) {
      for (auto jt = pis.begin(); jt != it; ++jt) {
        Maybe<Disj> d = Disj::Resolve(it->first, jt->first);
        if (d) {
          const auto p = pis.insert(std::make_pair(d.val, false));
          p.first->second = !(it->second || it->second);
          repeat |= p.second;
        }
      }
    }
  } while (repeat);
  // Disj::operator< orders by clause length first, so subsumed clauses
  // are greater than the subsuming.
  for (auto it = pis.rbegin(); it != pis.rend(); ++it) {
    for (auto jt = pis.rbegin(); jt != it; ++jt) {
      if (!it->second && jt->second && it->first.Subsumes(jt->first)) {
        it->second = true;
        jt->second = false;
      }
    }
  }
  ds_->clear();
  for (const auto& p : pis) {
    if (p.second) {
      ds_->insert(ds_->end(), p.first);
    }
  }
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

bool Formula::Cnf::Eval(const Setup& s, split_level k) const {
  return std::all_of(ds_->begin(), ds_->end(),
                     [s, k](const Disj& d) { return d.Eval(s, k); });
}

bool Formula::Cnf::Eval(const Setups& s, split_level k) const {
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
  return d;
}

template<class T>
bool ResolveLiterals(T* lhs, const T& rhs) {
  bool succ = false;
  for (const auto& l : rhs) {
    const auto it = lhs->find(l.Flip());
    if (it != lhs->end()) {
      lhs->erase(it);
      succ = true;
    } else {
      lhs->insert(l);
    }
  }
  return succ;
}

Maybe<Formula::Cnf::Disj> Formula::Cnf::Disj::Resolve(const Disj& d1,
                                                      const Disj& d2) {
  assert(d1.eqs_.empty() && d1.neqs_.empty());
  assert(d2.eqs_.empty() && d2.neqs_.empty());
  assert(d1.ground());
  assert(d2.ground());
  if (d1.c_.size() > d2.c_.size()) {
    return Resolve(d2, d1);
  }
  Disj r = d2;
  if (ResolveLiterals(&r.c_, d1.c_)) {
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
  return d;
}

bool Formula::Cnf::Disj::operator<(const Formula::Cnf::Disj& d) const {
  Comparator comp;
  return comp(*this, d);
}

bool Formula::Cnf::Disj::operator==(const Formula::Cnf::Disj& d) const {
  return eqs_ == d.eqs_ && neqs_ == d.neqs_ && c_ == d.c_;
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
                    c_.begin(), c_.end(), c_.key_comp());
}

bool Formula::Cnf::Disj::Tautologous() const {
  assert(ground());
  return std::any_of(eqs_.begin(), eqs_.end(),
                     [](const Equality& e) { return e.equal(); }) ||
      std::any_of(neqs_.begin(), neqs_.end(),
                  [](const Equality& e) { return !e.equal(); }) ||
      TautologousLiterals(c_);
}

bool Formula::Cnf::Disj::ground() const {
  return std::all_of(eqs_.begin(), eqs_.end(),
                     [](const Equality& e) { return e.ground(); }) &&
      std::all_of(neqs_.begin(), neqs_.end(),
                  [](const Equality& e) { return e.ground(); }) &&
      c_.ground();
}

void Formula::Cnf::Disj::AddToSetup(Setup* setup) const {
  assert(eqs_.empty() && neqs_.empty());
  setup->AddClause(Clause(Ewff::TRUE, c_));
}

void Formula::Cnf::Disj::AddToSetups(Setups* setups) const {
  assert(eqs_.empty() && neqs_.empty());
  setups->AddClause(Clause(Ewff::TRUE, c_));
}

bool Formula::Cnf::Disj::Eval(const Setup& s, split_level k) const {
  if (Tautologous()) {
    return true;
  }
  if (s.Entails(c_, k)) {
    return true;
  }
  return false;
}

bool Formula::Cnf::Disj::Eval(const Setups& s, split_level k) const {
  if (Tautologous()) {
    return true;
  }
  if (s.Entails(c_, k)) {
    return true;
  }
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
  *os << ')';
}

// }}}
// {{{ Specializations of Formula, particularly MakeCnf() and Regress().

struct Formula::Obj::Equal : public Formula::Obj {
  bool sign;
  Term t1;
  Term t2;

  Equal(const Term& t1, const Term& t2) : Equal(true, t1, t2) {}
  Equal(bool sign, const Term& t1, const Term& t2)
      : sign(sign), t1(t1), t2(t2) {}

  ObjPtr ObjCopy() const override { return ObjPtr(new Equal(sign, t1, t2)); }

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

  ObjPtr Reduce(const Setup&, const StdName::SortedSet&) const override {
    return ObjCopy();
  }

  ObjPtr Reduce(const Setups&, const StdName::SortedSet&) const override {
    return ObjCopy();
  }

  std::pair<Truth, ObjPtr> Simplify() const override {
    if ((t1.ground() && t2.ground()) || t1 == t2) {
      const Truth t = (t1 == t2) == sign ? TRIVIALLY_TRUE : TRIVIALLY_FALSE;
      return std::make_pair(t, ObjPtr());
    }
    return std::make_pair(NONTRIVIAL, ObjCopy());
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

  ObjPtr ObjRegress(Term::Factory*, const DynamicAxioms&) const override {
    return ObjCopy();
  }

  void Print(std::ostream* os) const override {
    const char* s = sign ? "=" : "!=";
    *os << '(' << t1 << ' ' << s << ' ' << t2 << ')';
  }
};

struct Formula::Obj::Lit : public Formula::Obj {
  Literal l;

  explicit Lit(const Literal& l) : l(l) {}

  ObjPtr ObjCopy() const override { return ObjPtr(new Lit(l)); }

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

  ObjPtr Reduce(const Setup&, const StdName::SortedSet&) const override {
    return ObjCopy();
  }

  ObjPtr Reduce(const Setups&, const StdName::SortedSet&) const override {
    return ObjCopy();
  }

  std::pair<Truth, ObjPtr> Simplify() const override {
    return std::make_pair(NONTRIVIAL, ObjCopy());
  }

  Cnf MakeCnf(const StdName::SortedSet&, StdName::SortedSet*) const override {
    Cnf::Disj d;
    d.AddLiteral(l);
    return Cnf(d);
  }

  ObjPtr ObjRegress(Term::Factory* tf,
                    const DynamicAxioms& axioms) const override {
    Maybe<ObjPtr> phi = axioms.RegressOneStep(tf, static_cast<const Atom&>(l));
    if (!phi) {
      return ObjCopy();
    }
    if (!l.sign()) {
      phi.val->Negate();
    }
    return phi.val->ObjRegress(tf, axioms);
  }

  void Print(std::ostream* os) const override {
    *os << l;
  }
};

template<class BaseFormula>
struct Formula::BaseJunction : public BaseFormula {
  enum Type { DISJUNCTION, CONJUNCTION };

  Type type;

  BaseJunction(Type type) : type(type) {}
  virtual ~BaseJunction() {}

  void Negate() override {
    type = type == DISJUNCTION ? CONJUNCTION : DISJUNCTION;
    get_l()->Negate();
    get_r()->Negate();
  }

  void PrependActions(const TermSeq& z) override {
    get_l()->PrependActions(z);
    get_r()->PrependActions(z);
  }

  void SubstituteInPlace(const Unifier& theta) override {
    get_l()->SubstituteInPlace(theta);
    get_r()->SubstituteInPlace(theta);
  }

  void GroundInPlace(const Assignment& theta) override {
    get_l()->GroundInPlace(theta);
    get_r()->GroundInPlace(theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    // We assume formulas to be rectified, so that's OK. Otherwise, if x
    // occurred freely in l but bound in r, we need to take care not to delete
    // it with the second call.
    get_l()->CollectFreeVariables(vs);
    get_r()->CollectFreeVariables(vs);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    get_l()->CollectNames(ns);
    get_r()->CollectNames(ns);
  }

  ObjPtr Reduce(const Setup& setup,
                const StdName::SortedSet& kb_and_query_ns) const override;
  ObjPtr Reduce(const Setups& setups,
                const StdName::SortedSet& kb_and_query_ns) const override;

  void Print(std::ostream* os) const override {
    const char c = type == DISJUNCTION ? 'v' : '^';
    *os << '(' << *get_l() << ' ' << c << ' ' << *get_r() << ')';
  }

 protected:
  virtual Formula* get_l() = 0;
  virtual Formula* get_r() = 0;
  virtual const Formula* get_l() const = 0;
  virtual const Formula* get_r() const = 0;
};

struct Formula::Junction : public Formula::BaseJunction<Formula> {
  Ptr l;
  Ptr r;

  Junction(Type type, Ptr l, Ptr r)
      : BaseJunction(type), l(std::move(l)), r(std::move(r)) {}

  Ptr Copy() const override {
    return Ptr(new Junction(type, l->Copy(), r->Copy()));
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Ptr ll = get_l()->Regress(tf, axioms);
    Ptr rr = get_r()->Regress(tf, axioms);
    return Ptr(new Junction(type, std::move(ll), std::move(rr)));
  }

 protected:
  virtual Formula* get_l() override { return l.get(); }
  virtual Formula* get_r() override { return r.get(); }
  virtual const Formula* get_l() const override { return l.get(); }
  virtual const Formula* get_r() const override { return r.get(); }
};

struct Formula::Obj::Junction : public Formula::BaseJunction<Formula::Obj> {
  ObjPtr l;
  ObjPtr r;

  Junction(Type type, ObjPtr l, ObjPtr r)
      : BaseJunction(type), l(std::move(l)), r(std::move(r)) {}

  ObjPtr ObjCopy() const override {
    return ObjPtr(new Junction(type, l->ObjCopy(), r->ObjCopy()));
  }

  ObjPtr ObjRegress(Term::Factory* tf,
                    const DynamicAxioms& axioms) const override {
    ObjPtr ll = l->ObjRegress(tf, axioms);
    ObjPtr rr = r->ObjRegress(tf, axioms);
    return ObjPtr(new Junction(type, std::move(ll), std::move(rr)));
  }

  std::pair<Truth, ObjPtr> Simplify() const override {
    auto p1 = l->Simplify();
    auto p2 = r->Simplify();
    if (type == DISJUNCTION) {
      if (p1.first == TRIVIALLY_TRUE || p2.first == TRIVIALLY_TRUE) {
        return std::make_pair(TRIVIALLY_TRUE, ObjPtr());
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
        return std::make_pair(TRIVIALLY_FALSE, ObjPtr());
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
    ObjPtr psi(new Junction(type, std::move(p1.second), std::move(p2.second)));
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

 protected:
  virtual Formula* get_l() override { return l.get(); }
  virtual Formula* get_r() override { return r.get(); }
  virtual const Formula* get_l() const override { return l.get(); }
  virtual const Formula* get_r() const override { return r.get(); }
};

template<class BaseFormula>
Formula::ObjPtr Formula::BaseJunction<BaseFormula>::Reduce(
    const Setup& setup, const StdName::SortedSet& kb_and_query_ns) const {
  auto new_type = type == CONJUNCTION
      ? Obj::Junction::CONJUNCTION
      : Obj::Junction::DISJUNCTION;
  return ObjPtr(new Obj::Junction(new_type,
                                  get_l()->Reduce(setup, kb_and_query_ns),
                                  get_r()->Reduce(setup, kb_and_query_ns)));
}

template<class BaseFormula>
Formula::ObjPtr Formula::BaseJunction<BaseFormula>::Reduce(
    const Setups& setups, const StdName::SortedSet& kb_and_query_ns) const {
  auto new_type = type == CONJUNCTION
      ? Obj::Junction::CONJUNCTION
      : Obj::Junction::DISJUNCTION;
  return ObjPtr(new Obj::Junction(new_type,
                                  get_l()->Reduce(setups, kb_and_query_ns),
                                  get_r()->Reduce(setups, kb_and_query_ns)));
}

template<class BaseFormula>
struct Formula::BaseQuantifier : public BaseFormula {
  enum Type { EXISTENTIAL, UNIVERSAL };

  Type type;
  Variable x;

  BaseQuantifier(Type type, const Variable& x) : type(type), x(x) {}
  virtual ~BaseQuantifier() {}

  void Negate() override {
    type = type == EXISTENTIAL ? UNIVERSAL : EXISTENTIAL;
    get_phi()->Negate();
  }

  void PrependActions(const TermSeq& z) override {
    assert(std::find(z.begin(), z.end(), x) == z.end());
    get_phi()->PrependActions(z);
  }

  void SubstituteInPlace(const Unifier& theta) override {
    Unifier new_theta = theta;
    new_theta.erase(x);
    get_phi()->SubstituteInPlace(theta);
  }

  void GroundInPlace(const Assignment& theta) override {
    Assignment new_theta = theta;
    new_theta.erase(x);
    get_phi()->GroundInPlace(new_theta);
  }

  void CollectFreeVariables(Variable::Set* vs) const override {
    get_phi()->CollectFreeVariables(vs);
    vs->erase(x);
  }

  void CollectNames(StdName::SortedSet* ns) const override {
    get_phi()->CollectNames(ns);
  }

  ObjPtr Reduce(const Setup& setup,
                const StdName::SortedSet& kb_and_query_ns) const override;
  ObjPtr Reduce(const Setups& setups,
                const StdName::SortedSet& kb_and_query_ns) const override;

  void Print(std::ostream* os) const override {
    const char* s = type == EXISTENTIAL ? "E " : "";
    *os << '(' << s << x << ". " << *get_phi() << ')';
  }

 protected:
  virtual Formula* get_phi() = 0;
  virtual const Formula* get_phi() const = 0;
};

struct Formula::Quantifier : public Formula::BaseQuantifier<Formula> {
  Ptr phi;

  Quantifier(Type type, const Variable& x, Ptr phi)
      : BaseQuantifier(type, x), phi(std::move(phi)) {}

  Ptr Copy() const override {
    return Ptr(new Quantifier(type, x, phi->Copy()));
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Ptr psi = get_phi()->Regress(tf, axioms);
    const Variable y = tf->CreateVariable(x.sort());
    psi->SubstituteInPlace({{x, y}});
    return Ptr(new Quantifier(type, y, std::move(psi)));
  }

 protected:
  virtual Formula* get_phi() override { return phi.get(); }
  virtual const Formula* get_phi() const override { return phi.get(); }
};

struct Formula::Obj::Quantifier : public Formula::BaseQuantifier<Formula::Obj> {
  ObjPtr phi;

  Quantifier(Type type, const Variable& x, ObjPtr phi)
      : BaseQuantifier(type, x), phi(std::move(phi)) {}

  ObjPtr ObjCopy() const override {
    return ObjPtr(new Quantifier(type, x, phi->ObjCopy()));
  }

  ObjPtr ObjRegress(Term::Factory* tf,
                    const DynamicAxioms& axioms) const override {
    ObjPtr psi = phi->ObjRegress(tf, axioms);
    const Variable y = tf->CreateVariable(x.sort());
    psi->SubstituteInPlace({{x, y}});
    return ObjPtr(new Quantifier(type, y, std::move(psi)));
  }

  std::pair<Truth, ObjPtr> Simplify() const override {
    auto p = phi->Simplify();
    if (type == EXISTENTIAL && p.first == TRIVIALLY_TRUE) {
      return std::make_pair(TRIVIALLY_TRUE, ObjPtr());
    }
    if (type == UNIVERSAL && p.first == TRIVIALLY_FALSE) {
      return std::make_pair(TRIVIALLY_FALSE, ObjPtr());
    }
    assert(p.first == NONTRIVIAL);
    ObjPtr psi(new Quantifier(type, x, std::move(p.second)));
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

 protected:
  virtual Formula* get_phi() override { return phi.get(); }
  virtual const Formula* get_phi() const override { return phi.get(); }
};

template<class BaseFormula>
Formula::ObjPtr Formula::BaseQuantifier<BaseFormula>::Reduce(
    const Setup& setup, const StdName::SortedSet& kb_and_query_ns) const {
  auto new_type = type == EXISTENTIAL
      ? Obj::Quantifier::EXISTENTIAL
      : Obj::Quantifier::UNIVERSAL;
  return ObjPtr(new Obj::Quantifier(new_type, x,
                                    get_phi()->Reduce(setup, kb_and_query_ns)));
}

template<class BaseFormula>
Formula::ObjPtr Formula::BaseQuantifier<BaseFormula>::Reduce(
    const Setups& setups, const StdName::SortedSet& kb_and_query_ns) const {
  auto new_type = type == EXISTENTIAL
      ? Obj::Quantifier::EXISTENTIAL
      : Obj::Quantifier::UNIVERSAL;
  return ObjPtr(new Obj::Quantifier(new_type, x,
                                    get_phi()->Reduce(setups, kb_and_query_ns)));
}

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

Formula::ObjPtr ReducedFormula(const Assignment& theta) {
  std::vector<Formula::ObjPtr> conjs;
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
  Formula::ObjPtr phi = Formula::True();
  for (Formula::ObjPtr& psi : conjs) {
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

  ObjPtr Reduce(const Setup& setup,
                const StdName::SortedSet& kb_and_query_ns) const override {
    assert(z.empty());
    Variable::Set vs;
    CollectFreeVariables(&vs);
    std::vector<ObjPtr> disjs;
    for (const Assignment& theta : ReducedAssignments(vs, kb_and_query_ns)) {
      ObjPtr e = ReducedFormula(theta);
      assert(e);
      ObjPtr phi_red = phi->Reduce(setup, kb_and_query_ns);
      phi_red->GroundInPlace(theta);
      Truth truth;
      std::tie(truth, phi_red) = phi_red->Simplify();
      bool holds;
      switch (truth) {
        case TRIVIALLY_TRUE:
          holds = true;
          break;
        case TRIVIALLY_FALSE:
          holds = setup.Inconsistent(k);
          break;
        case NONTRIVIAL:
          Cnf cnf = phi_red->MakeCnf(kb_and_query_ns);
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
    Formula::ObjPtr r;
    for (Formula::ObjPtr& disj : disjs) {
      r = !r ? std::move(disj) : Formula::Or(std::move(r), std::move(disj));
    }
    if (!sign) {
      r->Negate();
    }
    return std::move(r);
  }

  ObjPtr Reduce(const Setups& setups,
                const StdName::SortedSet& kb_and_query_ns) const override {
    return Reduce(setups.last_setup(), kb_and_query_ns);
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Maybe<TermSeq, Term> zt = z.SplitLast();
    if (zt) {
      ObjPtr pos(new Obj::Lit(SfLiteral(zt.val1, zt.val2, true)));
      ObjPtr neg(new Obj::Lit(SfLiteral(zt.val1, zt.val2, false)));
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

  ObjPtr Reduce(const Setup&,
                const StdName::SortedSet&) const override {
    assert(false);
    return ObjPtr();
  }

  ObjPtr Reduce(const Setups& setups,
                const StdName::SortedSet& kb_and_query_ns) const override {
    assert(z.empty());
    Variable::Set vs;
    CollectFreeVariables(&vs);
    std::vector<ObjPtr> disjs;
    for (const Assignment& theta : ReducedAssignments(vs, kb_and_query_ns)) {
      ObjPtr e = ReducedFormula(theta);
      assert(e);
      ObjPtr phi_red = phi->Reduce(setups, kb_and_query_ns);
      ObjPtr psi_red = psi->Reduce(setups, kb_and_query_ns);
      phi_red->GroundInPlace(theta);
      psi_red->GroundInPlace(theta);
      Truth phi_truth;
      Truth psi_truth;
      std::tie(phi_truth, phi_red) = phi_red->Simplify();
      std::tie(psi_truth, psi_red) = psi_red->Simplify();
      bool holds;
      switch (psi_truth) {
        case TRIVIALLY_TRUE:
          holds = true;
          break;
        case TRIVIALLY_FALSE:
          holds = setups.Inconsistent(k);
          break;
        case NONTRIVIAL:
          switch (phi_truth) {
            case TRIVIALLY_TRUE: {
              Cnf psi_cnf = psi_red->MakeCnf(kb_and_query_ns);
              psi_cnf.Minimize();
              // To show True => psi, we look for the first consistent setup.
              for (size_t i = 0; i < setups.n_setups(); ++i) {
                const Setup& s = setups.setup(i);
                if (!s.Inconsistent(k)) {
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
              ObjPtr neg_phi = std::move(phi_red);
              neg_phi->Negate();
              Cnf neg_phi_cnf = neg_phi->MakeCnf(kb_and_query_ns);
              Cnf psi_cnf = psi_red->MakeCnf(kb_and_query_ns);
              neg_phi_cnf.Minimize();
              psi_cnf.Minimize();
              holds = true;
              // To show phi => psi, we look for the first setup which is
              // consistent with phi and then test whether it also entails psi.
              // A setup is consistent with phi iff it does not entail neg_phi.
              // Since neg_phi_cnf is a conjunction of clauses, this is true if
              // any of these clauses is not entailed.
              for (size_t i = 0; i < setups.n_setups(); ++i) {
                const Setup& s = setups.setup(i);
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
    Formula::ObjPtr r;
    for (Formula::ObjPtr& disj : disjs) {
      r = !r ? std::move(disj) : Formula::Or(std::move(r), std::move(disj));
    }
    if (!sign) {
      r->Negate();
    }
    return std::move(r);
  }

  Ptr Regress(Term::Factory* tf, const DynamicAxioms& axioms) const override {
    Maybe<TermSeq, Term> zt = z.SplitLast();
    if (zt) {
      ObjPtr pos(new Obj::Lit(SfLiteral(zt.val1, zt.val2, true)));
      ObjPtr neg(new Obj::Lit(SfLiteral(zt.val1, zt.val2, false)));
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

Formula::ObjPtr Formula::True() {
  const StdName n = StdName();
  return Eq(n, n);
}

Formula::ObjPtr Formula::False() {
  return Neg(True());
}

Formula::ObjPtr Formula::Eq(const Term& t1, const Term& t2) {
  return ObjPtr(new Obj::Equal(t1, t2));
}

Formula::ObjPtr Formula::Neq(const Term& t1, const Term& t2) {
  ObjPtr eq(std::move(Eq(t1, t2)));
  eq->Negate();
  return std::move(eq);
}

Formula::ObjPtr Formula::Lit(const Literal& l) {
  return ObjPtr(new Obj::Lit(l));
}

Formula::ObjPtr Formula::Or(ObjPtr phi1, ObjPtr phi2) {
  return ObjPtr(new Obj::Junction(Obj::Junction::DISJUNCTION,
                                  std::move(phi1),
                                  std::move(phi2)));
}

Formula::Ptr Formula::Or(Ptr phi1, Ptr phi2) {
  return Ptr(new Junction(Junction::DISJUNCTION,
                          std::move(phi1),
                          std::move(phi2)));
}

Formula::ObjPtr Formula::And(ObjPtr phi1, ObjPtr phi2) {
  return ObjPtr(new Obj::Junction(Obj::Junction::CONJUNCTION,
                                  std::move(phi1),
                                  std::move(phi2)));
}

Formula::Ptr Formula::And(Ptr phi1, Ptr phi2) {
  return Ptr(new Junction(Junction::CONJUNCTION,
                          std::move(phi1),
                          std::move(phi2)));
}

Formula::ObjPtr Formula::OnlyIf(ObjPtr phi1, ObjPtr phi2) {
  return Or(Neg(std::move(phi1)), std::move(phi2));
}

Formula::Ptr Formula::OnlyIf(Ptr phi1, Ptr phi2) {
  return Or(Neg(std::move(phi1)), std::move(phi2));
}

Formula::ObjPtr Formula::If(ObjPtr phi1, ObjPtr phi2) {
  return Or(Neg(std::move(phi2)), std::move(phi1));
}

Formula::Ptr Formula::If(Ptr phi1, Ptr phi2) {
  return Or(Neg(std::move(phi2)), std::move(phi1));
}

Formula::ObjPtr Formula::Iff(ObjPtr phi1, ObjPtr phi2) {
  return And(If(std::move(phi1->ObjCopy()), std::move(phi2->ObjCopy())),
             OnlyIf(std::move(phi1), std::move(phi2)));
}

Formula::Ptr Formula::Iff(Ptr phi1, Ptr phi2) {
  return And(If(std::move(phi1->Copy()), std::move(phi2->Copy())),
             OnlyIf(std::move(phi1), std::move(phi2)));
}

Formula::ObjPtr Formula::Neg(ObjPtr phi) {
  phi->Negate();
  return std::move(phi);
}

Formula::Ptr Formula::Neg(Ptr phi) {
  phi->Negate();
  return std::move(phi);
}

Formula::ObjPtr Formula::Act(const Term& t, ObjPtr phi) {
  return Act(TermSeq{t}, std::move(phi));
}

Formula::Ptr Formula::Act(const Term& t, Ptr phi) {
  return Act(TermSeq{t}, std::move(phi));
}

Formula::ObjPtr Formula::Act(const TermSeq& z, ObjPtr phi) {
  phi->PrependActions(z);
  return std::move(phi);
}

Formula::Ptr Formula::Act(const TermSeq& z, Ptr phi) {
  phi->PrependActions(z);
  return std::move(phi);
}

Formula::ObjPtr Formula::Exists(const Variable& x, ObjPtr phi) {
  return ObjPtr(new Obj::Quantifier(Obj::Quantifier::EXISTENTIAL, x,
                                    std::move(phi)));
}

Formula::Ptr Formula::Exists(const Variable& x, Ptr phi) {
  return Ptr(new Quantifier(Quantifier::EXISTENTIAL, x, std::move(phi)));
}

Formula::ObjPtr Formula::Forall(const Variable& x, ObjPtr phi) {
  return ObjPtr(new Obj::Quantifier(Obj::Quantifier::UNIVERSAL, x,
                                    std::move(phi)));
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

Formula::Cnf Formula::Obj::MakeCnf(
    const StdName::SortedSet& kb_and_query_ns) const {
  StdName::SortedSet placeholders;
  return MakeCnf(kb_and_query_ns, &placeholders);
}

bool Formula::Eval(const Setup& setup) const {
  StdName::SortedSet ns = setup.hplus().WithoutPlaceholders();
  CollectNames(&ns);
  ObjPtr phi = Reduce(setup, ns);
  Truth truth;
  std::tie(truth, phi) = phi->Simplify();
  if (truth == TRIVIALLY_TRUE) {
    return true;
  }
  if (truth == TRIVIALLY_FALSE) {
    return setup.Inconsistent(0);
  }
  assert(phi);
  Cnf cnf = phi->MakeCnf(ns);
  cnf.Minimize();
  return std::all_of(cnf.clauses().begin(), cnf.clauses().end(),
                     [&setup](const Cnf::Disj& c) {
                       return c.Eval(setup, 0);
                     });
}

bool Formula::Eval(const Setups& setups) const {
  StdName::SortedSet ns = setups.hplus().WithoutPlaceholders();
  CollectNames(&ns);
  Truth truth;
  ObjPtr phi = Reduce(setups, ns);
  std::tie(truth, phi) = phi->Simplify();
  if (truth == TRIVIALLY_TRUE) {
    return true;
  }
  if (truth == TRIVIALLY_FALSE) {
    return setups.Inconsistent(0);
  }
  assert(phi);
  Cnf cnf = phi->MakeCnf(ns);
  cnf.Minimize();
  return std::all_of(cnf.clauses().begin(), cnf.clauses().end(),
                     [&setups](const Cnf::Disj& c) {
                       return c.Eval(setups.last_setup(), 0);
                     });
}

Formula::Ptr Formula::Obj::Copy() const {
  return ObjCopy();
}

Formula::Ptr Formula::Obj::Regress(Term::Factory* tf,
                                   const DynamicAxioms& axioms) const {
  return ObjRegress(tf, axioms);
}

void Formula::Obj::AddToSetup(Setup* setup) const {
  StdName::SortedSet hplus = setup->hplus().WithoutPlaceholders();
  CollectNames(&hplus);
  std::pair<Truth, ObjPtr> p = Simplify();
  if (p.first == TRIVIALLY_TRUE) {
    return;
  }
  if (p.first == TRIVIALLY_FALSE) {
    setup->AddClause(Clause::EMPTY);
    return;
  }
  ObjPtr phi = std::move(p.second);
  assert(phi);
  Cnf cnf = phi->MakeCnf(hplus);
  cnf.Minimize();
  cnf.AddToSetup(setup);
}

void Formula::Obj::AddToSetups(Setups* setups) const {
  StdName::SortedSet hplus = setups->hplus().WithoutPlaceholders();
  CollectNames(&hplus);
  std::pair<Truth, ObjPtr> p = Simplify();
  if (p.first == TRIVIALLY_TRUE) {
    return;
  }
  if (p.first == TRIVIALLY_FALSE) {
    setups->AddClause(Clause::EMPTY);
    return;
  }
  ObjPtr phi = std::move(p.second);
  assert(phi);
  Cnf cnf = phi->MakeCnf(hplus);
  cnf.Minimize();
  cnf.AddToSetups(setups);
}

std::ostream& operator<<(std::ostream& os, const Formula& phi) {
  phi.Print(&os);
  return os;
}

// }}}

}  // namespace esbl

