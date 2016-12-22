// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014--2016 Christoph Schwering

#ifndef LELA_MODAL_H_
#define LELA_MODAL_H_

#include <cassert>

#include <vector>

#include <lela/formula.h>
#include <lela/solver.h>

namespace lela {

class KnowledgeBase {
 public:
  typedef std::size_t sphere_index;

  KnowledgeBase(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf), objective_(sf, tf) {
    spheres_.push_back(Solver(sf, tf));
  }

  KnowledgeBase(const KnowledgeBase&) = delete;
  KnowledgeBase& operator=(const KnowledgeBase&) = delete;
  KnowledgeBase(KnowledgeBase&&) = default;
  KnowledgeBase& operator=(KnowledgeBase&&) = default;

  static bool ProperPlus(const Formula& phi) {
    return bool(GetProperPlusClause(phi));
  }

  void Add(const Clause& c) {
    for (Solver& sphere : spheres_) {
      sphere.AddClause(c);
    }
    knowledge_.push_back(c);
#ifndef NDEBUG
    init_spheres_ = false;
#endif
  }

  void Add(Formula::split_level k, Formula::split_level l,
           const Formula& antecedent, const Clause& not_antecedent_or_consequent) {
    beliefs_.push_back(Conditional{k, l, antecedent.Clone(), not_antecedent_or_consequent});
#ifndef NDEBUG
    init_spheres_ = false;
#endif
  }

  bool Add(const Formula& phi) {
    Formula::Ref psi = phi.NF(sf_, tf_);
    if (psi->type() == Formula::kBel) {
      const Formula::split_level k = psi->as_bel().k();
      const Formula::split_level l = psi->as_bel().l();
      const Formula& ante = psi->as_bel().antecedent();
      internal::Maybe<Clause> not_ante_or_conse = GetProperPlusClause(psi->as_bel().not_antecedent_or_consequent());
      if (not_ante_or_conse) {
        Add(k, l, ante, not_ante_or_conse.val);
        return true;
      }
    } else {
      internal::Maybe<Clause> c = GetProperPlusClause(psi->as_know().arg());
      if (c) {
        Add(c.val);
        return true;
      }
    }
    return false;
  }

  void BuildSpheres() {
    spheres_.clear();
    std::vector<bool> done(beliefs_.size(), false);
    bool is_plausibility_consistent = true;
    std::size_t n_done = 0;
    std::size_t last_n_done;
    do {
      last_n_done = n_done;
      Solver sphere(sf_, tf_);
      for (const Clause& c : knowledge_) {
        sphere.AddClause(c);
      }
      for (std::size_t i = 0; i < beliefs_.size(); ++i) {
        const Conditional& c = beliefs_[i];
        if (!done[i]) {
          sphere.AddClause(c.not_ante_or_conse);
        }
      }
      for (std::size_t i = 0; is_plausibility_consistent && i < beliefs_.size(); ++i) {
        const Conditional& c = beliefs_[i];
        if (!done[i]) {
          const bool possiblyConsistent = !sphere.Entails(c.k, *Formula::Factory::Not(c.ante->Clone()));
          if (possiblyConsistent) {
            done[i] = true;
            ++n_done;
            const bool necessarilyConsistent = sphere.Consistent(c.l, *c.ante);
            if (!necessarilyConsistent) {
              is_plausibility_consistent = false;
            }
          }
        }
      }
      if (is_plausibility_consistent || n_done == last_n_done) {
        spheres_.push_back(std::move(sphere));
      }
    } while (n_done > last_n_done);
#ifndef NDEBUG
    init_spheres_ = true;
#endif
  }

  bool Entails(const Formula& sigma, bool assume_consistent = true) {
    assert(init_spheres_);
    assert(sigma.subjective());
    Formula::Ref phi = ReduceModalities(sigma, assume_consistent);
    assert(phi->objective());
    return objective_.Entails(0, *phi);
  }

