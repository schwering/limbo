// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#ifndef LELA_MODAL_H_
#define LELA_MODAL_H_

#include <cassert>

#include <utility>
#include <vector>

#include <lela/formula.h>
#include <lela/solver.h>
#include <lela/internal/ints.h>

namespace lela {

class KnowledgeBase {
 public:
  typedef internal::size_t size_t;
  typedef size_t sphere_index;

  KnowledgeBase(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf), objective_(sf, tf) {
    spheres_.push_back(Solver(sf, tf));
  }

  KnowledgeBase(const KnowledgeBase&) = delete;
  KnowledgeBase& operator=(const KnowledgeBase&) = delete;
  KnowledgeBase(KnowledgeBase&&) = default;
  KnowledgeBase& operator=(KnowledgeBase&&) = default;

  void Add(const Clause& c) {
    for (Solver& sphere : spheres_) {
      sphere.AddClause(c);
    }
    knowledge_.push_back(c);
    c.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
  }

  void Add(Formula::split_level k, Formula::split_level l,
           const Formula& antecedent, const Clause& not_antecedent_or_consequent) {
    beliefs_.push_back(Conditional{k, l, antecedent.Clone(), not_antecedent_or_consequent});
    spheres_changed_ = true;
    antecedent.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
    not_antecedent_or_consequent.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
  }

  bool Add(const Formula& alpha) {
    Formula::Ref psi = alpha.NF(sf_, tf_);
    if (psi->type() == Formula::kBel) {
      const Formula::split_level k = psi->as_bel().k();
      const Formula::split_level l = psi->as_bel().l();
      const Formula& ante = psi->as_bel().antecedent();
      internal::Maybe<Clause> not_ante_or_conse = psi->as_bel().not_antecedent_or_consequent().AsUnivClause();
      if (not_ante_or_conse) {
        Add(k, l, ante, not_ante_or_conse.val);
        return true;
      }
    } else {
      internal::Maybe<Clause> c = (psi->type() == Formula::kKnow ? psi->as_know().arg() : *psi).AsUnivClause();
      if (c) {
        Add(c.val);
        return true;
      }
    }
    return false;
  }

  bool Entails(const Formula& sigma, bool assume_consistent = true) {
    assert(sigma.subjective());
    if (spheres_changed_) {
      BuildSpheres(assume_consistent);
      spheres_changed_ = false;
    }
    Formula::Ref sigma_nf = sigma.NF(sf_, tf_);
    Formula::Ref phi = ReduceModalities(*sigma_nf, assume_consistent);
    assert(phi->objective());
    return objective_.Entails(0, *phi);
  }

  sphere_index n_spheres() const { return spheres_.size(); }
  Solver* sphere(sphere_index p) { return &spheres_[p]; }
  const Solver& sphere(sphere_index p) const { return spheres_[p]; }
  const std::vector<Solver>& spheres() const { return spheres_; }

 private:
  struct Conditional {
    Formula::split_level k;
    Formula::split_level l;
    Formula::Ref ante;
    Clause not_ante_or_conse;
  };
  typedef Grounder::TermSet TermSet;
  typedef Grounder::SortedTermSet SortedTermSet;

  void BuildSpheres(bool assume_consistent) {
    spheres_.clear();
    std::vector<bool> done(beliefs_.size(), false);
    bool is_plausibility_consistent = true;
    size_t n_done = 0;
    size_t last_n_done;
    do {
      last_n_done = n_done;
      Solver sphere(sf_, tf_);
      for (const Clause& c : knowledge_) {
        sphere.AddClause(c);
      }
      for (size_t i = 0; i < beliefs_.size(); ++i) {
        const Conditional& c = beliefs_[i];
        if (!done[i]) {
          sphere.AddClause(c.not_ante_or_conse);
        }
      }
      bool next_is_plausibility_consistent = true;
      for (size_t i = 0; i < beliefs_.size(); ++i) {
        const Conditional& c = beliefs_[i];
        if (!done[i]) {
          const bool possibly_consistent = !sphere.Entails(c.k, *Formula::Factory::Not(c.ante->Clone()),
                                                           assume_consistent);
          if (possibly_consistent) {
            done[i] = true;
            ++n_done;
            const bool necessarilyConsistent = sphere.Consistent(c.l, *c.ante, assume_consistent);
            if (!necessarilyConsistent) {
              next_is_plausibility_consistent = false;
            }
          }
        }
      }
      if (is_plausibility_consistent || n_done == last_n_done) {
        spheres_.push_back(std::move(sphere));
      }
      is_plausibility_consistent = next_is_plausibility_consistent;
    } while (n_done > last_n_done);
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
        const sphere_index p = n_spheres() - 1;
        Formula::Ref phi = ReduceModalities(alpha.as_cons().arg(), assume_consistent);
        return ResConsistent(p, alpha.as_cons().k(), *phi, assume_consistent);
      }
      case Formula::kBel: {
        const Formula::Ref ante = ReduceModalities(alpha.as_bel().antecedent(), assume_consistent);
        const Formula::Ref not_ante_or_conse = ReduceModalities(alpha.as_bel().not_antecedent_or_consequent(),
                                                                assume_consistent);
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
    throw;
  }

