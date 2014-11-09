// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <algorithm>
#include <cassert>
#include "./formula.h"

namespace esbl {

Formula::Cnf::C Formula::Cnf::C::Concat(const C& cl1, const C& cl2) {
  C cl = cl1;
  cl.eqs.insert(cl.eqs.end(), cl2.eqs.begin(), cl2.eqs.end());
  cl.neqs.insert(cl.neqs.end(), cl2.neqs.begin(), cl2.neqs.end());
  cl.clause.insert(cl2.clause.begin(), cl2.clause.end());
  assert(cl.eqs.size() == cl1.eqs.size() + cl2.eqs.size());
  assert(cl.neqs.size() == cl1.neqs.size() + cl2.neqs.size());
  assert(cl.clause.size() == cl1.clause.size() + cl2.clause.size());
  return cl;
}

Formula::Cnf::C Formula::Cnf::C::Substitute(const Unifier& theta) const {
  C cl;
  cl.eqs.reserve(eqs.size());
  for (const auto& p : eqs) {
    cl.eqs.push_back(std::make_pair(p.first.Substitute(theta),
                                    p.second.Substitute(theta)));
  }
  cl.neqs.reserve(neqs.size());
  for (const auto& p : neqs) {
    cl.eqs.push_back(std::make_pair(p.first.Substitute(theta),
                                    p.second.Substitute(theta)));
  }
  clause.Substitute(theta);
  return cl;
}

bool Formula::Cnf::C::VacuouslyTrue() const {
  for (const auto& p : eqs) {
    if (p.first == p.second) {
      return true;
    }
  }
  for (const auto& p : neqs) {
    if (p.first != p.second) {
      return true;
    }
  }
  return false;
}

Formula::Cnf Formula::Cnf::Substitute(const Unifier& theta) const {
  Cnf c;
  c.cs.reserve(cs.size());
  for (const C& cl : cs) {
    c.cs.push_back(cl.Substitute(theta));
  }
  return c;
}

Formula::Cnf Formula::Cnf::And(const Cnf& c) const {
  Cnf r = *this;
  r.cs.insert(r.cs.end(), c.cs.begin(), c.cs.end());
  r.n_vars = std::max(n_vars, c.n_vars);
  assert(r.cs.size() == cs.size() + c.cs.size());
  return r;
}

Formula::Cnf Formula::Cnf::Or(const Cnf& c) const {
  Cnf r;
  for (const C& cl1 : cs) {
    for (const C& cl2 : c.cs) {
      r.cs.push_back(Cnf::C::Concat(cl1, cl2));
    }
  }
  r.n_vars = std::max(n_vars, c.n_vars);
  assert(c.cs.size() == cs.size() * c.cs.size());
  return r;
}

std::vector<SimpleClause> Formula::Cnf::UnsatisfiedClauses() const {
  std::vector<SimpleClause> scs;
  for (const C& c : cs) {
    if (!c.VacuouslyTrue()) {
      scs.push_back(c.clause);
    }
  }
  return scs;
}

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

  Cnf MakeCnf(const StdName::SortedSet&) const override {
    Cnf::C cl;
    if (sign) {
      cl.eqs.push_back(std::make_pair(t1, t2));
    } else {
      cl.neqs.push_back(std::make_pair(t1, t2));
    }
    Cnf c;
    c.cs.push_back(cl);
    return c;
  }

  void print(std::ostream* os) const override {
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

  Cnf MakeCnf(const StdName::SortedSet&) const override {
    Cnf::C cl;
    cl.clause.insert(l);
    Cnf c;
    c.cs.push_back(cl);
    return c;
  }

  void print(std::ostream* os) const override {
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

  Cnf MakeCnf(const StdName::SortedSet& hplus) const override {
    if (type == DISJUNCTION) {
      return l->MakeCnf(hplus).Or(r->MakeCnf(hplus));
    } else {
      return l->MakeCnf(hplus).And(r->MakeCnf(hplus));
    }
  }

  void print(std::ostream* os) const override {
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

  Cnf MakeCnf(const StdName::SortedSet& hplus) const override {
    const Cnf c = phi->MakeCnf(hplus);
    std::vector<StdName> ns;
    const auto it = hplus.find(x.sort());
    if (it != hplus.end()) {
      ns.insert(ns.end(),
                it->second.lower_bound(StdName::MIN_NORMAL),
                it->second.end());
    }
    for (int i = 0; i <= c.n_vars; ++i) {
      ns.insert(ns.end(), Term::Factory::CreatePlaceholderStdName(i, x.sort()));
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
    ++r.n_vars;
    return r;
  }

  void print(std::ostream* os) const override {
    const char* s = type == EXISTENTIAL ? "E " : "";
    *os << '(' << s << x << ". " << *phi << ')';
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

std::vector<SimpleClause> Formula::Clauses(
    const StdName::SortedSet& hplus) const {
  Cnf c = MakeCnf(hplus);
  return c.UnsatisfiedClauses();
}

}  // namespace esbl

