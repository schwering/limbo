// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "setup.h"
#include "memory.h"
#include <assert.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define SWAP(x,y)   ({ typeof(x) tmp = x; x = y; y = tmp; })

SET_IMPL(clause, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);
VECTOR_IMPL(univ_clauses, univ_clause_t *, NULL);
VECTOR_IMPL(box_univ_clauses, box_univ_clause_t *, NULL);

const clause_t *clause_empty(void)
{
    static clause_t c;
    static clause_t *ptr = NULL;
    if (ptr == NULL) {
        c = clause_init_with_size(0);
        ptr = &c;
    }
    return ptr;
}

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

static const clause_t *clause_resolve(const clause_t *c, const splitset_t *u)
{
    clause_t *d = MALLOC(sizeof(clause_t));
    *d = clause_lazy_copy(c);
    int *indices = MALLOC(clause_size(c) * sizeof(int));
    int n_indices = 0;
    for (int i = 0; i < splitset_size(u); ++i) {
        const literal_t l = literal_flip(splitset_get(u, i));
        const int j = clause_find(d, &l);
        if (j != -1) {
            indices[n_indices++] = j;
        }
    }
    clause_remove_all_indices(d, indices, n_indices);
    FREE(indices);
    return d;
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
    clause_t *d = MALLOC(sizeof(clause_t));
    *d = clause_init_with_size(clause_size(c));
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        const stdvec_t zz = stdvec_concat(z, literal_z(l));
        literal_t *ll = MALLOC(sizeof(literal_t));
        *ll = literal_init(&zz, literal_sign(l), literal_pred(l),
                literal_args(l));
        const int index = clause_add(d, ll);
        assert(index != -1);
    }
    setup_add(setup, d);
}

