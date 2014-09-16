// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Note that setup_get_unit_clauses() and EMPTY_CLAUSE_INDEX rely on the
 * ordering structure of the sets, particularly of setups: the first criterion
 * is the length of clauses. That is, the empty clause is always on the first
 * position, and followed by all unit clauses.
 *
 * setup_minimize_wrt() removes all clauses subsumed by the given one.
 * This of course makes setup_propagate_units() significantly more expensive,
 * but it also keeps the setups smaller. For the examples in the unit tests, the
 * extra work of setup_minimize_wrt() already pays off.
 */
#include "setup.h"
#include "memory.h"
#include <assert.h>
#include <stdlib.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define SWAP(x,y)   ({ typeof(x) tmp = x; x = y; y = tmp; })

#define EMPTY_CLAUSE_INDEX 0

SET_IMPL(clause, literal_t *, literal_cmp);
SET_IMPL(splitset, literal_t *, literal_cmp);
SET_IMPL(pelset, literal_t *, literal_cmp);
SET_IMPL(setup, clause_t *, clause_cmp);

enum ewff_type { EWFF_EQ, EWFF_SORT, EWFF_NEG, EWFF_OR };
struct ewff {
    enum ewff_type type;
    union {
        struct { term_t t1; term_t t2; } eq;
        struct { term_t t; bool (*is_sort)(stdname_t n); } sort;
        struct { const ewff_t *e; } neg;
        struct { const ewff_t *e1; const ewff_t *e2; } or;
    } u;
};

VECTOR_IMPL(univ_clauses, univ_clause_t *, NULL);
VECTOR_IMPL(box_univ_clauses, box_univ_clause_t *, NULL);

const ewff_t *ewff_true(void)
{
    return ewff_eq(0, 0);
}

const ewff_t *ewff_eq(term_t t1, term_t t2)
{
    ewff_t *e = MALLOC(sizeof(ewff_t));
    e->type = EWFF_EQ;
    e->u.eq.t1 = t1;
    e->u.eq.t2 = t2;
    return e;
}

const ewff_t *ewff_neq(term_t t1, term_t t2)
{
    return ewff_neg(ewff_eq(t1, t2));
}

const ewff_t *ewff_sort(term_t t, bool (*is_sort)(stdname_t n))
{
    ewff_t *e = MALLOC(sizeof(ewff_t));
    e->type = EWFF_SORT;
    e->u.sort.t = t;
    e->u.sort.is_sort = is_sort;
    return e;
}

const ewff_t *ewff_neg(const ewff_t *e1)
{
    ewff_t *e = MALLOC(sizeof(ewff_t));
    e->type = EWFF_NEG;
    e->u.neg.e = e1;
    return e;
}

const ewff_t *ewff_or(const ewff_t *e1, const ewff_t *e2)
{
    ewff_t *e = MALLOC(sizeof(ewff_t));
    e->type = EWFF_OR;
    e->u.or.e1 = e1;
    e->u.or.e2 = e2;
    return e;
}

const ewff_t *ewff_and(const ewff_t *e1, const ewff_t *e2)
{
    return ewff_neg(ewff_or(ewff_neg(e1), ewff_neg(e2)));
}

static void ewff_collect_vars(const ewff_t *e, varset_t *vars)
{
    switch (e->type) {
        case EWFF_EQ:
            if (IS_VARIABLE(e->u.eq.t1)) {
                varset_add(vars, e->u.eq.t1);
            }
            if (IS_VARIABLE(e->u.eq.t2)) {
                varset_add(vars, e->u.eq.t2);
            }
            break;
        case EWFF_SORT:
            if (IS_VARIABLE(e->u.sort.t)) {
                varset_add(vars, e->u.sort.t);
            }
            break;
        case EWFF_NEG:
            ewff_collect_vars(e->u.neg.e, vars);
            break;
        case EWFF_OR:
            ewff_collect_vars(e->u.or.e1, vars);
            ewff_collect_vars(e->u.or.e2, vars);
            break;
        default:
            abort();
    }
}

static void ewff_collect_names(const ewff_t *e, stdset_t *names)
{
    switch (e->type) {
        case EWFF_EQ:
            if (IS_STDNAME(e->u.eq.t1)) {
                stdset_add(names, e->u.eq.t1);
            }
            if (IS_STDNAME(e->u.eq.t2)) {
                stdset_add(names, e->u.eq.t2);
            }
            break;
        case EWFF_SORT:
            if (IS_STDNAME(e->u.sort.t)) {
                stdset_add(names, e->u.sort.t);
            }
            break;
        case EWFF_NEG:
            ewff_collect_names(e->u.neg.e, names);
            break;
        case EWFF_OR:
            ewff_collect_names(e->u.or.e1, names);
            ewff_collect_names(e->u.or.e2, names);
            break;
        default:
            abort();
    }
}

