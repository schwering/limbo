// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "belief.h"
#include "memory.h"
#include <assert.h>

struct belief_cond {
    const univ_clause_t *neg_phi;
    const univ_clause_t *psi;
};

typedef struct {
    const clause_t *neg_phi;
    const clause_t *neg_phi_or_psi;
} ground_belief_cond_t;

static int ground_belief_cond_cmp(const ground_belief_cond_t *l,
        const ground_belief_cond_t *r)
{
    return clause_cmp(l->neg_phi_or_psi, r->neg_phi_or_psi);
}

VECTOR_IMPL(belief_conds, belief_cond_t *, NULL);
VECTOR_IMPL(ranked_setups, setup_t *, setup_cmp);
SET_DECL(ground_belief_conds, ground_belief_cond_t *);
SET_IMPL(ground_belief_conds, ground_belief_cond_t *, ground_belief_cond_cmp);

const belief_cond_t *belief_cond_init(
        const ewff_t *cond,
        const clause_t *neg_phi,
        const clause_t *psi)
{
    belief_cond_t *bc = MALLOC(sizeof(belief_cond_t));
    *bc = (belief_cond_t) {
        .neg_phi = univ_clause_init(cond, neg_phi),
        .psi     = univ_clause_init(cond, psi)
    };
    return bc;
}

static ground_belief_conds_t beliefs_ground(
        const belief_conds_t *beliefs,
        const stdset_t *hplus)
{
    ground_belief_conds_t gbcs = ground_belief_conds_init();
    for (int i = 0; i < belief_conds_size(beliefs); ++i) {
        const belief_cond_t *bc = belief_conds_get(beliefs, i);
        const varset_t vars = varset_union(&bc->neg_phi->vars, &bc->psi->vars);
        void ground_belief_cond(const varmap_t *varmap)
        {
            const clause_t *neg_phi = ({
                const clause_t *c;
                if (varset_size(&bc->neg_phi->vars) > 0) {
                    clause_t *d = MALLOC(sizeof(clause_t));
                    *d = clause_substitute(bc->neg_phi->clause, varmap);
                    c = d;
                } else {
                    c = bc->neg_phi->clause;
                }
                c;
            });
            const clause_t *psi = ({
                const clause_t *c;
                if (varset_size(&bc->psi->vars) > 0) {
                    clause_t *d = MALLOC(sizeof(clause_t));
                    *d = clause_substitute(bc->psi->clause, varmap);
                    c = d;
                } else {
                    c = bc->psi->clause;
                }
                c;
            });
            const clause_t *neg_phi_or_psi = ({
                clause_t *c = MALLOC(sizeof(clause_t));
                *c = clause_union(neg_phi, psi);
                c;
            });
            ground_belief_cond_t *gbc = MALLOC(sizeof(ground_belief_cond_t));
            *gbc = (ground_belief_cond_t) {
                .neg_phi        = neg_phi,
                .neg_phi_or_psi = neg_phi_or_psi
            };
            ground_belief_conds_add(&gbcs, gbc);
        }
        ewff_ground(bc->neg_phi->cond, &vars, hplus, &ground_belief_cond);
    }
    return gbcs;
}

ranked_setups_t setup_init_beliefs(
        const setup_t *static_bat_setup,
        const belief_conds_t *beliefs,
        const stdset_t *hplus,
        const int k)
{
    const int m = belief_conds_size(beliefs);
    ranked_setups_t setups = ranked_setups_init_with_size(m+1);
    ground_belief_conds_t gbcs = beliefs_ground(beliefs, hplus);
    while (ground_belief_conds_size(&gbcs) > 0 &&
            ranked_setups_size(&setups) <= m) {
        setup_t *setup = MALLOC(sizeof(setup_t));
        *setup = setup_copy(static_bat_setup);
        for (int i = 0; i < ground_belief_conds_size(&gbcs); ++i) {
            const ground_belief_cond_t *gbc = ground_belief_conds_get(&gbcs, i);
            setup_add(setup, gbc->neg_phi_or_psi);
        }
        setup_minimize(setup);
        setup_propagate_units(setup);
        const pelset_t orig_pel = setup_pel(setup);
        for (int i = 0; i < ground_belief_conds_size(&gbcs); ++i) {
            const ground_belief_cond_t *gbc = ground_belief_conds_get(&gbcs, i);
            pelset_t pel = pelset_lazy_copy(&orig_pel);
            if (setup_with_splits_subsumes(setup, &pel, gbc->neg_phi, k)) {
                ground_belief_conds_remove_index(&gbcs, i--);
            }
        }
        ranked_setups_append(&setups, setup);
    }
    return setups;
}

