// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "ewff.h"
#include "memory.h"

const ewff_t *ewff_true(void)
{
    static const ewff_t *e = NULL;
    if (e == NULL) {
        e = ewff_eq(FIRST_VAR, FIRST_VAR);
    }
    return e;
}

const ewff_t *ewff_false(void)
{
    static const ewff_t *e = NULL;
    if (e == NULL) {
        e = ewff_neg(ewff_true());
    }
    return e;
}

const ewff_t *ewff_eq(term_t t1, term_t t2)
{
    return NEW(((ewff_t) {
        .type = EWFF_EQ,
        .u.eq.t1 = t1,
        .u.eq.t2 = t2
    }));
}

const ewff_t *ewff_neq(term_t t1, term_t t2)
{
    return ewff_neg(ewff_eq(t1, t2));
}

const ewff_t *ewff_sort(term_t t, bool (*is_sort)(stdname_t n))
{
    return NEW(((ewff_t) {
        .type = EWFF_SORT,
        .u.sort.t = t,
        .u.sort.is_sort = is_sort
    }));
}

const ewff_t *ewff_neg(const ewff_t *e1)
{
    return NEW(((ewff_t) {
        .type = EWFF_NEG,
        .u.neg.e = e1
    }));
}

const ewff_t *ewff_or(const ewff_t *e1, const ewff_t *e2)
{
    return NEW(((ewff_t) {
        .type = EWFF_OR,
        .u.or.e1 = e1,
        .u.or.e2 = e2
    }));
}

const ewff_t *ewff_and(const ewff_t *e1, const ewff_t *e2)
{
    return ewff_neg(ewff_or(ewff_neg(e1), ewff_neg(e2)));
}

int ewff_cmp(const ewff_t *e1, const ewff_t *e2)
{
    if (e1->type != e2->type) {
        return e1->type < e2->type ? -1 : 1;
    }
    switch (e1->type) {
        case EWFF_EQ:
            return memcmp(&e1->u.eq, &e2->u.eq, sizeof(e1->u.eq));
        case EWFF_SORT:
            return memcmp(&e1->u.sort, &e2->u.sort, sizeof(e1->u.sort));
        case EWFF_NEG:
            return ewff_cmp(e1->u.neg.e, e2->u.neg.e);
        case EWFF_OR: {
            int i = ewff_cmp(e1->u.or.e1, e2->u.or.e1);
            return i != 0 ? i : ewff_cmp(e1->u.or.e2, e2->u.or.e2);
        }
        default:
            abort();
    }
}

void ewff_collect_vars(const ewff_t *e, varset_t *vars)
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

void ewff_collect_names(const ewff_t *e, stdset_t *names)
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
            if (t1 == t2) {
                return true;
            }
            assert(!IS_VARIABLE(t1) || varmap_contains(varmap, t1));
            assert(!IS_VARIABLE(t2) || varmap_contains(varmap, t2));
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