  Formula::Ref ResEntails(sphere_index p, Formula::split_level k, const Formula& phi, bool assume_consistent) {
    auto if_no_free_vars = [k, assume_consistent, this](Solver* sphere, const Formula& psi) {
      return sphere->Entails(k, psi, assume_consistent);
    };
    return Res(p, phi.Clone(), if_no_free_vars);
  }

  Formula::Ref ResConsistent(sphere_index p, Formula::split_level k, const Formula& phi, bool assume_consistent) {
    auto if_no_free_vars = [k, assume_consistent, this](Solver* sphere, const Formula& psi) {
      return sphere->Consistent(k, psi, assume_consistent);
    };
    return Res(p, phi.Clone(), if_no_free_vars);
  }

  template<typename BinaryPredicate>
  Formula::Ref Res(sphere_index p, Formula::Ref phi, BinaryPredicate if_no_free_vars) {
    SortedTermSet names = names_;
    phi->Traverse([&names](Term t) { if (t.name()) names.insert(t); return true; });
    return Res(p, std::move(phi), &names, if_no_free_vars);
  }

  template<typename BinaryPredicate>
  Formula::Ref Res(sphere_index p, Formula::Ref phi, SortedTermSet* names, BinaryPredicate if_no_free_vars) {
    if (phi->free_vars().empty()) {
      const bool r = if_no_free_vars(&spheres_[p], *phi);
      return bool_to_formula(r);
    }
    Term x = *phi->free_vars().begin();
    Formula::Ref psi = ResOtherName(p, phi->Clone(), x, names, if_no_free_vars);
    for (Term n : (*names)[x.sort()]) {
      Formula::Ref xi = ResName(p, phi->Clone(), x, n, names, if_no_free_vars);
      psi = Formula::Factory::Not(Formula::Factory::Or(Formula::Factory::Not(std::move(xi)),
                                                       Formula::Factory::Not(std::move(psi))));
    }
    return psi;
  }

  template<typename BinaryPredicate>
  Formula::Ref ResName(sphere_index p, Formula::Ref phi, Term x, Term n, SortedTermSet* names, BinaryPredicate if_no_free_vars) {
    // (x == n -> RES(p, phi^x_n)) in clausal form
    phi->SubstituteFree(Term::Substitution(x, n), tf_);
    phi = Res(p, std::move(phi), names, if_no_free_vars);
    Literal if_not = Literal::Neq(x, n);
    return Formula::Factory::Or(Formula::Factory::Atomic(Clause({if_not})), std::move(phi));
  }

  template<typename BinaryPredicate>
  Formula::Ref ResOtherName(sphere_index p, Formula::Ref phi, Term x, SortedTermSet* names, BinaryPredicate if_no_free_vars) {
    // (x != n1 && ... && x != nK -> RES(p, phi^x_n0)^n0_x) in clausal form
    Term n0 = spheres_[p].grounder()->CreateName(x.sort());
    phi->SubstituteFree(Term::Substitution(x, n0), tf_);
    names->insert(n0);
    phi = Res(p, std::move(phi), names, if_no_free_vars);
    names->erase(n0);
    phi->SubstituteFree(Term::Substitution(n0, x), tf_);
    spheres_[p].grounder()->ReturnName(n0);
    const TermSet& ns = (*names)[x.sort()];
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
  SortedTermSet names_;
  std::vector<Solver> spheres_;
  Solver objective_;
  bool spheres_changed_ = false;
};

}  // namespace lela

#endif  // LELA_MODAL_H_

