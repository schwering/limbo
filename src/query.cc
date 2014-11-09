// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include "./query.h"

namespace esbl {

Query::Cnf::C Query::Cnf::C::Concat(const C& cl1, const C& cl2) {
  C cl = cl1;
  cl.eqs.insert(cl.eqs.end(), cl2.eqs.begin(), cl2.eqs.end());
  cl.neqs.insert(cl.neqs.end(), cl2.neqs.begin(), cl2.neqs.end());
  cl.clause.insert(cl2.clause.begin(), cl2.clause.end());
  return cl;
}

Query::Cnf::C Query::Cnf::C::Substitute(const Unifier& theta) const {
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

bool Query::Cnf::C::VacuouslyTrue() const {
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

Query::Cnf Query::Cnf::Substitute(const Unifier& theta) const {
  Cnf q;
  q.cs.reserve(cs.size());
  for (const C& cl : cs) {
    q.cs.push_back(cl.Substitute(theta));
  }
  return q;
}

Query::Cnf Query::Cnf::And(const Cnf& c) const {
  Cnf r;
  for (const C& cl1 : cs) {
    for (const C& cl2 : c.cs) {
      r.cs.push_back(Cnf::C::Concat(cl1, cl2));
    }
  }
  r.n_vars = std::max(n_vars, c.n_vars);
  return r;
}

Query::Cnf Query::Cnf::Or(const Cnf& c) const {
  Cnf r = *this;
  r.cs.insert(r.cs.begin(), c.cs.begin(), c.cs.end());
  r.n_vars = std::max(n_vars, c.n_vars);
  return r;
}

std::vector<SimpleClause> Query::Cnf::UnsatisfiedClauses() const {
  std::vector<SimpleClause> scs(cs.size());
  for (const C& c : cs) {
    if (!c.VacuouslyTrue()) {
      scs.push_back(c.clause);
    }
  }
  return scs;
}

struct Query::Equal : public Query {
  bool sign;
  Term t1;
  Term t2;

  Equal(const Term& t1, const Term& t2) : sign(true), t1(t1), t2(t2) {}

  void Negate() override { sign = !sign; }

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
};

struct Query::Lit : public Query {
  Literal l;

  Lit(const Literal& l) : l(l) {}

  void Negate() override { l = l.Flip(); }

  Cnf MakeCnf(const StdName::SortedSet&) const override {
    Cnf::C cl;
    cl.clause.insert(l);
    Cnf c;
    c.cs.push_back(cl);
    return c;
  }
};

struct Query::Junction : public Query {
  enum Type { DISJUNCTION, CONJUNCTION };

  Type type;
  std::unique_ptr<Query> l;
  std::unique_ptr<Query> r;

  Junction(Type type, std::unique_ptr<Query> l, std::unique_ptr<Query> r)
      : type(type), l(std::move(l)), r(std::move(r)) {}

  void Negate() override {
    type = type == DISJUNCTION ? CONJUNCTION : DISJUNCTION;
    l->Negate();
    r->Negate();
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus) const override {
    if (type == DISJUNCTION) {
      return l->MakeCnf(hplus).Or(r->MakeCnf(hplus));
    } else {
      return l->MakeCnf(hplus).And(r->MakeCnf(hplus));
    }
  }
};

struct Query::Action : public Query {
  Term t;
  std::unique_ptr<Query> q;

  Action(const Term& t, std::unique_ptr<Query> q) : t(t), q(std::move(q)) {}

  void Negate() override {
    q->Negate();
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus) const override {
    Cnf c = q->MakeCnf(hplus);
    for (Cnf::C& cl : c.cs) {
      cl.clause = cl.clause.PrependActions({t});
    }
    return c;
  }
};

struct Query::Quantifier : public Query {
  enum Type { EXISTENTIAL, UNIVERSAL };

  Type type;
  Variable x;
  std::unique_ptr<Query> q;

  Quantifier(Type type, const Variable& x, std::unique_ptr<Query> q)
      : type(type), x(x), q(std::move(q)) {}

  void Negate() override {
    type = type == EXISTENTIAL ? UNIVERSAL : EXISTENTIAL;
    q->Negate();
  }

  Cnf MakeCnf(const StdName::SortedSet& hplus) const override {
    const Cnf c = q->MakeCnf(hplus);
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
};

std::unique_ptr<Query> Query::Eq(const Term& t1, const Term& t2) {
  return std::unique_ptr<Query>(new Equal(t1, t2));
}

std::unique_ptr<Query> Query::Neq(const Term& t1, const Term& t2) {
  std::unique_ptr<Query> eq(std::move(Eq(t1, t2)));
  eq->Negate();
  return std::move(eq);
}

std::unique_ptr<Query> Query::Lit(const Literal& l) {
  return std::unique_ptr<Query>(new struct Lit(l));
}

std::unique_ptr<Query> Query::Or(std::unique_ptr<Query> q1,
                                 std::unique_ptr<Query> q2) {
  return std::unique_ptr<Query>(new Junction(Junction::DISJUNCTION,
                                             std::move(q1), std::move(q2)));
}

std::unique_ptr<Query> Query::And(std::unique_ptr<Query> q1,
                                  std::unique_ptr<Query> q2) {
  return std::unique_ptr<Query>(new Junction(Junction::CONJUNCTION,
                                             std::move(q1), std::move(q2)));
}

std::unique_ptr<Query> Query::Neg(std::unique_ptr<Query> q) {
  q->Negate();
  return std::move(q);
}

std::unique_ptr<Query> Query::Act(const Term& a,
                                  std::unique_ptr<Query> q) {
  return std::unique_ptr<Query>(new Action(a, std::move(q)));
}

std::unique_ptr<Query> Query::Exists(const Variable& x,
                                     std::unique_ptr<Query> q) {
  return std::unique_ptr<Query>(new Quantifier(Quantifier::EXISTENTIAL, x,
                                               std::move(q)));
}

std::unique_ptr<Query> Query::Forall(const Variable& x,
                                     std::unique_ptr<Query> q) {
  return std::unique_ptr<Query>(new Quantifier(Quantifier::UNIVERSAL, x,
                                               std::move(q)));
}

std::vector<SimpleClause> Query::Clauses(
    const StdName::SortedSet& hplus) const {
  Cnf c = MakeCnf(hplus);
  return c.UnsatisfiedClauses();
}

}  // namespace esbl

