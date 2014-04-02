// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "setup.h"
#include <assert.h>
#include <stdlib.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define SWAP(x,y)   ({ const typeof(x) tmp = x; x = y; y = tmp; })

SET_IMPL(litset, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);
VECTOR_IMPL(univ_clauses, univ_clause_t *, NULL);
VECTOR_IMPL(box_univ_clauses, box_univ_clause_t *, NULL);

static bool clause_is_empty(const clause_t *c)
{
    return clause_size(c) == 0;
}

static bool clause_is_unit(const clause_t *c)
{
    return clause_size(c) == 1;
}

static const literal_t *clause_unit(const clause_t *c)
{
    assert(clause_is_unit(c));
    return clause_get(c, 0);
}

static const clause_t *clause_resolve(const clause_t *c, const litset_t *u)
{
    clause_t *d = malloc(sizeof(clause_t));
    *d = clause_lazy_copy(c);
    int *indices = malloc(clause_size(c) * sizeof(int));
    int n_indices = 0;
    for (int i = 0; i < litset_size(u); ++i) {
        const literal_t l = literal_flip(litset_get(u, i));
        const int j = clause_find(d, &l);
        if (j != -1) {
            indices[n_indices++] = j;
        }
    }
    clause_remove_all_indices(d, indices, n_indices);
    free(indices);
    return d;
}

const clause_t *clause_empty(void)
{
    static clause_t c;
    static clause_t *ptr = NULL;
    if (ptr == NULL) {
        c = clause_init_with_size(1);
        ptr = &c;
    }
    return ptr;
}

bool clause_subsumes(const clause_t *c, const clause_t *d)
{
    return clause_contains_all(d, c);
}

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
        const stdvec_t zz = stdvec_concat(z, literal_z(l));
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
                const stdvec_t z_prefix = stdvec_lazy_copy_range(z, 0, k);
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

static void add_unit_clauses_from_setup(litset_t *ls, const setup_t *setup)
{
    for (int i = 0; i < setup_size(setup); ++i) {
        const clause_t *c = setup_get(setup, i);
        if (clause_is_empty(c)) {
            continue;
        } else if (clause_is_unit(c)) {
            litset_add(ls, clause_unit(c));
        } else {
            // Clauses are sets, which are again stored as vectors, and
            // vector_cmp()'s first ordering criterion is the length.
            // Thus, a unit clause is shorter than all non-empty and non-unit
            // clauses. Therefore we only need to iterate over the setup until
            // we find the first non-empty and non-unit clause and can then
            // stop.
            break;
        }
    }
}

setup_t setup_propagate_units(const setup_t *setup, const litset_t *split)
{
    litset_t units = litset_lazy_copy(split);
    add_unit_clauses_from_setup(&units, setup);
    setup_t s = setup_lazy_copy(setup);
    // XXX I think we don't have to add the split literals to the setup.
    // Otherwise we have to add all elements from split to &s.
    bool new_units;
    do {
        new_units = false;
        clause_t const **new_cs = malloc(setup_size(&s) * sizeof(clause_t *));
        int *old_cs = malloc(setup_size(&s) * sizeof(int));
        int n = 0;
        for (int i = 0; i < setup_size(&s); ++i) {
            const clause_t *c = setup_get(&s, i);
            const clause_t *d = clause_resolve(c, &units);
            if (!clause_eq(c, d)) {
                new_cs[n] = d;
                old_cs[n] = i;
                ++n;
                if (clause_is_unit(d)) {
                    new_units |= litset_add(&units, clause_unit(d));
                }
            }
        }
        setup_remove_all_indices(&s, old_cs, n);
        for (int i = 0; i < n; ++i) {
            const clause_t *d = new_cs[i];
            const bool added = setup_add(&s, d);
            if (!added) {
                free((clause_t *) d);
            }
        }
        free(old_cs);
        free(new_cs);
    } while (new_units);
    return s;
}

bool query_v(const setup_t *setup, litset_t *split,
        const pelset_t *pel, const query_t *phi, const stdvec_t *z, int k)
{
    if (k > 0) {
        bool tried = false;
        for (int i = 0; i < pelset_size(pel); ++i) {
            const literal_t l1 = *pelset_get(pel, i);
            const literal_t l2 = literal_flip(&l1);
            if (litset_contains(split, &l1) || litset_contains(split, &l2)) {
                continue;
            }
            tried = true;
            litset_add(split, &l1);
            const bool r1 = query_v(setup, split, pel, phi, z, k - 1);
            litset_remove(split, &l1);
            if (!r1) {
                continue;
            }
            litset_add(split, &l2);
            const bool r2 = query_v(setup, split, pel, phi, z, k - 1);
            litset_remove(split, &l2);
            if (r1 && r2) {
                return true;
            }
        }
        if (!tried) {
            return query_v(setup, split, pel, phi, z, 0);
        }
        return false;
    } else {
        return false;
    }
}

