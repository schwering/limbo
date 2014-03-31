// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "clause.h"
#include <assert.h>
#include <stdlib.h>

#define MAX(x,y) ((x) > (y) ? (x) : (y))

SET_IMPL(pelset, literal_t *, literal_cmp);
SET_IMPL(clause, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);
VECTOR_IMPL(univ_clauses, univ_clause_t *, NULL);
VECTOR_IMPL(box_univ_clauses, box_univ_clause_t *, NULL);

static void ground_univ(setup_t *setup, varmap_t *varmap,
        const univ_clause_t *univ_clause, const stdset_t *ns, int i)
{
    if (i < varset_size(&univ_clause->vars)) {
        const var_t var = varset_get(&univ_clause->vars, i);
        for (int j = 0; j < stdset_size(ns); ++j) {
            const stdname_t n = stdset_get(ns, j);
            varmap_add_replace(varmap, var, n);
            ground_univ(setup, varmap, univ_clause, ns, i+1);
        }
    } else {
        const clause_t *c = univ_clause->univ_clause(varmap);
        if (c != NULL) {
            setup_add(setup, c);
        }
    }
}

static void ground_box(setup_t *setup, const stdvec_t *z, const clause_t *c)
{
    clause_t *d = malloc(sizeof(clause_t));
    *d = clause_init_with_size(clause_size(c));
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        const stdvec_t zz = stdvec_copy_append_all(z, literal_z(l));
        literal_t *ll = malloc(sizeof(literal_t));
        *ll = literal_init(&zz, literal_sign(l), literal_pred(l),
                literal_args(l));
        const bool added = clause_add(d, ll);
        assert(added);
    }
    const bool added = setup_add(setup, d);
    if (!added) {
        for (int i = 0; i < clause_size(d); ++i) {
            free((literal_t *) clause_get(d, i));
        }
        free(d);
    }
}

static stdset_t hplus(
        const box_univ_clauses_t *box_cs,
        const univ_clauses_t *cs,
        const stdset_t *query_names,
        int n_query_vars)
{
    stdset_t names = stdset_copy(query_names);
    int max_vars = n_query_vars;
    for (int i = 0; i < box_univ_clauses_size(box_cs); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(box_cs, i);
        stdset_add_all(&names, &c->c.names);
        max_vars = MAX(max_vars, varset_size(&c->c.vars));
    }
    for (int i = 0; i < univ_clauses_size(cs); ++i) {
        const univ_clause_t *c = univ_clauses_get(cs, i);
        stdset_add_all(&names, &c->names);
        max_vars = MAX(max_vars, varset_size(&c->vars));
    }
    stdname_t max_name = 0;
    for (int i = 0; i < stdset_size(&names); ++i) {
        max_name = MAX(max_name, stdset_get(&names, i));
    }
    assert(max_name < STDNAME_MAX);
    for (int i = 0; i < max_vars; ++i) {
        assert(max_name + i + 1 < STDNAME_MAX);
        stdset_add(&names, max_name + i + 1);
    }
    return names;
}

setup_t setup_ground_clauses(
        const box_univ_clauses_t *dynamic_bat,
        const univ_clauses_t *static_bat,
        const stdvecset_t *query_zs,
        const stdset_t *query_ns,
        int n_query_vars)
{
    const stdset_t ns = hplus(dynamic_bat, static_bat, query_ns, n_query_vars);
    setup_t setup = setup_init();
    // first ground the static universally quantified clauses
    for (int i = 0; i < univ_clauses_size(static_bat); ++i) {
        const univ_clause_t *c = univ_clauses_get(static_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->vars));
        ground_univ(&setup, &varmap, c, &ns, 0);
    }
    setup_t box_clauses = setup_init();
    // first ground the universal quantifiers
    // this is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i].vars) and
    // standard names (elements of ns) for all i
    for (int i = 0; i < box_univ_clauses_size(dynamic_bat); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(dynamic_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->c.vars));
        ground_univ(&box_clauses, &varmap, &c->c, &ns, 0);
    }
    // ground each box by substituting it with the prefixes of all action
    // sequences
    for (int i = 0; i < setup_size(&box_clauses); ++i) {
        const clause_t *c = setup_get(&box_clauses, i);
        for (int j = 0; j < stdvecset_size(query_zs); ++j) {
            const stdvec_t *z = stdvecset_get(query_zs, j);
            for (int k = 0; k <= stdvec_size(z); ++k) {
                const stdvec_t z_prefix = stdvec_sub(z, 0, k);
                ground_box(&setup, &z_prefix, c);
            }
        }
    }
    return setup;
}

pelset_t setup_pel(const setup_t *setup)
{
    pelset_t pel = pelset_init();
    for (int i = 0; i < setup_size(setup); ++i) {
        const clause_t *c = setup_get(setup, i);
        for (int j = 0; j < clause_size(c); ++j) {
            const literal_t *l = clause_get(c, j);
            if (!literal_sign(l)) {
                literal_t *ll = malloc(sizeof(literal_t));
                *ll = literal_flip(l);
                const bool added = pelset_add(&pel, ll);
                if (!added) {
                    free(ll);
                }
            } else {
                pelset_add(&pel, l);
            }
        }
    }
    return pel;
}

bool clause_is_resolvable(const clause_t *c, const literal_t *l)
{
    const literal_t ll = literal_flip(l);
    return clause_contains(c, &ll);
}

clause_t clause_resolve(const clause_t *c, const literal_t *l)
{
    const literal_t ll = literal_flip(l);
    clause_t cc = *c;
    const bool removed = clause_remove(&cc, &ll);
    assert(removed);
    return cc;
}

bool clause_is_unit(const clause_t *c)
{
    return clause_size(c) == 1;
}

literal_t clause_unit(const clause_t *c)
{
    assert(clause_is_unit(c));
    return *clause_get(c, 0);
}

void unit_propagation(setup_t *setup)
{
}

