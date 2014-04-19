// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "setup.h"
#include "memory.h"
#include <assert.h>

#define MAX(x,y)    ((x) > (y) ? (x) : (y))
#define SWAP(x,y)   ({ typeof(x) tmp = x; x = y; y = tmp; })

SET_IMPL(clause, literal_t *, literal_cmp);
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

struct univ_clause {
    const ewff_t *cond;
    const clause_t *clause;
    stdset_t names;
    varset_t vars;
};

union box_univ_clause { univ_clause_t c; };

static int univ_clause_cmp(const univ_clause_t *c1, const univ_clause_t *c2)
{
    return clause_cmp(c1->clause, c2->clause);
}

static int box_univ_clause_cmp(const box_univ_clause_t *c1,
        const box_univ_clause_t *c2)
{
    return clause_cmp(c1->c.clause, c2->c.clause);
}

SET_IMPL(univ_clauses, univ_clause_t *, univ_clause_cmp);
SET_IMPL(box_univ_clauses, box_univ_clause_t *, box_univ_clause_cmp);

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
            assert(false);
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
            assert(false);
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
            assert(false);
            return false;
    }
}

static clause_t clause_substitute(const clause_t *c, const varmap_t *map)
{
    clause_t cc = clause_init_with_size(clause_size(c));
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        literal_t *ll = MALLOC(sizeof(literal_t));
        *ll = literal_substitute(l, map);
        clause_add(&cc, ll);
    }
    return cc;
}

static varset_t clause_vars(const clause_t *c)
{
    varset_t vars = varset_init();
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        literal_collect_vars(l, &vars);
    }
    return vars;
}

static stdset_t clause_names(const clause_t *c)
{
    stdset_t names = stdset_init();
    for (int i = 0; i < clause_size(c); ++i) {
        const literal_t *l = clause_get(c, i);
        literal_collect_names(l, &names);
    }
    return names;
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
        .cond       = cond,
        .clause     = clause,
        .names      = names,
        .vars       = vars
    };
    return c;
}

const box_univ_clause_t *box_univ_clause_init(
        const ewff_t *cond,
        const clause_t *clause)
{
    return (box_univ_clause_t *) univ_clause_init(cond, clause);
}

static void ground_univ(setup_t *setup, varmap_t *varmap,
        const univ_clause_t *univ_clause, const stdset_t *ns, int i)
{
    if (i < varset_size(&univ_clause->vars)) {
        const var_t var = varset_get(&univ_clause->vars, i);
        for (int j = 0; j < stdset_size(ns); ++j) {
            const stdname_t n = stdset_get(ns, j);
            varmap_add_replace(varmap, var, n);
            ground_univ(setup, varmap, univ_clause, ns, i + 1);
        }
    } else {
        if (ewff_eval(univ_clause->cond, varmap)) {
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
        assert(!ELEM_WAS_IN_SET(index));
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

#define EMPTY_CLAUSE_INDEX 0

static bool setup_contains_empty_clause(const setup_t *s)
{
    return setup_size(s) > EMPTY_CLAUSE_INDEX &&
        clause_is_empty(setup_get(s, EMPTY_CLAUSE_INDEX));
}

static int setup_minimize_wrt(setup_t *setup, int index, int ref_index)
{
    // Removes all clauses subsumed by the index-th clause.
    // Returns the number of removed clauses whose index is <= ref_index.
    // A special case is the empty clause: since that's it's easy to detect, we
    // drop all other clauses and return -1.
    if (setup_contains_empty_clause(setup)) {
        setup_remove_index_range(setup, EMPTY_CLAUSE_INDEX + 1,
                setup_size(setup));
        return -1;
    }
    int drops = 0;
    const clause_t *c = setup_get(setup, index);
    for (int i = index + 1; i < setup_size(setup); ++i) {
        const clause_t *d = setup_get(setup, i);
        if (clause_size(d) > clause_size(c) && clause_contains_all(d, c)) {
            setup_remove_index(setup, i--);
            if (i < ref_index) {
                ++drops;
            }
        }
    }
    return drops;
}

void setup_minimize(setup_t *setup)
{
    for (int i = 0; i < setup_size(setup); ++i) {
        const int k = setup_minimize_wrt(setup, i, i);
        if (k == -1) {
            return;
        }
        i -= k;
    }
}

void setup_propagate_units(setup_t *setup)
{
    if (setup_contains_empty_clause(setup)) {
        return;
    }
    splitset_t units = setup_get_unit_clauses(setup);
    splitset_t new_units = splitset_init();
    splitset_t *units_ptr = &units;
    splitset_t *new_units_ptr = &new_units;
    do {
        splitset_clear(new_units_ptr);
        for (int i = 0; i < setup_size(setup); ++i) {
            const clause_t *c = setup_get(setup, i);
            const clause_t *d = clause_resolve(c, units_ptr);
            if (!clause_eq(c, d)) {
                const int j = setup_replace_index(setup, i, d);
                if (ELEM_WAS_IN_SET(j) || i < REAL_SET_INDEX(j)) {
                    --i;
                }
                if (!ELEM_WAS_IN_SET(j)) {
                    const int k = setup_minimize_wrt(setup,
                            REAL_SET_INDEX(j), i);
                    if (k == -1) {
                        return;
                    }
                    i -= k;
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
    } while (splitset_size(units_ptr) > 0);
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

