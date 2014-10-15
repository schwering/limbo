// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Note that setup_unit_clauses() and EMPTY_CLAUSE_INDEX rely on the
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

SET_DECL(splitset, literal_t *);
SET_DECL(pelset, literal_t *);

SET_IMPL(clause, literal_t *, literal_cmp);
SET_IMPL(splitset, literal_t *, literal_cmp);
SET_IMPL(pelset, literal_t *, literal_cmp);
SET_IMPL(clauses, clause_t *, clause_cmp);

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
    return ewff_eq(-1, -1);
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
        varmap_remove(varmap, x);
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

static const clause_t *clause_empty(void)
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

static pelset_t clause_pel(const clause_t *c)
{
    pelset_t pel = pelset_init_with_size(clause_size(c));
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        if (literal_pred(l) == SF) {
            continue;
        }
        if (!literal_sign(l)) {
            literal_t *ll = MALLOC(sizeof(literal_t));
            *ll = literal_flip(l);
            pelset_add(&pel, ll);
        } else {
            pelset_add(&pel, l);
        }
    }
    return pel;
}

static pelset_t clause_sf(const clause_t *c)
{
    // Build SF literals that correspond to actions executed within epistemic
    // operator. They will be split in setup_with_splits_and_sf_subsumes().
    pelset_t pel = pelset_init();
    stdvecset_t z_done = stdvecset_init();
    for (EACH_CONST(clause, c, i)) {
        const literal_t *l = i.val;
        const stdvec_t *z = literal_z(l);
        if (!stdvecset_contains(&z_done, z)) {
            for (int j = 0; j < stdvec_size(z) - 1; ++j) {
                const stdvec_t z_prefix = stdvec_lazy_copy_range(z, 0, j);
                const stdname_t n = stdvec_get(z, j);
                const stdvec_t n_vec = stdvec_singleton(n);
                literal_t *sf = MALLOC(sizeof(literal_t));
                *sf = literal_init(&z_prefix, true, SF, &n_vec);
            }
        }
        stdvecset_add(&z_done, z);
    }
    return pel;
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
        clauses_add(&setup->clauses, c);
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
    clauses_add(&setup->clauses, d);
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
    setup_t setup = (setup_t) {
        .clauses = clauses_init(),
        .incons  = bitmap_init()
    };
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
    setup_t box_clauses = (setup_t) {
        .clauses = clauses_init(),
        .incons  = bitmap_init()
    };
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
    setup_t setup = (setup_t) {
        .clauses = clauses_init(),
        .incons  = bitmap_init()
    };
    for (EACH_CONST(clauses, &box_clauses.clauses, i)) {
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

setup_t setup_union(const setup_t *setup1, const setup_t *setup2)
{
    return (setup_t) {
        .clauses = clauses_union(&setup1->clauses, &setup2->clauses),
        .incons  = bitmap_or(&setup1->incons, &setup2->incons)
    };
}

setup_t setup_copy(const setup_t *setup)
{
    return (setup_t) {
        .clauses = clauses_copy(&setup->clauses),
        .incons  = bitmap_copy(&setup->incons)
    };
}

setup_t setup_lazy_copy(const setup_t *setup)
{
    return (setup_t) {
        .clauses = clauses_lazy_copy(&setup->clauses),
        .incons  = bitmap_lazy_copy(&setup->incons)
    };
}

int setup_cmp(const setup_t *setup1, const setup_t *setup2)
{
    return clauses_cmp(&setup1->clauses, &setup2->clauses);
}

static bool setup_contains_empty_clause(const setup_t *setup)
{
    return clauses_size(&setup->clauses) > EMPTY_CLAUSE_INDEX &&
        clause_is_empty(clauses_get(&setup->clauses, EMPTY_CLAUSE_INDEX));
}

void setup_add_sensing_result(
        setup_t *setup,
        const stdvec_t *z,
        const stdname_t n,
        const bool r)
{
    if (setup_contains_empty_clause(setup)) {
        return;
    }

    const stdvec_t args = stdvec_singleton(n);
    literal_t *l = MALLOC(sizeof(literal_t));
    *l = literal_init(z, r, SF, &args);
    clause_t *c = MALLOC(sizeof(clause_t));
    *c = clause_singleton(l);
    clauses_add(&setup->clauses, c);

    const literal_t ll = literal_flip(l);
    clause_t d = clause_singleton(&ll);
    for (int k = 0; k < bitmap_size(&setup->incons); ++k) {
        if (!bitmap_get(&setup->incons, k) && setup_entails(setup, &d, k)) {
            bitmap_set(&setup->incons, k, true);
            for (; k < bitmap_size(&setup->incons); ++k) {
                bitmap_set(&setup->incons, k, true);
            }
        }
    }
}

static pelset_t setup_full_pel(const setup_t *setup)
{
    pelset_t pel = pelset_init();
    for (EACH_CONST(clauses, &setup->clauses, i)) {
        const clause_t *d = i.val;
        const pelset_t dp = clause_pel(d);
        pelset_add_all(&pel, &dp);
    }
    return pel;
}

static pelset_t setup_clause_small_pel(const setup_t *setup, const clause_t *c)
{
    pelset_t pel = clause_sf(c);
    const pelset_t cp = clause_pel(c);
    pelset_add_all(&pel, &cp);
    for (int size = 0, new_size; size != (new_size = pelset_size(&pel)); ) {
        size = new_size;
        for (EACH_CONST(clauses, &setup->clauses, i)) {
            const clause_t *d = i.val;
            const pelset_t dp = clause_pel(d);
            if (!pelset_disjoint(&pel, &dp)) {
                pelset_add_all(&pel, &dp);
            }
        }
    }
    return pel;
}

static bool setup_relevant_split(
        const setup_t *setup,
        const clause_t *c,
        const literal_t *l,
        const int k)
{
    const clause_t lc = clause_singleton(l);
    if (clauses_contains(&setup->clauses, &lc)) {
        return false;
    }
    const literal_t ll = literal_flip(l);
    const clause_t llc = clause_singleton(&ll);
    if (clauses_contains(&setup->clauses, &llc)) {
        return false;
    }
    if (clause_contains(c, l) || clause_contains(c, &ll)) {
        return true;
    }
    const pelset_t cp = clause_pel(c);
    for (EACH_CONST(clauses, &setup->clauses, i)) {
        const clause_t *d = i.val;
        if ((!clause_contains(d, l) && !clause_contains(d, &ll)) ||
                clause_size(d) <= 1) {
            continue;
        }
        if (clause_size(c) <= k+1) {
            return true;
        }
        const pelset_t dp = clause_pel(d);
        const pelset_t diff = pelset_difference(&dp, &cp);
        if (pelset_size(&diff) <= k) {
            return true;
        }
    }
    return false;
}

static splitset_t setup_unit_clauses(const setup_t *setup)
{
    splitset_t ls = splitset_init();
    for (EACH_CONST(clauses, &setup->clauses, i)) {
        const clause_t *c = i.val;
        if (clause_is_empty(c)) {
            continue;
        } else if (clause_is_unit(c)) {
            splitset_add(&ls, clause_unit(c));
        } else {
            // Clauses with higher indices have >1 literal.
            break;
        }
    }
    return ls;
}

static void setup_minimize_wrt(setup_t *setup, const clause_t *c,
        clauses_iter_t *iter)
{
    if (setup_contains_empty_clause(setup)) {
        clauses_clear(&setup->clauses);
        bitmap_clear(&setup->incons);
        clauses_add(&setup->clauses, clause_empty());
        bitmap_insert(&setup->incons, 0, true);
        assert(setup_contains_empty_clause(setup));
        assert(clauses_size(&setup->clauses) == 1);
        assert(bitmap_size(&setup->incons) == 1);
        assert(bitmap_get(&setup->incons, 0));
        return;
    }
    clauses_iter_t i = clauses_iter_from(&setup->clauses, c);
    clauses_iter_add_auditor(&i, iter);
    while (clauses_iter_next(&i)) {
        const clause_t *d = i.val;
        if (clause_size(d) > clause_size(c) && clause_contains_all(d, c)) {
            clauses_iter_remove(&i);
        }
    }
}

void setup_minimize(setup_t *setup)
{
    for (EACH(clauses, &setup->clauses, i)) {
        setup_minimize_wrt(setup, i.val, &i);
        if (setup_contains_empty_clause(setup)) {
            return;
        }
    }
}

void setup_propagate_units(setup_t *setup)
{
    if (setup_contains_empty_clause(setup)) {
        return;
    }
    splitset_t units = setup_unit_clauses(setup);
    splitset_t new_units = splitset_init();
    splitset_t *units_ptr = &units;
    splitset_t *new_units_ptr = &new_units;
    while (splitset_size(units_ptr) > 0) {
        splitset_clear(new_units_ptr);
        for (EACH(clauses, &setup->clauses, i)) {
            const clause_t *c = i.val;
            const clause_t *d = clause_resolve(c, units_ptr);
            if (!clause_eq(c, d)) {
                const int j = clauses_iter_replace(&i, d);
                if (j >= 0) {
                    setup_minimize_wrt(setup, d, &i);
                    if (setup_contains_empty_clause(setup)) {
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

bool setup_subsumes(setup_t *setup, const clause_t *c)
{
    setup_propagate_units(setup);
    for (EACH_CONST(clauses, &setup->clauses, i)) {
        if (clause_contains_all(c, i.val)) {
            return true;
        }
    }
    return false;
}

static bool setup_with_splits_subsumes(
        setup_t *setup,
        pelset_t *pel,
        const clause_t *c,
        const int k)
{
    const bool r = setup_subsumes(setup, c);
    if (r || k < 0) {
        return r;
    }
    for (EACH(pelset, pel, i)) {
        const literal_t *l1 = i.val;
        const pred_t p = literal_pred(l1);
        if ((p == SF) != (k == 0) ||
                !setup_relevant_split(setup, c, l1, k)) {
            continue;
        }
        const literal_t l2 = literal_flip(l1);
        pelset_iter_remove(&i);
        const clause_t c1 = clause_singleton(l1);
        const clause_t c2 = clause_singleton(&l2);
        const int k1 = (p == SF) ? k : k - 1;
        setup_t setup1 = setup_lazy_copy(setup);
        clauses_add(&setup1.clauses, &c1);
        if (!setup_with_splits_subsumes(&setup1, pel, c, k1)) {
            continue;
        }
        setup_t setup2 = setup_lazy_copy(setup);
        clauses_add(&setup2.clauses, &c2);
        if (setup_with_splits_subsumes(&setup2, pel, c, k1)) {
            return true;
        }
    }
    return false;
}

bool setup_inconsistent(setup_t *setup, const int k)
{
    if (setup_contains_empty_clause(setup)) {
        return true;
    }
    int l = bitmap_size(&setup->incons);
    if (l > 0) {
        if (k < l) {
            return bitmap_get(&setup->incons, MAX(0, k));
        }
        if (bitmap_get(&setup->incons, l-1)) {
            return true;
        }
    }
    const pelset_t pel = k <= 0 ? pelset_init() : setup_full_pel(setup);
    const clause_t *c = clause_empty();
    for (; l <= k; ++l) {
        pelset_t pel_copy = pelset_lazy_copy(&pel);
        const bool r = setup_with_splits_subsumes(setup, &pel_copy, c, l);
        bitmap_insert(&setup->incons, l, r);
        if (r) {
            for (++l; l <= k; ++l) {
                bitmap_insert(&setup->incons, l, r);
            }
        }
    }
    assert(k < bitmap_size(&setup->incons));
    return bitmap_get(&setup->incons, k);
}

bool setup_entails(setup_t *setup, const clause_t *c, const int k)
{
    if (setup_inconsistent(setup, k)) {
        return true;
    }
    pelset_t pel = k == 0 ? pelset_init() : setup_clause_small_pel(setup, c);
    return setup_with_splits_subsumes(setup, &pel, c, k);
}

