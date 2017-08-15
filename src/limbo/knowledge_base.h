// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016-2017 Christoph Schwering
// Licensed under the MIT license. See LICENSE file in the project root.
//
// A KnowledgeBase manages a knowledge base consisting of objective sentences
// or conditionals and evaluates queries in this knowledge base.
//
// The knowledge base is populated with Add(), whose argument shall be either a
// clause; an objective sentence whose normal form is a universally quantified
// clause; an objective sentence within Formula::Know() whose normal form is a
// universally quantified clause; or a Formula::Bel() such that the normal form
// of the material implication of antecedent and consequent is a universally
// quantified clause. Semantically, knowledge base is only-known.
//
// The optional Formula::Know() modality in formulas added to the knowledge base
// is fully ignored, including the belief level (an unconditional knowledge base
// is always only-known at belief level 0).
//
// For Formula::Bel() formulas added to the knowledge base, the belief levels do
// matter; they control how much effort is put into constructing the system of
// spheres.
//
// Queries are not subject to any syntactic restrictions. Technically, they are
// evaluated using variants of Levesque's representation theorem.

#ifndef LIMBO_KNOWLEDGE_BASE_H_
#define LIMBO_KNOWLEDGE_BASE_H_

#include <cassert>

#include <utility>
#include <vector>

#include <limbo/clause.h>
#include <limbo/formula.h>
#include <limbo/grounder.h>
#include <limbo/literal.h>
#include <limbo/solver.h>
#include <limbo/term.h>

#include <limbo/internal/ints.h>
#include <limbo/internal/iter.h>
#include <limbo/internal/maybe.h>

namespace limbo {

class KnowledgeBase {
 public:
  typedef internal::size_t size_t;
  typedef size_t sphere_index;
  typedef Formula::TermSet TermSet;
  typedef Formula::SortedTermSet SortedTermSet;

  KnowledgeBase(Symbol::Factory* sf, Term::Factory* tf) : sf_(sf), tf_(tf), real_world_(sf, tf) {
    spheres_.emplace_back(sf, tf);
  }

  KnowledgeBase(const KnowledgeBase&) = delete;
  KnowledgeBase& operator=(const KnowledgeBase&) = delete;
  KnowledgeBase(KnowledgeBase&&) = default;
  KnowledgeBase& operator=(KnowledgeBase&&) = default;

  void AddReal(Literal a) {
    real_facts_.push_back(Clause{a});
    a.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
  }

  void Add(const Clause& c) {
    assert(c.well_formed());
    knowledge_.push_back(c);
    c.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
  }

  bool Add(const Formula& alpha) {
    Formula::Ref beta = alpha.NF(sf_, tf_, false);
    bool assume_consistent = false;
    if (beta->type() == Formula::kGuarantee) {
      beta = beta->as_guarantee().arg().Clone();
      assume_consistent = true;
    }
    if (beta->type() == Formula::kBel) {
      const belief_level k = beta->as_bel().k();
      const belief_level l = beta->as_bel().l();
      const Formula& ante = beta->as_bel().antecedent();
      internal::Maybe<Clause> not_ante_or_conse = beta->as_bel().not_antecedent_or_consequent().AsUnivClause();
      if (not_ante_or_conse) {
        Add(k, l, ante, not_ante_or_conse.val, assume_consistent);
        return true;
      }
    } else {
      internal::Maybe<Clause> c = (beta->type() == Formula::kKnow ? beta->as_know().arg() : *beta).AsUnivClause();
      if (c) {
        Add(c.val);
        return true;
      }
    }
    return false;
  }

  bool Entails(const Formula& sigma, bool distribute = true) {
    assert(sigma.free_vars().all_empty());
    UpdateSpheres();
    Formula::Ref phi = ReduceModalities(*sigma.NF(sf_, tf_, distribute));
    assert(phi->objective());
    return real_world_.Entails(0, *phi, Solver::kNoConsistencyGuarantee);
  }

  sphere_index n_spheres() const { const_cast<KnowledgeBase&>(*this).UpdateSpheres(); return spheres_.size(); }
  Solver& sphere(sphere_index p) { UpdateSpheres(); return spheres_[p]; }
  const Solver& sphere(sphere_index p) const { const_cast<KnowledgeBase&>(*this).UpdateSpheres(); return spheres_[p]; }
  const std::vector<Solver>& spheres() const { const_cast<KnowledgeBase&>(*this).UpdateSpheres(); return spheres_; }

  const SortedTermSet& mentioned_names() const { return names_; }
  const TermSet& mentioned_names(Symbol::Sort sort) const { return names_[sort]; }

 private:
  typedef Formula::belief_level belief_level;

