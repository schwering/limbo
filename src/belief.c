// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "belief.h"
#include "memory.h"
#include <assert.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))

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
VECTOR_IMPL(bsetup, setup_t *, setup_cmp);
VECTOR_IMPL(pelsets, pelset_t *, pelset_cmp);
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
    for (EACH_CONST(belief_conds, beliefs, i)) {
        const belief_cond_t *bc = i.val;
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

stdset_t bbat_hplus(
        const univ_clauses_t *static_bat,
        const belief_conds_t *beliefs,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *query_names,
        int n_query_vars)
{
    stdset_t names = stdset_copy(query_names);
    int max_vars = n_query_vars;
    for (EACH_CONST(belief_conds, beliefs, i)) {
        const belief_cond_t *bc = i.val;
        stdset_add_all(&names, &bc->neg_phi->names);
        stdset_add_all(&names, &bc->psi->names);
        const varset_t vars = varset_union(&bc->neg_phi->vars, &bc->psi->vars);
        max_vars = MAX(max_vars, varset_size(&vars));
    }
    return bat_hplus(static_bat, dynamic_bat, &names, max_vars);
}

pelsets_t pelsets_deep_copy(const pelsets_t *pels)
{
    pelsets_t copy = pelsets_init_with_size(pelsets_size(pels));
    for (EACH_CONST(pelsets, pels, i)) {
        const pelset_t *pel = i.val;
        pelset_t *new_pel = MALLOC(sizeof(pelset_t));
        *new_pel = pelset_copy(pel);
        pelsets_append(&copy, new_pel);
    }
    return copy;
}

pelsets_t pelsets_deep_lazy_copy(const pelsets_t *pels)
{
    pelsets_t copy = pelsets_init_with_size(pelsets_size(pels));
    for (EACH_CONST(pelsets, pels, i)) {
        const pelset_t *pel = i.val;
        pelset_t *new_pel = MALLOC(sizeof(pelset_t));
        *new_pel = pelset_lazy_copy(pel);
        pelsets_append(&copy, new_pel);
    }
    return copy;
}

bsetup_t bsetup_init_beliefs(
        const setup_t *static_bat_setup,
        const belief_conds_t *beliefs,
        const stdset_t *hplus,
        const int k)
{
    const int m = belief_conds_size(beliefs);
    bsetup_t setups = bsetup_init_with_size(m + 1);
    ground_belief_conds_t gbcs = beliefs_ground(beliefs, hplus);
    bool satisfied_belief_cond;
    do {
        setup_t *setup = MALLOC(sizeof(setup_t));
        *setup = setup_copy(static_bat_setup);
        for (EACH_CONST(ground_belief_conds, &gbcs, i)) {
            const ground_belief_cond_t *gbc = i.val;
            setup_add(setup, gbc->neg_phi_or_psi);
        }
        //setup_minimize(setup);
        //setup_propagate_units(setup);
        const pelset_t pel = setup_pel(setup);
        satisfied_belief_cond = false;
        for (EACH(ground_belief_conds, &gbcs, i)) {
            const ground_belief_cond_t *gbc = i.val;
            if (!setup_with_splits_and_sf_subsumes(setup, &pel,
                        gbc->neg_phi, k)) {
                ground_belief_conds_iter_remove(&i);
                satisfied_belief_cond = true;
            }
        }
        bsetup_append(&setups, setup);
    } while (satisfied_belief_cond);
    assert(bsetup_size(&setups) <= m + 1);
    return setups;
}

bsetup_t bsetup_unions(const bsetup_t *l, const setup_t *r)
{
    bsetup_t us = bsetup_init_with_size(bsetup_size(l));
    for (EACH_CONST(bsetup, l, i)) {
        const setup_t *setup = i.val;
        setup_t *u = MALLOC(sizeof(setup_t));
        *u = setup_union(setup, r);
        bsetup_append(&us, u);
    }
    return us;
}

bsetup_t bsetup_deep_copy(const bsetup_t *setups)
{
    bsetup_t new_setups = bsetup_init_with_size(bsetup_size(setups));
    for (EACH_CONST(bsetup, setups, i)) {
        const setup_t *setup = i.val;
        setup_t *new_setup = MALLOC(sizeof(setup_t));
        *new_setup = setup_copy(setup);
        bsetup_append(&new_setups, new_setup);
    }
    return new_setups;
}

void bsetup_add_sensing_results(
        bsetup_t *setups,
        const splitset_t *sensing_results)
{
    for (EACH_CONST(bsetup, setups, i)) {
        setup_t *setup = i.val_unsafe;
        setup_add_sensing_results(setup, sensing_results);
    }
}

pelsets_t bsetup_pels(const bsetup_t *setups)
{
    pelsets_t pels = pelsets_init_with_size(bsetup_size(setups));
    for (EACH_CONST(bsetup, setups, i)) {
        const setup_t *setup = i.val;
        pelset_t *pel = MALLOC(sizeof(pelset_t));
        *pel = setup_pel(setup);
        pelsets_append(&pels, pel);
    }
    return pels;
}

void bsetup_propagate_units(bsetup_t *setups)
{
    for (EACH_CONST(bsetup, setups, i)) {
        setup_t *setup = i.val_unsafe;
        setup_propagate_units(setup);
    }
}

bool bsetup_with_splits_and_sf_subsumes(
        bsetup_t *setups,
        const pelsets_t *pels,
        const clause_t *c,
        const int k,
        int *plausibility)
{
    assert(bsetup_size(setups) == pelsets_size(pels));
    bsetup_const_iter_t i = bsetup_const_iter(setups);
    pelsets_const_iter_t j = pelsets_const_iter(pels);
    if (plausibility != NULL) {
        *plausibility = -1;
    }
    while (bsetup_const_iter_next(&i) && pelsets_const_iter_next(&j)) {
        setup_t *setup = i.val_unsafe;
        const pelset_t *pel = j.val;
        const bool r = setup_with_splits_and_sf_subsumes(setup, pel, c, k);
        if (plausibility != NULL) {
            ++(*plausibility);
        }
        if (!setup_is_inconsistent(setup)) {
            return r;
        }
    }
    return false;
}

