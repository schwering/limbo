// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// schwering@kbsg.rwth-aachen.de

#include "ewff.h"
#include <memory>

class Ewff::Equality : public Ewff {
 public:
  Equality(const Term& t1, const Term& t2);

  virtual std::unique_ptr<Ewff> Substitute(
      const std::map<Term,Term>& theta) const override;
  virtual Truth Eval(bool sign) const override;

  virtual bool is_ground() const override;

 protected:
  virtual void CollectVariables(std::set<Term>& vs) const override;
  virtual void CollectNames(std::set<Term>& ns) const override;

 private:
  const Term t1_;
  const Term t2_;
};

Ewff::Equality::Equality(const Term& t1, const Term& t2)
  : t1_(t1), t2_(t2) {
}

std::unique_ptr<Ewff> Ewff::Equality::Substitute(
    const std::map<Term,Term>& theta) const {
  Equality *e = new Equality(t1_.Substitute(theta), t2_.Substitute(theta));
  return std::unique_ptr<Ewff>(e);
}

Ewff::Truth Ewff::Equality::Eval(bool sign) const {
  if (t1_ == t2_) {
    return kTrue;
  }
  if (t1_.is_ground() && t2_.is_ground()) {
    return t1_ == t2_ ? kTrue : kFalse;
  }
  return kUnknown;
}

bool Ewff::Equality::is_ground() const {
  return t1_.is_ground() && t2_.is_ground();
}

void Ewff::Equality::CollectVariables(std::set<Term>& vs) const {
  if (t1_.is_variable()) {
    vs.insert(t1_);
  }
  if (t2_.is_variable()) {
    vs.insert(t2_);
  }
}

void Ewff::Equality::CollectNames(std::set<Term>& ns) const {
  if (t1_.is_name()) {
    ns.insert(t1_);
  }
  if (t2_.is_name()) {
    ns.insert(t2_);
  }
}


class Ewff::Negation : public Ewff {
 public:
  explicit Negation(std::unique_ptr<Ewff>&& e);

  virtual std::unique_ptr<Ewff> Substitute(
      const std::map<Term,Term>& theta) const override;
  virtual Truth Eval(bool sign) const override;

  virtual bool is_ground() const override;

 protected:
  virtual void CollectVariables(std::set<Term>& vs) const override;
  virtual void CollectNames(std::set<Term>& ns) const override;

 private:
  std::unique_ptr<Ewff> e_;
};

Ewff::Negation::Negation(std::unique_ptr<Ewff>&& e)
  : e_(std::move(e)) {
}

std::unique_ptr<Ewff> Ewff::Negation::Substitute(
    const std::map<Term,Term>& theta) const {
  return std::unique_ptr<Ewff>(new Negation(e_->Substitute(theta)));
}

Ewff::Truth Ewff::Negation::Eval(bool sign) const {
  switch (e_->Eval(!sign)) {
    case kTrue: return kFalse;
    case kFalse: return kTrue;
    case kUnknown: return kUnknown;
  }
}

bool Ewff::Negation::is_ground() const {
  return e_->is_ground();
}

void Ewff::Negation::CollectVariables(std::set<Term>& vs) const {
  e_->CollectVariables(vs);
}

void Ewff::Negation::CollectNames(std::set<Term>& ns) const {
  e_->CollectNames(ns);
}


class Ewff::Disjunction : public Ewff {
 public:
  explicit Disjunction(std::unique_ptr<Ewff>&& e1, std::unique_ptr<Ewff>&& e2);

  virtual std::unique_ptr<Ewff> Substitute(
      const std::map<Term,Term>& theta) const override;
  virtual Truth Eval(bool sign) const override;

  virtual bool is_ground() const override;

 protected:
  virtual void CollectVariables(std::set<Term>& vs) const override;
  virtual void CollectNames(std::set<Term>& ns) const override;

 private:
  std::unique_ptr<Ewff> e1_;
  std::unique_ptr<Ewff> e2_;
};

Ewff::Disjunction::Disjunction(std::unique_ptr<Ewff>&& e1,
                               std::unique_ptr<Ewff>&& e2)
  : e1_(std::move(e1)), e2_(std::move(e2)) {
}

std::unique_ptr<Ewff> Ewff::Disjunction::Substitute(
    const std::map<Term,Term>& theta) const {
  return std::unique_ptr<Ewff>(new Disjunction(e1_->Substitute(theta),
                                               e2_->Substitute(theta)));
}