  sphere_index n_spheres() const { return spheres_.size(); }
  Solver& sphere(sphere_index p) { return spheres_[p]; }
  const std::vector<Solver>& spheres() const { return spheres_; }

 private:
  struct Conditional {
    Formula::split_level k;
    Formula::split_level l;
    Formula::Ref ante;
    Clause not_ante_or_conse;
  };
  typedef Grounder::SortedTermSet SortedTermSet;

  static internal::Maybe<Clause> GetProperPlusClause(const Formula& phi) {
    size_t nots = 0;
    const Formula* phi_ptr = &phi;
    for (;;) {
      switch (phi_ptr->type()) {
        case Formula::kAtomic: {
          if (nots % 2 != 0) {
            return internal::Nothing;
          }
          const Clause c = phi_ptr->as_atomic().arg();
          if (!std::all_of(c.begin(), c.end(), [](Literal a) {
                           return a.quasiprimitive() || (!a.lhs().function() && !a.rhs().function()); })) {
            return internal::Nothing;
          }
          return internal::Just(c);
        }
        case Formula::kNot: {
          ++nots;
          phi_ptr = &phi_ptr->as_not().arg();
          break;
        }
        case Formula::kExists: {
          if (nots % 2 == 0) {
            return internal::Nothing;
          }
          phi_ptr = &phi_ptr->as_exists().arg();
          break;
        }
        case Formula::kOr:
        case Formula::kKnow:
        case Formula::kCons:
        case Formula::kBel:
          return internal::Nothing;
      }
    }
  }

