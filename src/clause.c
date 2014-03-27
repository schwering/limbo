// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "clause.h"
#include <assert.h>
#include <stdlib.h>

#define MAX(x,y) ((x) > (y) ? (x) : (y))

SET_IMPL(pelset, literal_t *, literal_cmp);
SET_IMPL(clause, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);

static void ground_univs(setup_t *setup, varmap_t *varmap,
        const univ_clause_t *univ_clause, const stdset_t *ns, int i)
{
    if (i < varset_size(&univ_clause->vars)) {
        const var_t var = varset_get(&univ_clause->vars, i);
        for (int j = 0; j < stdset_size(ns); ++j) {
            const stdname_t n = stdset_get(ns, j);
            varmap_add_replace(varmap, var, n);
            ground_univs(setup, varmap, univ_clause, ns, i+1);
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
    bool added;
    clause_t *d = malloc(sizeof(clause_t));
    *d = clause_init_with_size(clause_size(c));
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l1 = clause_get(c, i);
        literal_t *l2 = malloc(sizeof(literal_t));
        *l2 = literal_init(z, literal_sign(l1), literal_pred(l1),
                literal_args(l1));
        added = clause_add(d, l2);
        assert(added);
    }
    added = setup_add(setup, d);
    if (!added) {
        for (int i = 0; i < clause_size(d); ++i) {
            free((literal_t *) clause_get(d, i));
        }
        free(d);
    }
}

static stdset_t hplus(const univ_clause_t *univ_clauses[], int n_univ_clauses,
        const stdset_t *query_names, int n_query_vars)
{
    stdset_t names = stdset_copy(query_names);
    int max_vars = n_query_vars;
    for (int i = 0; i < n_univ_clauses; ++i) {
        stdset_add_all(&names, &univ_clauses[i]->names);
        max_vars = MAX(max_vars, varset_size(&univ_clauses[i]->vars));
    }
    stdname_t max_name = 0;
    for (int i = 0; i < stdset_size(&names); ++i) {
        max_name = MAX(max_name, stdset_get(&names, i));
    }
    assert(max_name < STDNAME_MAX);
    for (int i = 0; i < n_query_vars; ++i) {
        assert(max_name + i + 1 < STDNAME_MAX);
        stdset_add(&names, max_name + i + 1);
    }
    return names;
}

setup_t ground_clauses(
        const univ_clause_t *univ_clauses[], int n_univ_clauses,
        const stdvecset_t *query_zs, const stdset_t *query_ns, int n_query_vars)
{
    const stdset_t ns = hplus(univ_clauses, n_univ_clauses,
            query_ns, n_query_vars);
    setup_t box_clauses = setup_init();
    // first ground the universal quantifiers
    // this is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i]->vars) and
    // standard names (elements of ns) for all i
    for (int i = 0; i < n_univ_clauses; ++i) {
        varmap_t varmap = varmap_init_with_size(
                varset_size(&univ_clauses[i]->vars));
        ground_univs(&box_clauses, &varmap, univ_clauses[i], &ns, 0);
    }
    // ground each box by substituting it with the prefixes of all action
    // sequences
    setup_t setup = setup_init();
    for (int i = 0; i < setup_size(&box_clauses); ++i) {
        const clause_t *c = setup_get(&box_clauses, i);
        for (int j = 0; j < stdvecset_size(query_zs); ++j) {
            const stdvec_t *z = stdvecset_get(query_zs, j);
            for (int k = 0; k < stdvec_size(z); ++k) {
                const stdvec_t z_prefix = stdvec_sub(z, 0, k);
                ground_box(&setup, &z_prefix, c);
            }
        }
    }
    return setup;
}

pelset_t pel(const setup_t *setup)
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