Ewff::Truth Ewff::Disjunction::Eval(bool sign) const {
  const Truth t1 = e1_->Eval(sign);
  const Truth t2 = e2_->Eval(sign);
  if (t1 == t2) {
    return t1;
  }
  if (sign && (t1 == kTrue || t2 == kTrue)) {
    return kTrue;
  }
  if (!sign && (t1 == kFalse || t2 == kFalse)) {
    return kFalse;
  }
  return kUnknown;
}

bool Ewff::Disjunction::is_ground() const {
  return e1_->is_ground() && e2_->is_ground();
}

void Ewff::Disjunction::CollectVariables(std::set<Term>& vs) const {
  e1_->CollectVariables(vs);
  e2_->CollectVariables(vs);
}

void Ewff::Disjunction::CollectNames(std::set<Term>& ns) const {
  e1_->CollectNames(ns);
  e2_->CollectNames(ns);
}


class Ewff::SortCheck : public Ewff {
 public:
  SortCheck(const Term& t, const std::function<Truth(const Term& t)> &f);

  virtual std::unique_ptr<Ewff> Substitute(
      const std::map<Term,Term>& theta) const override;
  virtual Truth Eval(bool sign) const override;

  virtual bool is_ground() const override;

 protected:
  virtual void CollectVariables(std::set<Term>& vs) const override;
  virtual void CollectNames(std::set<Term>& ns) const override;

 private:
  const Term t_;
  const std::function<Truth(const Term& t)> f_;
};

Ewff::SortCheck::SortCheck(const Term& t,
                           const std::function<Truth(const Term& t)> &f)
  : t_(t), f_(f) {
}

std::unique_ptr<Ewff> Ewff::SortCheck::Substitute(
    const std::map<Term,Term>& theta) const {
  const Term t = t_.Substitute(theta);
  if (t.is_ground()) {
    return f_(t) ? Ewff::True() : Ewff::False();
  } else {
    SortCheck *e = new SortCheck(t, f_);
    return std::unique_ptr<Ewff>(e);
  }
}

Ewff::Truth Ewff::SortCheck::Eval(bool sign) const {
  return !is_ground() ? kUnknown : f_(t_) ? kTrue : kFalse;
}

bool Ewff::SortCheck::is_ground() const {
  return t_.is_ground();
}

void Ewff::SortCheck::CollectVariables(std::set<Term>& vs) const {
  if (t_.is_variable()) {
    vs.insert(t_);
  }
}

void Ewff::SortCheck::CollectNames(std::set<Term>& ns) const {
  if (t_.is_name()) {
    ns.insert(t_);
  }
}


Ewff::~Ewff() {
}

std::unique_ptr<Ewff> Ewff::True() {
  const Term t = Term::CreateVariable();
  return Equal(t, t);
}

std::unique_ptr<Ewff> Ewff::False() {
  return Neg(True());
}

std::unique_ptr<Ewff> Ewff::Equal(const Term& t1, const Term& t2) {
  return std::unique_ptr<Ewff>(new Equality(t1, t2));
}

std::unique_ptr<Ewff> Ewff::Unequal(const Term& t1, const Term& t2) {
  return Neg(Equal(t1, t2));
}

Ewff::Truth Ewff::Eval(bool sign) const {
  return Eval(true);
}

bool Ewff::is_ground() const {
  return !variables().empty();
}

std::set<Term> Ewff::variables() const {
  std::set<Term> vs;
  CollectVariables(vs);
  return vs;
}

std::set<Term> Ewff::names() const {
  std::set<Term> ns;
  CollectNames(ns);
  return ns;
}

#if 0
static void ewff_ground_h(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *),
        varmap_t *varmap)
{
    if (varmap_size(varmap) < varset_size(vars)) {
        const var_t x = varset_get(vars, varmap_size(varmap));
        for (EACH_CONST(stdset, hplus, i)) {
            const stdname_t n = i.val;
            varmap_add_replace(varmap, x, n);
            ewff_ground_h(e, vars, hplus, ground, varmap);
        }
        varmap_remove(varmap, x);
    } else {
        if (ewff_eval(e, varmap)) {
            ground(varmap);
        }
    }
}

void ewff_ground(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *))
{
    varmap_t varmap = varmap_init_with_size(varset_size(vars));
    ewff_ground_h(e, vars, hplus, ground, &varmap);
}
#endif