  struct Conditional {
    belief_level k;
    belief_level l;
    Formula::Ref ante;
    Clause not_ante_or_conse;
    bool assume_consistent;
  };

  void Add(belief_level k,
           belief_level l,
           const Formula& antecedent,
           const Clause& not_antecedent_or_consequent,
           bool assume_consistent) {
    beliefs_.push_back(Conditional{k, l, antecedent.Clone(), not_antecedent_or_consequent, assume_consistent});
    antecedent.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
    not_antecedent_or_consequent.Traverse([this](Term t) { if (t.name()) names_.insert(t); return true; });
  }

  void UpdateSpheres() {
    if (n_processed_real_facts_ == real_facts_.size() &&
        n_processed_beliefs_ == beliefs_.size() &&
        n_processed_knowledge_ == knowledge_.size()) {
      return;
    }
    real_world_.grounder().AddClauses(real_facts_.begin() + n_processed_real_facts_, real_facts_.end());
    if (beliefs_.empty()) {
      assert(spheres_.size() == 1);
      assert(n_processed_beliefs_ == 0);
      for (Solver& sphere : spheres_) {
        sphere.grounder().AddClauses(knowledge_.begin() + n_processed_knowledge_, knowledge_.end());
      }
    } else {
      spheres_.clear();
      std::vector<bool> done(beliefs_.size(), false);
      bool is_plausibility_consistent = true;
      size_t n_done = 0;
      size_t last_n_done;
      do {
        last_n_done = n_done;
        Solver sphere(sf_, tf_);
        auto is = internal::filter_range(internal::int_iterator<size_t>(0),
                                         internal::int_iterator<size_t>(beliefs_.size()),
                                         [this, &done](size_t i) { return !done[i]; });
        auto bs = internal::transform_range(is.begin(), is.end(),
                                            [this](size_t i) -> const Clause& {
                                              return beliefs_[i].not_ante_or_conse;
                                            });
        auto cs = internal::join_ranges(knowledge_.cbegin(), knowledge_.cend(), bs.begin(), bs.end());
        sphere.grounder().AddClauses(cs.begin(), cs.end());
        bool next_is_plausibility_consistent = true;
        for (size_t i = 0; i < beliefs_.size(); ++i) {
          const Conditional& c = beliefs_[i];
          if (!done[i]) {
            Grounder::Undo undo;
            if (c.assume_consistent) {
              sphere.grounder().GuaranteeConsistency(*c.ante, &undo);
            }
            const bool possibly_consistent = !sphere.Entails(c.k, *Formula::Factory::Not(c.ante->Clone()));
            if (possibly_consistent) {
              done[i] = true;
              ++n_done;
              const bool necessarily_consistent = sphere.Consistent(c.l, *c.ante);
              if (!necessarily_consistent) {
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
    n_processed_beliefs_ = beliefs_.size();
    n_processed_knowledge_ = knowledge_.size();
  }

  Formula::Ref ReduceModalities(const Formula& alpha) {
    switch (alpha.type()) {
      case Formula::kAtomic: {
        return alpha.Clone();
      }
      case Formula::kNot: {
        return Formula::Factory::Not(ReduceModalities(alpha.as_not().arg()));
      }
      case Formula::kOr: {
        return Formula::Factory::Or(ReduceModalities(alpha.as_or().lhs()), ReduceModalities(alpha.as_or().rhs()));
      }
      case Formula::kExists: {
        return Formula::Factory::Exists(alpha.as_exists().x(), ReduceModalities(alpha.as_exists().arg()));
      }
      case Formula::kKnow: {
        const sphere_index p = spheres_.size() - 1;
        Formula::Ref phi = ReduceModalities(alpha.as_know().arg());
        return ResEntails(p, alpha.as_know().k(), *phi);
      }
      case Formula::kCons: {
        const sphere_index p = spheres_.size() - 1;
        Formula::Ref phi = ReduceModalities(alpha.as_cons().arg());
        return ResConsistent(p, alpha.as_cons().k(), *phi);
      }
      case Formula::kBel: {
        const Formula::Ref ante = ReduceModalities(alpha.as_bel().antecedent());
        const Formula::Ref not_ante_or_conse = ReduceModalities(alpha.as_bel().not_antecedent_or_consequent());
        const belief_level k = alpha.as_bel().k();
        const belief_level l = alpha.as_bel().l();
        std::vector<Formula::Ref> consistent;
        std::vector<Formula::Ref> entails;
        for (sphere_index p = 0; p < spheres_.size(); ++p) {
          consistent.push_back(ResConsistent(p, l, *ante));
          entails.push_back(ResEntails(p, k, *not_ante_or_conse));
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
      case Formula::kGuarantee: {
        std::vector<Grounder::Undo> undos(spheres_.size());
        const Formula& beta = alpha.as_guarantee().arg();
        for (sphere_index p = 0; p < spheres_.size(); ++p) {
          spheres_[p].grounder().GuaranteeConsistency(beta, &undos[p]);
        }
        return ReduceModalities(beta);
      }
      case Formula::kAction:
        assert(false);
    }
    throw;
  }

  Formula::Ref ResEntails(sphere_index p, belief_level k, const Formula& phi) {
    // If phi is just a literal (t = n) or (t = x) for primitive t, we can
    // use Solver::Determines to speed things up.
    if (phi.type() == Formula::kAtomic) {
      const Clause& c = phi.as_atomic().arg();
      if (c.unit()) {
        Literal a = c.first();
        // Currently we enable this only for (t = x) and not for (t = n), for
        // the latter introduces a new temporary variable which in turn leads
        // to additional names for grounding.
        // TODO Make Grounder::PrepareForQuery() more efficient for (t = n).
        if (a.lhs().primitive() && a.pos() && a.rhs().variable()) {
          internal::Maybe<Term> r = spheres_[p].Determines(k, a.lhs());
          if (a.rhs().name()) {
            return bool_to_formula(r && (r.val.null() || r.val == a.rhs()));
          } else if (a.rhs().variable()) {
            if (r) {
              if (r.val.null()) {
                return bool_to_formula(true);
              } else {
                return Formula::Factory::Atomic(Clause(Literal::Eq(a.rhs(), r.val)));
              }
            } else {
              return bool_to_formula(false);
            }
          }
        }
      }
    }
    auto if_no_free_vars = [k, this](Solver* sphere, const Formula& psi) {
      return sphere->Entails(k, psi);
    };
    return Res(p, phi.Clone(), if_no_free_vars);
  }

  Formula::Ref ResConsistent(sphere_index p, belief_level k, const Formula& phi) {
    auto if_no_free_vars = [k, this](Solver* sphere, const Formula& psi) {
      return sphere->Consistent(k, psi);
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
    if (phi->free_vars().all_empty()) {
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
  Formula::Ref ResName(sphere_index p,
                       Formula::Ref phi,
                       Term x,
                       Term n,
                       SortedTermSet* names,
                       BinaryPredicate if_no_free_vars) {
    // (x == n -> RES(p, phi^x_n)) in clausal form
    phi->SubstituteFree(Term::Substitution(x, n), tf_);
    phi = Res(p, std::move(phi), names, if_no_free_vars);
    Literal if_not = Literal::Neq(x, n);
    return Formula::Factory::Or(Formula::Factory::Atomic(Clause(if_not)), std::move(phi));
  }

  template<typename BinaryPredicate>
  Formula::Ref ResOtherName(sphere_index p,
                            Formula::Ref phi,
                            Term x,
                            SortedTermSet* names,
                            BinaryPredicate if_no_free_vars) {
    // (x != n1 && ... && x != nK -> RES(p, phi^x_n0)^n0_x) in clausal form
    Term n0 = spheres_[p].grounder().temp_name_pool().Create(x.sort());
    phi->SubstituteFree(Term::Substitution(x, n0), tf_);
    names->insert(n0);
    phi = Res(p, std::move(phi), names, if_no_free_vars);
    names->erase(n0);
    phi->SubstituteFree(Term::Substitution(n0, x), tf_);
    spheres_[p].grounder().temp_name_pool().Return(n0);
    const TermSet& ns = (*names)[x.sort()];
    const auto if_not = internal::transform_range(ns.begin(), ns.end(), [x](Term n) { return Literal::Eq(x, n); });
    const Clause c(ns.size(), if_not.begin(), if_not.end());
    return Formula::Factory::Or(Formula::Factory::Atomic(c), std::move(phi));
  }

  static Formula::Ref bool_to_formula(bool b) {
    Formula::Ref falsum = Formula::Factory::Atomic(Clause());
    return b ? Formula::Factory::Not(std::move(falsum)) : std::move(falsum);
  }

  Symbol::Factory* sf_;
  Term::Factory* tf_;
  std::vector<Clause> real_facts_;
  std::vector<Clause> knowledge_;
  std::vector<Conditional> beliefs_;
  SortedTermSet names_;
  std::vector<Solver> spheres_;
  Solver real_world_;
  size_t n_processed_real_facts_ = 0;
  size_t n_processed_knowledge_ = 0;
  size_t n_processed_beliefs_ = 0;
};

}  // namespace limbo

#endif  // LIMBO_KNOWLEDGE_BASE_H_