bool ewff_eval(const ewff_t *e, const varmap_t *varmap)
{
    switch (e->type) {
        case EWFF_EQ: {
            const term_t t1 = e->u.eq.t1;
            const term_t t2 = e->u.eq.t2;
            const stdname_t n1 =
                IS_VARIABLE(t1) ? varmap_lookup(varmap, t1) : t1;
            const stdname_t n2 =
                IS_VARIABLE(t2) ? varmap_lookup(varmap, t2) : t2;
            assert(IS_STDNAME(n1));
            assert(IS_STDNAME(n2));
            return n1 == n2;
        }
        case EWFF_SORT: {
            const term_t t = e->u.sort.t;
            const stdname_t n = IS_VARIABLE(t) ? varmap_lookup(varmap, t) : t;
            assert(IS_STDNAME(n));
            return e->u.sort.is_sort(n);
        }
        case EWFF_NEG:
            return !ewff_eval(e->u.neg.e, varmap);
        case EWFF_OR:
            return ewff_eval(e->u.or.e1, varmap) ||
                ewff_eval(e->u.or.e2, varmap);
        default:
            abort();
    }
}

static void ewff_ground_h(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *),
        varmap_t *varmap)
{
    if (varmap_size(varmap) < varset_size(vars)) {
        const var_t x = varset_get(vars, varmap_size(varmap));
        for (EACH_CONST(stdset, hplus, i)) {
            const stdname_t n = i.val;
            varmap_add_replace(varmap, x, n);
            ewff_ground_h(e, vars, hplus, ground, varmap);
        }
    } else {
        if (ewff_eval(e, varmap)) {
            ground(varmap);
        }
    }
}

void ewff_ground(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *))
{
    varmap_t varmap = varmap_init_with_size(varset_size(vars));
    ewff_ground_h(e, vars, hplus, ground, &varmap);
}

clause_t clause_substitute(const clause_t *c, const varmap_t *map)
{
    clause_t cc = clause_init_with_size(clause_size(c));
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        literal_t *ll = MALLOC(sizeof(literal_t));
        *ll = literal_substitute(l, map);
        clause_add(&cc, ll);
    }
    return cc;
}

static varset_t clause_vars(const clause_t *c)
{
    varset_t vars = varset_init();
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        literal_collect_vars(l, &vars);
    }
    return vars;
}

static stdset_t clause_names(const clause_t *c)
{
    stdset_t names = stdset_init();
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        literal_collect_names(l, &names);
    }
    return names;
}

const clause_t *clause_empty(void)
{
    static clause_t c;
    static clause_t *c_ptr = NULL;
    if (c_ptr == NULL) {
        c = clause_init_with_size(0);
        c_ptr = &c;
    }
    return c_ptr;
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
    for (EACH(clause, d, i)) {
        const literal_t l = literal_flip(i.val);
        if (splitset_contains(u, &l)) {
            clause_iter_remove(&i);
        }
    }
#if 0
    // In our examples the clauses are so small that looping over the clause is
    // faster than over the splitset.
    if (clause_size(c) <= splitset_size(u)) {
        // above loop
    } else {
        int *indices = MALLOC(clause_size(c) * sizeof(int));
        int n_indices = 0;
        for (EACH_CONST(splitset, u, i)) {
            const literal_t l = literal_flip(i.val);
            const int j = clause_find(d, &l);
            if (j != -1) {
                indices[n_indices++] = j;
            }
        }
        clause_remove_all_indices(d, indices, n_indices);
        FREE(indices);
    }
#endif
    return d;
}

const univ_clause_t *univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause)
{
    univ_clause_t *c = MALLOC(sizeof(univ_clause_t));
    varset_t vars = clause_vars(clause);
    ewff_collect_vars(cond, &vars);
    stdset_t names = clause_names(clause);
    ewff_collect_names(cond, &names);
    *c = (univ_clause_t) {
        .cond   = cond,
        .clause = clause,
        .names  = names,
        .vars   = vars
    };
    return c;
}

const box_univ_clause_t *box_univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause)
{
    return (box_univ_clause_t *) univ_clause_init(cond, clause);
}

