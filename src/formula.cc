// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include <utility>
#include "./formula.h"

namespace esbl {

struct Formula::Cnf {
 public:
  struct Disj;

  Cnf();
  explicit Cnf(const Disj& d);
  Cnf(const Cnf&);
  Cnf& operator=(const Cnf&);

  Cnf Substitute(const Unifier& theta) const;
  Cnf And(const Cnf& c) const;
  Cnf Or(const Cnf& c) const;
  bool EntailedBy(Setup* setup, split_level k) const;
  bool EntailedBy(Setups* setups, split_level k) const;

  // unique_ptr prevents incomplete type errors
  std::unique_ptr<std::vector<Disj>> ds;
};

struct Formula::Cnf::Disj {
  static Disj Concat(const Disj& c1, const Disj& c2);
  Disj Substitute(const Unifier& theta) const;
  bool VacuouslyTrue() const;
  bool EntailedBy(Setup* setup, split_level k) const;
  bool EntailedBy(Setups* setups, split_level k) const;

  std::vector<std::pair<Term, Term>> eqs;
  std::vector<std::pair<Term, Term>> neqs;
  SimpleClause clause;
  std::vector<std::tuple<split_level, Cnf>> ks;
  std::vector<std::tuple<split_level, Cnf, Cnf>> bs;
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