  Formula::Ref ReduceModalities(const Formula& alpha, bool assume_consistent) {
    if (alpha.objective()) {
      return alpha.Clone();
    }
    switch (alpha.type()) {
      case Formula::kAtomic: {
        assert(false);
        return Formula::Ref(nullptr);
      }
      case Formula::kNot: {
        return Formula::Factory::Not(ReduceModalities(alpha.as_not().arg(), assume_consistent));
      }
      case Formula::kOr: {
        return Formula::Factory::Or(ReduceModalities(alpha.as_or().lhs(), assume_consistent),
                                    ReduceModalities(alpha.as_or().rhs(), assume_consistent));
      }
      case Formula::kExists: {
        return Formula::Factory::Exists(alpha.as_exists().x(),
                                        ReduceModalities(alpha.as_exists().arg(), assume_consistent));
      }
      case Formula::kKnow: {
        const sphere_index p = n_spheres() - 1;
        Formula::Ref phi = ReduceModalities(alpha.as_know().arg(), assume_consistent);
        return ResEntails(p, alpha.as_know().k(), *phi, assume_consistent);
      }
      case Formula::kCons: {
        const sphere_index p = 0;
        Formula::Ref phi = ReduceModalities(alpha.as_cons().arg(), assume_consistent);
        return ResConsistent(p, alpha.as_cons().k(), *phi, assume_consistent);
      }
      case Formula::kBel: {
        const Formula::Ref ante = ReduceModalities(alpha.as_bel().antecedent(), assume_consistent);
        const Formula::Ref not_ante_or_conse = ReduceModalities(alpha.as_bel().not_antecedent_or_consequent(), assume_consistent);
        const Formula::split_level k = alpha.as_bel().k();
        const Formula::split_level l = alpha.as_bel().l();
        std::vector<Formula::Ref> consistent;
        std::vector<Formula::Ref> entails;
        for (sphere_index p = 0; p < n_spheres(); ++p) {
          consistent.push_back(ResConsistent(p, l, *ante, assume_consistent));
          entails.push_back(ResEntails(p, k, *not_ante_or_conse, assume_consistent));
          // The above calls to ResConsistent() and ResEntails() are potentially
          // very expensive, so we should abort this loop when the subsequent
          // spheres are clearly irrelevant.
          if (consistent.back()->trivially_valid()) {
            break;
          }
        }
        Formula::Ref phi;
        for (sphere_index p = 0; p < entails.size(); ++p) {
          Formula::Ref conj = entails[p]->Clone();
          for (sphere_index q = 0; q < p; ++q) {
            conj = Formula::Factory::Or(consistent[q]->Clone(), std::move(conj));
          }
          if (!phi) {
            phi = std::move(conj);
          } else {
            phi = Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(phi)),
                                                             Formula::Factory::Not(std::move(conj))));
          }
        }
        return phi;
      }
    }
  }

  Formula::Ref ResEntails(sphere_index p, Formula::split_level k, const Formula& phi, bool assume_consistent) {
    return bool_to_formula(spheres_[p].Entails(k, *Res(p, phi.Clone()), assume_consistent));
  }

  Formula::Ref ResConsistent(sphere_index p, Formula::split_level k, const Formula& phi, bool assume_consistent) {
    return bool_to_formula(spheres_[p].Consistent(k, *Res(p, phi.Clone())/*, assume_consistent*/));
  }

  Formula::Ref Res(sphere_index p, Formula::Ref phi) {
    SortedTermSet names = spheres_[p].names();
    phi->Traverse([&names](Term t) { if (t.name()) names.insert(t); return true; });
    return Res(p, std::move(phi), &names);
  }

  Formula::Ref Res(sphere_index p, Formula::Ref phi, SortedTermSet* names) {
    if (phi->free_vars().empty()) {
      return phi;
    }
    Term x = *phi->free_vars().begin();
    Formula::Ref psi = ResOtherName(p, phi->Clone(), x, names);
    for (Term n : (*names)[x.sort()]) {
      Formula::Ref xi = ResName(p, phi->Clone(), x, n, names);
      psi = Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(xi)),
                                                       Formula::Factory::Not(std::move(psi))));
    }
    return psi;
  }

  Formula::Ref ResName(sphere_index p, Formula::Ref phi, Term x, Term n, SortedTermSet* names) {
    // (x == n -> RES(p, phi^x_n)) in clausal form
    phi->SubstituteFree(Term::SingleSubstitution(x, n), tf_);
    phi = Res(p, std::move(phi), names);
    Literal if_not = Literal::Neq(x, n);
    return Formula::Factory::Or(Formula::Factory::Atomic(Clause({if_not})), std::move(phi));
  }

  Formula::Ref ResOtherName(sphere_index p, Formula::Ref phi, Term x, SortedTermSet* names) {
    // (x != n1 && ... && x != nK -> RES(p, phi^x_n0)^n0_x) in clausal form
    Term n0 = tf_->CreateTerm(sf_->CreateName(x.sort()));
    phi->SubstituteFree(Term::SingleSubstitution(x, n0), tf_);
    names->insert(n0);
    phi = Res(p, std::move(phi), names);
    names->erase(n0);
    phi->SubstituteFree(Term::SingleSubstitution(n0, x), tf_);
    const SortedTermSet::value_type& ns = (*names)[x.sort()];
    const auto if_not = internal::transform_range(ns.begin(), ns.end(), [x](Term n) { return Literal::Eq(x, n); });
    return Formula::Factory::Or(Formula::Factory::Atomic(Clause(if_not.begin(), if_not.end())), std::move(phi));
  }

  static Formula::Ref bool_to_formula(bool b) {
    Formula::Ref falsum = Formula::Factory::Atomic(Clause({}));
    return b ? Formula::Factory::Not(std::move(falsum)) : std::move(falsum);
  }

  Symbol::Factory* sf_;
  Term::Factory* tf_;
  std::vector<Clause> knowledge_;
  std::vector<Conditional> beliefs_;
  std::vector<Solver> spheres_;
  Solver objective_;
#ifndef NDEBUG
  bool init_spheres_ = false;
#endif
};

}  // namespace lela

#endif  // LELA_MODAL_H_

