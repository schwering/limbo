// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Reasoning about beliefs and belief revision.
 * At the core of our notion of belief there are belief conditionals which have
 * the form phi => psi and express that in the most plausible scenario where phi
 * holds, psi holds as well.
 *
 * In ESB, the belief conditional arrow `=>' is translated to material
 * implication plus some other conditions.
 * Since ESL allows only proper+ formulas, which are disjunctions of formulas,
 * we require phi in phi => psi to be a conjunction and psi to be a disjunction.
 * For technical reasons we do not explicitly introduce conjunctions but rather
 * use neg_phi which shall be a disjunction which is the negation of phi.
 *
 * Thus, we can construct a model for belief conditionals using the procedure
 * described in the proof of Theorem 7 from the ESB paper:
 * Let p := 0 and S be the set of belief conditionals not yet satisfied.
 * Compute the setup that satisfies, for all phi => psi in S, (neg_phi v psi),
 * which represents the material implication from rule 12(a).
 * If this setup satisfies neg_phi, rule 12(c) is violated for that belief
 * conditional, and therefore we leave phi => psi in S; otherwise, the belief
 * conditional is satisfied and we can remove phi => psi from S.
 * Repeat that loop until p > m where m is the total number of belief
 * conditionals.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _BELIEF_H_
#define _BELIEF_H_

#include "setup.h"

typedef struct belief_cond belief_cond_t;

VECTOR_DECL(belief_conds, belief_cond_t *);
VECTOR_DECL(bsetup, setup_t *);

const belief_cond_t *belief_cond_init(
        const ewff_t *cond,
        const clause_t *neg_phi,
        const clause_t *psi);

stdset_t bbat_hplus(
        const univ_clauses_t *static_bat,
        const belief_conds_t *beliefs,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *query_names,
        int n_query_vars);

bsetup_t bsetup_init_beliefs(
        const setup_t *static_bat_setup,
        const belief_conds_t *beliefs,
        const stdset_t *hplus,
        const int k);
bsetup_t bsetup_unions(const bsetup_t *l, const setup_t *r);
bsetup_t bsetup_deep_copy(const bsetup_t *setups);

void bsetup_guarantee_consistency(bsetup_t *setups, const int k);

void bsetup_add_sensing_result(
        bsetup_t *setups,
        const stdvec_t *z,
        const stdname_t n,
        const bool r);

void bsetup_minimize(bsetup_t *setups);
void bsetup_propagate_units(bsetup_t *setups);

bool bsetup_entails(bsetup_t *setups, const clause_t *c, const int k, int *pl);

#endif