  Ptr Substitute(const Unifier& theta) const override {
    return Ptr(new Equal(sign, t1.Substitute(theta), t2.Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    if (t1.is_variable()) {
      (*vs)[t1.sort()].insert(Variable(t1));
    }
    if (t2.is_variable()) {
      (*vs)[t2.sort()].insert(Variable(t2));
    }
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    CollectVariables(vs);
  }

  Cnf MakeCnf(const StdName::SortedSet&,
              const Variable::SortedSet&) const override {
    Cnf::Disj d;
    if (sign) {
      d.eqs.push_back(std::make_pair(t1, t2));
    } else {
      d.neqs.push_back(std::make_pair(t1, t2));
    }
    return Cnf(d);
  }

  void Print(std::ostream* os) const override {
    *os << '(' << t1 << " = " << t2 << ')';
  }
};

struct Formula::Lit : public Formula {
  Literal l;

  explicit Lit(const Literal& l) : l(l) {}

  Ptr Copy() const override { return Ptr(new Lit(l)); }

  void Negate() override { l = l.Flip(); }

  void PrependActions(const TermSeq& z) override { l = l.PrependActions(z); }

  Ptr Substitute(const Unifier& theta) const override {
    return Ptr(new Lit(l.Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    l.CollectVariables(vs);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    CollectVariables(vs);
  }

  Cnf MakeCnf(const StdName::SortedSet&,
              const Variable::SortedSet&) const override {
    Cnf::Disj d;
    d.clause.insert(l);
    return Cnf(d);
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

  Ptr Substitute(const Unifier& theta) const override {
    return Ptr(new Junction(type, l->Substitute(theta), r->Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    l->CollectVariables(vs);
    r->CollectVariables(vs);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    // We assume formulas to be rectified, so that's OK. Otherwise, if x
    // occurred freely in l but bound in r, we need to take care not to delete
    // it with the second call.
    l->CollectFreeVariables(vs);
    r->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus,
              const Variable::SortedSet& vars) const override {
    if (type == DISJUNCTION) {
      return l->MakeCnf(hplus, vars).Or(r->MakeCnf(hplus, vars));
    } else {
      return l->MakeCnf(hplus, vars).And(r->MakeCnf(hplus, vars));
    }
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

  Ptr Substitute(const Unifier& theta) const override {
    const Variable y = Variable(x.Substitute(theta));
    return Ptr(new Quantifier(type, y, phi->Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    phi->CollectVariables(vs);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    phi->CollectFreeVariables(vs);
    (*vs)[x.sort()].erase(x);
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus,
              const Variable::SortedSet& vars) const override {
    const Cnf c = phi->MakeCnf(hplus, vars);
    std::vector<StdName> ns;
    const Term::Sort sort = x.sort();
    const auto it = hplus.find(sort);
    if (it != hplus.end()) {
      ns.insert(ns.end(),
                it->second.lower_bound(StdName::MIN_NORMAL),
                it->second.end());
    }
    const auto jt = vars.find(sort);
    assert(jt != vars.end());
    const size_t n_vars = jt->second.size();
    assert(n_vars > 0);
    for (size_t i = 0; i <= n_vars; ++i) {
      ns.insert(ns.end(), Term::Factory::CreatePlaceholderStdName(i, sort));
    }
    assert(!ns.empty());

    Cnf r = c.Substitute({{x, ns[ns.size() - 1]}});
    for (size_t i = 0; i < ns.size() - 1 ; ++i) {
      const Cnf d = c.Substitute({{x, ns[i]}});
      if (type == EXISTENTIAL) {
        r = r.Or(d);
      } else {
        r = r.And(d);
      }
    }
    return r;
  }

  void Print(std::ostream* os) const override {
    const char* s = type == EXISTENTIAL ? "E " : "";
    *os << '(' << s << x << ". " << *phi << ')';
  }
};

struct Formula::Knowledge : public Formula {
  split_level k;
  Ptr phi;

  Knowledge(split_level k, Ptr phi) : k(k), phi(std::move(phi)) {}

  Ptr Copy() const override { return Ptr(new Knowledge(k, phi->Copy())); }

  void Negate() override { phi->Negate(); }

  void PrependActions(const TermSeq& z) override { phi->PrependActions(z); }

  Ptr Substitute(const Unifier& theta) const {
    return Ptr(new Knowledge(k, phi->Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    phi->CollectVariables(vs);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    phi->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus,
              const Variable::SortedSet& vars) const override {
    Cnf::Disj d;
    d.ks.push_back(std::make_tuple(k, phi->MakeCnf(hplus, vars)));
    return Cnf(d);
  }

  void Print(std::ostream* os) const override {
    *os << "K_" << k << '(' << *phi << ')';
  }
};

struct Formula::Belief : public Formula {
  split_level k;
  Ptr neg_phi;
  Ptr psi;

  Belief(split_level k, Ptr neg_phi, Ptr psi)
      : k(k), neg_phi(std::move(neg_phi)), psi(std::move(psi)) {}

  Ptr Copy() const override {
    return Ptr(new Belief(k, neg_phi->Copy(), psi->Copy()));
  }

  void Negate() override { neg_phi->Negate(); psi->Negate(); }

  void PrependActions(const TermSeq& z) override {
    neg_phi->PrependActions(z);
    psi->PrependActions(z);
  }

  Ptr Substitute(const Unifier& theta) const {
    return Ptr(new Belief(k, neg_phi->Substitute(theta),
                          psi->Substitute(theta)));
  }

  void CollectVariables(Variable::SortedSet* vs) const {
    neg_phi->CollectVariables(vs);
    psi->CollectVariables(vs);
  }

  void CollectFreeVariables(Variable::SortedSet* vs) const {
    neg_phi->CollectFreeVariables(vs);
    psi->CollectFreeVariables(vs);
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus,
              const Variable::SortedSet& vars) const override {
    Cnf::Disj d;
    d.bs.push_back(std::make_tuple(k, neg_phi->MakeCnf(hplus, vars),
                                   psi->MakeCnf(hplus, vars)));
    return Cnf(d);
  }

  void Print(std::ostream* os) const override {
    *os << "K_" << k << '(' << '~' << *neg_phi << " => " << *psi << ')';
  }
};

Formula::Cnf::Disj Formula::Cnf::Disj::Concat(const Disj& d1, const Disj& d2) {
  Disj d = d1;
  d.eqs.insert(d.eqs.end(), d2.eqs.begin(), d2.eqs.end());
  d.neqs.insert(d.neqs.end(), d2.neqs.begin(), d2.neqs.end());
  d.clause.insert(d2.clause.begin(), d2.clause.end());
  d.ks.insert(d.ks.end(), d2.ks.begin(), d2.ks.end());
  d.bs.insert(d.bs.end(), d2.bs.begin(), d2.bs.end());
  assert(d.eqs.size() == d1.eqs.size() + d2.eqs.size());
  assert(d.neqs.size() == d1.neqs.size() + d2.neqs.size());
  assert(d.clause.size() <= d1.clause.size() + d2.clause.size());
  assert(d.ks.size() == d1.ks.size() + d2.ks.size());
  assert(d.bs.size() == d1.bs.size() + d2.bs.size());
  return d;
}

Formula::Cnf::Disj Formula::Cnf::Disj::Substitute(const Unifier& theta) const {
  Disj d;
  d.eqs.reserve(eqs.size());
  for (const auto& p : eqs) {
    d.eqs.push_back(std::make_pair(p.first.Substitute(theta),
                                   p.second.Substitute(theta)));
  }
  d.neqs.reserve(neqs.size());
  for (const auto& p : neqs) {
    d.eqs.push_back(std::make_pair(p.first.Substitute(theta),
                                   p.second.Substitute(theta)));
  }
  d.clause = clause.Substitute(theta);
  for (const auto& k : ks) {
    d.ks.push_back(std::make_tuple(
            std::get<0>(k),
            std::get<1>(k).Substitute(theta)));
  }
  for (const auto& b : bs) {
    d.bs.push_back(std::make_tuple(
            std::get<0>(b),
            std::get<1>(b).Substitute(theta),
            std::get<2>(b).Substitute(theta)));
  }
  return d;
}

bool Formula::Cnf::Disj::VacuouslyTrue() const {
  auto eq = [](const std::pair<Term, Term>& p) { return p.first == p.second; };
  return std::any_of(eqs.begin(), eqs.end(), eq) ||
      std::any_of(neqs.begin(), neqs.end(),  eq);
}

bool Formula::Cnf::Disj::EntailedBy(Setup* s, split_level k) const {
  assert(bs.empty());
  if (VacuouslyTrue()) {
    return true;
  }
  if (s->Entails(clause, k)) {
    return true;
  }
  for (const auto& p : ks) {
    split_level k1 = std::get<0>(p);
    const Cnf& phi = std::get<1>(p);
    // TODO(chs) (1) The CNF should first be minimized (using resolution).
    // TODO(chs) (2) The negation of d.clause should be added to the setup.
    // TODO(chs) (3) Or representation theorem instead of (2)?
    // That way, at least the SSA of knowledge/belief should be come out
    // correctly.
    if (phi.EntailedBy(s, k1)) {
      return true;
    }
  }
  return false;
}

bool Formula::Cnf::Disj::EntailedBy(Setups* s, split_level k) const {
  if (VacuouslyTrue()) {
    return true;
  }
  if (s->Entails(clause, k)) {
    return true;
  }
  for (const auto& p : ks) {
    split_level k1 = std::get<0>(p);
    const Cnf& phi = std::get<1>(p);
    if (phi.EntailedBy(s, k1)) {
      return true;
    }
  }
  // TODO(chs) ~neg_phi => psi, c.f. TODOs for Knowledge class; API changes
  // needed to account for ~neg_phi and psi at once.
  assert(bs.empty());
  return false;
}

Formula::Cnf::Cnf()
    : ds(new std::vector<Formula::Cnf::Disj>()) {}

Formula::Cnf::Cnf(const Formula::Cnf::Disj& d)
    : ds(new std::vector<Formula::Cnf::Disj>()) {
  ds->push_back(d);
}

Formula::Cnf::Cnf(const Cnf& c)
    : ds(new std::vector<Formula::Cnf::Disj>()) {
  *ds = *c.ds;
}

Formula::Cnf& Formula::Cnf::operator=(const Formula::Cnf& c) {
  *ds = *c.ds;
  return *this;
}

Formula::Cnf Formula::Cnf::Substitute(const Unifier& theta) const {
  Cnf c;
  c.ds->reserve(ds->size());
  for (const Disj& d : *ds) {
    c.ds->push_back(d.Substitute(theta));
  }
  return c;
}

Formula::Cnf Formula::Cnf::And(const Cnf& c) const {
  Cnf r = *this;
  r.ds->insert(r.ds->end(), c.ds->begin(), c.ds->end());
  assert(r.ds->size() == ds->size() + c.ds->size());
  return r;
}

Formula::Cnf Formula::Cnf::Or(const Cnf& c) const {
  Cnf r;
  for (const Disj& d1 : *ds) {
    for (const Disj& d2 : *c.ds) {
      r.ds->push_back(Cnf::Disj::Concat(d1, d2));
    }
  }
  assert(r.ds->size() == ds->size() * c.ds->size());
  return r;
}

bool Formula::Cnf::EntailedBy(Setup* s, split_level k) const {
  return std::all_of(ds->begin(), ds->end(),
                     [s, k](const Disj& d) { return d.EntailedBy(s, k); });
}

bool Formula::Cnf::EntailedBy(Setups* s, split_level k) const {
  return std::all_of(ds->begin(), ds->end(),
                     [s, k](const Disj& d) { return d.EntailedBy(s, k); });
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

bool Formula::EntailedBy(Setup* setup, split_level k) const {
  Variable::SortedSet vs;
  CollectVariables(&vs);
  Cnf cnf = MakeCnf(setup->hplus(), vs);
  return cnf.EntailedBy(setup, k);
}

bool Formula::EntailedBy(Setups* setups, split_level k) const {
  Variable::SortedSet vs;
  CollectVariables(&vs);
  return MakeCnf(setups->hplus(), vs).EntailedBy(setups, k);
}

#if 0
std::ostream& operator<<(std::ostream& os, const Formula::Cnf::Disj& d) {
  os << '(';
  for (auto it = d.eqs.begin(); it != d.eqs.end(); ++it) {
    if (it != d.eqs.begin()) {
      os << " v ";
    }
    os << it->first << " = " << it->second;
  }
  for (auto it = d.neqs.begin(); it != d.neqs.end(); ++it) {
    if (!d.eqs.empty() || it != d.neqs.begin()) {
      os << " v ";
    }
    os << it->first << " != " << it->second;
  }
  for (auto it = d.clause.begin(); it != d.clause.end(); ++it) {
    if (!d.eqs.empty() || !d.neqs.empty() || it != d.clause.begin()) {
      os << " v ";
    }
    os << *it;
  }
  for (auto it = d.ks.begin(); it != d.ks.end(); ++it) {
    if (!d.eqs.empty() || !d.neqs.empty() || !d.clause.empty() ||
        it != d.ks.begin()) {
      os << " v ";
    }
    os << "K_" << std::get<0>(*it) << '(' << std::get<1>(*it) << ')';
  }
  for (auto it = d.bs.begin(); it != d.bs.end(); ++it) {
    if (!d.eqs.empty() || !d.neqs.empty() || !d.clause.empty() ||
        !d.ks.empty() || it != d.bs.begin()) {
      os << " v ";
    }
    os << "B_" << std::get<0>(*it) <<
        "(~" << std::get<1>(*it) << " => " << std::get<2>(*it) << ')';
  }
  os << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Formula::Cnf& cnf) {
  os << '(';
  for (auto it = cnf.ds->begin(); it != cnf.ds->end(); ++it) {
    if (it != cnf.ds->begin()) {
      os << " ^ ";
    }
    os << *it;
  }
  os << ')';
  return os;
}
#endif

}  // namespace esbl