stdset_t bat_hplus(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *query_names,
        int n_query_vars)
{
    stdset_t names = stdset_copy(query_names);
    int max_vars = n_query_vars;
    for (int i = 0; i < box_univ_clauses_size(dynamic_bat); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(dynamic_bat, i);
        stdset_add_all(&names, &c->c.names);
        max_vars = MAX(max_vars, varset_size(&c->c.vars));
    }
    for (int i = 0; i < univ_clauses_size(static_bat); ++i) {
        const univ_clause_t *c = univ_clauses_get(static_bat, i);
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

setup_t setup_init_static(
        const univ_clauses_t *static_bat,
        const stdset_t *hplus)
{
    setup_t setup = setup_init();
    for (int i = 0; i < univ_clauses_size(static_bat); ++i) {
        const univ_clause_t *c = univ_clauses_get(static_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->vars));
        ground_univ(&setup, &varmap, c, hplus, 0);
    }
    return setup;
}

setup_t setup_init_dynamic(
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs)
{
    setup_t box_clauses = setup_init();
    // first ground the universal quantifiers
    // this is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i].vars) and
    // standard names (elements of hplus) for all i
    for (int i = 0; i < box_univ_clauses_size(dynamic_bat); ++i) {
        const box_univ_clause_t *c = box_univ_clauses_get(dynamic_bat, i);
        varmap_t varmap = varmap_init_with_size(varset_size(&c->c.vars));
        ground_univ(&box_clauses, &varmap, &c->c, hplus, 0);
    }
    // ground each box by substituting it with the prefixes of all action
    // sequences
    setup_t setup = setup_init();
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

setup_t setup_init_static_and_dynamic(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs)
{
    setup_t static_setup = setup_init_static(static_bat, hplus);
    setup_t dynamic_setup = setup_init_dynamic(dynamic_bat, hplus, query_zs);
    setup_t setup = setup_union(&static_setup, &dynamic_setup);
    return setup;
}

void setup_add_sensing_results(
        setup_t *setup,
        const splitset_t *sensing_results)
{
    for (int i = 0; i < splitset_size(sensing_results); ++i) {
        const literal_t *l = splitset_get(sensing_results, i);
        clause_t *c = MALLOC(sizeof(clause_t));
        *c = clause_singleton(l);
        setup_add(setup, c);
    }
}

void add_pel_of_clause(pelset_t *pel, const clause_t *c, const setup_t *setup)
{
    if (clause_is_unit(c)) {
        return;
    }
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        // XXX TODO is that still sound?
        // We don't add SF literals to PEL to keep the semantics equivalent to
        // the ESL paper: We want to treat SF literals which come from reasoning
        // about actions (rule 11 and 12 in V in the KR-2014 paper) same like
        // other split literals, as this simplifies the code. This however also
        // affects the k parameter. Our solution is to let the splitting run
        // until k = 0 (inclusive) and split non-SF literals only for k > 0 and
        // split SF literals only for k = 0. Now if we would add SF literals
        // from the sense axioms to PEL, these could always be split even for
        // k = 0 and therefore we could get (sound) positive results which the
        // formal semantics would not find, because it wouldn't split this SF
        // literal.
        if (literal_pred(l) == SF) {
            continue;
        }
        if (setup_would_be_needless_split(setup, l)) {
            continue;
        }
        if (!literal_sign(l)) {
            literal_t *ll = MALLOC(sizeof(literal_t));
            *ll = literal_flip(l);
            pelset_add(pel, ll);
        } else {
            pelset_add(pel, l);
        }
    }
}

bool setup_would_be_needless_split(const setup_t *setup, const literal_t *l)
{
    const clause_t c = clause_singleton(l);
    if (setup_contains(setup, &c)) {
        return true;
    }
    const literal_t ll = literal_flip(l);
    const clause_t d = clause_singleton(&ll);
    if (setup_contains(setup, &d)) {
        return true;
    }
    return false;
}

pelset_t setup_pel(const setup_t *setup)
{
    pelset_t pel = pelset_init();
    for (int i = 0; i < setup_size(setup); ++i) {
        add_pel_of_clause(&pel, setup_get(setup, i), setup);
    }
    return pel;
}

static splitset_t setup_get_unit_clauses(const setup_t *setup)
{
    splitset_t ls = splitset_init();
    for (int i = 0; i < setup_size(setup); ++i) {
        const clause_t *c = setup_get(setup, i);
        if (clause_is_empty(c)) {
            continue;
        } else if (clause_is_unit(c)) {
            splitset_add(&ls, clause_unit(c));
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
    return ls;
}

static bool setup_contains_empty_clause(const setup_t *s)
{
    return setup_size(s) > 0 && clause_size(setup_get(s, 0)) == 0;
}

void setup_propagate_units(setup_t *setup)
{
    if (setup_contains_empty_clause(setup)) {
        return;
    }
    splitset_t units = setup_get_unit_clauses(setup);
    splitset_t new_units = splitset_init();
    // we use pointers for faster swapping
    splitset_t *units_ptr = &units;
    splitset_t *new_units_ptr = &new_units;
    do {
        splitset_clear(new_units_ptr);
        for (int i = 0; i < setup_size(setup); ++i) {
            const clause_t *c = setup_get(setup, i);
            const clause_t *d = clause_resolve(c, units_ptr);
            if (!clause_eq(c, d)) {
                setup_replace_index(setup, i, d);
                if (clause_is_unit(d)) {
                    const literal_t *l = clause_unit(d);
                    if (!splitset_contains(units_ptr, l)) {
                        splitset_add(new_units_ptr, l);
                    }
                }
            }
        }
        SWAP(units_ptr, new_units_ptr);
    } while (splitset_size(units_ptr) > 0 &&
            !setup_contains_empty_clause(setup));
}

bool setup_subsumes(setup_t *setup, const clause_t *c)
{
    setup_propagate_units(setup);
    for (int i = 0; i < setup_size(setup); ++i) {
        if (clause_contains_all(c, setup_get(setup, i))) {
            return true;
        }
    }
    return false;
}