static void ground_univ(
        setup_t *setup,
        const univ_clause_t *univ_clause,
        const stdset_t *hplus)
{
    void ground_clause(const varmap_t *varmap) {
        const clause_t *c;
        if (varset_size(&univ_clause->vars) > 0) {
            clause_t *d = MALLOC(sizeof(clause_t));
            *d = clause_substitute(univ_clause->clause, varmap);
            c = d;
        } else {
            c = univ_clause->clause;
        }
        setup_add(setup, c);
    }
    ewff_ground(univ_clause->cond, &univ_clause->vars, hplus,
            &ground_clause);
}

static void ground_box(setup_t *setup, const stdvec_t *z, const clause_t *c)
{
    clause_t *d = MALLOC(sizeof(clause_t));
    *d = clause_init_with_size(clause_size(c));
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        const stdvec_t zz = stdvec_concat(z, literal_z(l));
        literal_t *ll = MALLOC(sizeof(literal_t));
        *ll = literal_init(&zz, literal_sign(l), literal_pred(l),
                literal_args(l));
        const int index = clause_add(d, ll);
        assert(index >= 0);
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
    for (EACH_CONST(box_univ_clauses, dynamic_bat, i)) {
        const box_univ_clause_t *c = i.val;
        stdset_add_all(&names, &c->c.names);
        max_vars = MAX(max_vars, varset_size(&c->c.vars));
    }
    for (EACH_CONST(univ_clauses, static_bat, i)) {
        const univ_clause_t *c = i.val;
        stdset_add_all(&names, &c->names);
        max_vars = MAX(max_vars, varset_size(&c->vars));
    }
    stdname_t max_name = 0;
    for (EACH_CONST(stdset, &names, i)) {
        max_name = MAX(max_name, i.val);
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
    for (EACH_CONST(univ_clauses, static_bat, i)) {
        const univ_clause_t *c = i.val;
        ground_univ(&setup, c, hplus);
    }
    return setup;
}

setup_t setup_init_dynamic(
        const box_univ_clauses_t *dynamic_bat,
        const stdset_t *hplus,
        const stdvecset_t *query_zs)
{
    setup_t box_clauses = setup_init();
    // First ground the universal quantifiers.
    // This is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i].vars) and
    // standard names (elements of hplus) for all i.
    for (EACH_CONST(box_univ_clauses, dynamic_bat, i)) {
        const box_univ_clause_t *c = i.val;
        ground_univ(&box_clauses, &c->c, hplus);
    }
    // Ground each box by substituting it with the prefixes of all action
    // sequences.
    setup_t setup = setup_init();
    for (EACH_CONST(setup, &box_clauses, i)) {
        const clause_t *c = i.val;
        for (EACH_CONST(stdvecset, query_zs, j)) {
            const stdvec_t *z = j.val;
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
    if (setup_is_inconsistent(setup)) {
        return;
    }
    for (EACH_CONST(splitset, sensing_results, i)) {
        const literal_t *l = i.val;
        clause_t *c = MALLOC(sizeof(clause_t));
        *c = clause_singleton(l);
        setup_add(setup, c);
    }
}

static void clause_collect_pel(
        const clause_t *c,
        const setup_t *setup,
        pelset_t *pel)
{
    if (clause_is_unit(c)) {
        return;
    }
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
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

static void clause_collect_pel_with_sf(
        const clause_t *c,
        const setup_t *setup,
        pelset_t *pel)
{
    // We split SF literals from imaginarily executed actions only when needed
    // in setup_with_splits_and_sf_subsumes(). For that, the PEL needs to
    // contain the relevant SF atoms, though.
    clause_collect_pel(c, setup, pel);
    stdvecset_t z_done = stdvecset_init();
    for (EACH_CONST(clause, c, i)) {
        const stdvec_t *z = literal_z(i.val);
        if (!stdvecset_contains(&z_done, z)) {
            for (int j = 0; j < stdvec_size(z) - 1; ++j) {
                const stdvec_t z_prefix = stdvec_lazy_copy_range(z, 0, j);
                const stdname_t n = stdvec_get(z, j);
                const stdvec_t n_vec = stdvec_singleton(n);
                literal_t *sf = MALLOC(sizeof(literal_t));
                *sf = literal_init(&z_prefix, true, SF, &n_vec);
                if (!setup_would_be_needless_split(setup, sf)) {
                    pelset_add(pel, sf);
                }
            }
        }
        stdvecset_add(&z_done, z);
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
    for (EACH_CONST(setup, setup, i)) {
        clause_collect_pel(i.val, setup, &pel);
    }
    return pel;
}

static splitset_t setup_get_unit_clauses(const setup_t *setup)
{
    splitset_t ls = splitset_init();
    for (EACH_CONST(setup, setup, i)) {
        const clause_t *c = i.val;
        if (clause_is_empty(c)) {
            continue;
        } else if (clause_is_unit(c)) {
            splitset_add(&ls, clause_unit(c));
        } else {
            // XXX Clauses with higher indices have >1 literal.
            break;
        }
    }
    return ls;
}

static void setup_minimize_wrt(setup_t *setup, const clause_t *c,
        setup_iter_t *iter)
{
    if (setup_is_inconsistent(setup)) {
        setup_clear(setup);
        setup_add(setup, clause_empty());
        assert(setup_is_inconsistent(setup) && setup_size(setup) == 1);
        return;
    }
    setup_iter_t i = setup_iter_from(setup, c);
    setup_iter_add_auditor(&i, iter);
    while (setup_iter_next(&i)) {
        const clause_t *d = i.val;
        if (clause_size(d) > clause_size(c) && clause_contains_all(d, c)) {
            setup_iter_remove(&i);
        }
    }
}

void setup_minimize(setup_t *setup)
{
    for (EACH(setup, setup, i)) {
        setup_minimize_wrt(setup, i.val, &i);
        if (setup_is_inconsistent(setup)) {
            return;
        }
    }
}

void setup_propagate_units(setup_t *setup)
{
    if (setup_is_inconsistent(setup)) {
        return;
    }
    splitset_t units = setup_get_unit_clauses(setup);
    splitset_t new_units = splitset_init();
    splitset_t *units_ptr = &units;
    splitset_t *new_units_ptr = &new_units;
    while (splitset_size(units_ptr) > 0) {
        splitset_clear(new_units_ptr);
        for (EACH(setup, setup, i)) {
            const clause_t *c = i.val;
            const clause_t *d = clause_resolve(c, units_ptr);
            if (!clause_eq(c, d)) {
                const int j = setup_iter_replace(&i, d);
                if (j >= 0) {
                    setup_minimize_wrt(setup, d, &i);
                    if (setup_is_inconsistent(setup)) {
                        return;
                    }
                    if (clause_is_unit(d)) {
                        const literal_t *l = clause_unit(d);
                        if (!splitset_contains(units_ptr, l)) {
                            splitset_add(new_units_ptr, l);
                        }
                    }
                }
            }
        }
        SWAP(units_ptr, new_units_ptr);
    }
}

bool setup_is_inconsistent(const setup_t *s)
{
    return setup_size(s) > EMPTY_CLAUSE_INDEX &&
        clause_is_empty(setup_get(s, EMPTY_CLAUSE_INDEX));
}

bool setup_subsumes(setup_t *setup, const clause_t *c)
{
    setup_propagate_units(setup);
    for (EACH_CONST(setup, setup, i)) {
        if (clause_contains_all(c, i.val)) {
            return true;
        }
    }
    return false;
}

bool setup_with_splits_subsumes(
        setup_t *setup,
        pelset_t *pel,
        const clause_t *c,
        const int k)
{
    bool r = setup_subsumes(setup, c);
    if (r || k < 0) {
        return r;
    }
    for (EACH(pelset, pel, i)) {
        const literal_t *l1 = i.val;
        if ((literal_pred(l1) == SF) != (k == 0)) {
            continue;
        }
        const literal_t l2 = literal_flip(l1);
        pelset_iter_remove(&i);
        const clause_t c1 = clause_singleton(l1);
        const clause_t c2 = clause_singleton(&l2);
        const int k1 = (literal_pred(l1) == SF) ? k : k - 1;
        setup_t setup1 = setup_lazy_copy(setup);
        setup_add(&setup1, &c1);
        bool r;
        r = setup_with_splits_subsumes(&setup1, pel, c, k1);
        if (!r) {
            continue;
        }
        setup_t setup2 = setup_lazy_copy(setup);
        setup_add(&setup2, &c2);
        r = setup_with_splits_subsumes(&setup2, pel, c, k1);
        if (r) {
            return true;
        }
    }
    return false;
}

bool setup_with_splits_and_sf_subsumes(
        setup_t *setup,
        const pelset_t *pel,
        const clause_t *c,
        const int k)
{
    if (setup_is_inconsistent(setup)) {
        return true;
    }
    //setup_t setup_copy = setup_lazy_copy(setup);
    pelset_t pel_copy = pelset_lazy_copy(pel);
    clause_collect_pel_with_sf(c, setup, &pel_copy);
    return setup_with_splits_subsumes(setup, &pel_copy, c, k);
}

