// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "literal.h"
#include <string.h>

const pred_t SF = ~0;

literal_t literal_init(const stdvec_t *z, bool sign, pred_t pred,
        const stdvec_t *args)
{
    return (literal_t) {
        .z = (z == NULL) ? stdvec_init() : stdvec_copy(z),
        .sign = sign,
        .pred = pred,
        .args = stdvec_copy(args)
    };
}

literal_t literal_prepend(stdname_t n, const literal_t *l)
{
    return (literal_t) {
        .z = stdvec_prepend_copy(n, &l->z),
        .sign = l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

literal_t literal_prepend_all(const stdvec_t *z, const literal_t *l)
{
    return (literal_t) {
        .z = stdvec_concat(z, &l->z),
        .sign = l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

literal_t literal_append(const literal_t *l, stdname_t n)
{
    return (literal_t) {
        .z = stdvec_copy_append(&l->z, n),
        .sign = l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

literal_t literal_append_all(const literal_t *l, const stdvec_t *z)
{
    return (literal_t) {
        .z = stdvec_concat(&l->z, z),
        .sign = l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

literal_t literal_flip(const literal_t *l)
{
    return (literal_t) {
        .z = l->z,
        .sign = !l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

literal_t literal_substitute(const literal_t *l, const varmap_t *map)
{
    stdvec_t z = stdvec_lazy_copy(&l->z);
    stdvec_t args = stdvec_lazy_copy(&l->args);
    for (int i = 0; i < stdvec_size(&z); ++i) {
        const var_t x = stdvec_get(&z, i);
        if (IS_VARIABLE(x)) {
            assert(varmap_contains(map, x));
            stdvec_set(&z, i, varmap_lookup(map, x));
        }
    }
    for (int i = 0; i < stdvec_size(&args); ++i) {
        const var_t x = stdvec_get(&args, i);
        if (IS_VARIABLE(x)) {
            assert(varmap_contains(map, x));
            stdvec_set(&args, i, varmap_lookup(map, x));
        }
    }
    return (literal_t) {
        .z = z,
        .sign = l->sign,
        .pred = l->pred,
        .args = args
    };
}

void literal_cleanup(literal_t *l)
{
    stdvec_cleanup(&l->z);
    stdvec_cleanup(&l->args);
}

bool literal_is_ground(const literal_t *l)
{
    for (int i = 0; i < stdvec_size(&l->z); ++i) {
        const var_t x = stdvec_get(&l->z, i);
        if (IS_VARIABLE(x)) {
            return false;
        }
    }
    for (int i = 0; i < stdvec_size(&l->args); ++i) {
        const var_t x = stdvec_get(&l->args, i);
        if (IS_VARIABLE(x)) {
            return false;
        }
    }
    return true;
}

void literal_collect_vars(const literal_t *l, varset_t *vars)
{
    for (int i = 0; i < stdvec_size(&l->z); ++i) {
        const var_t x = stdvec_get(&l->z, i);
        if (IS_VARIABLE(x)) {
            varset_add(vars, x);
        }
    }
    for (int i = 0; i < stdvec_size(&l->args); ++i) {
        const var_t x = stdvec_get(&l->args, i);
        if (IS_VARIABLE(x)) {
            varset_add(vars, x);
        }
    }
}

void literal_collect_names(const literal_t *l, stdset_t *names)
{
    for (int i = 0; i < stdvec_size(&l->z); ++i) {
        const stdname_t n = stdvec_get(&l->z, i);
        if (IS_STDNAME(n)) {
            stdset_add(names, n);
        }
    }
    for (int i = 0; i < stdvec_size(&l->args); ++i) {
        const stdname_t n = stdvec_get(&l->args, i);
        if (IS_STDNAME(n)) {
            stdset_add(names, n);
        }
    }
}

int literal_cmp(const literal_t *l1, const literal_t *l2)
{
    int cmp;
    if ((cmp = stdvec_cmp(&l1->z, &l2->z)) != 0) {
        return cmp;
    } else if ((cmp = l1->pred - l2->pred) != 0) {
        return cmp;
    } else if ((cmp = stdvec_cmp(&l1->args, &l2->args)) != 0) {
        return cmp;
    } else if ((cmp = (int) l1->sign - (int) l2->sign) != 0) {
        return cmp;
    } else {
        return 0;
    }
}

int literal_cmp_flipped(const literal_t *l1, const literal_t *l2)
{
    const literal_t l3 = literal_flip(l2);
    return literal_cmp(l1, &l3);
}

bool literal_eq(const literal_t *l1, const literal_t *l2)
{
    return l1->pred == l2->pred && l1->sign == l2->sign &&
        stdvec_eq(&l1->z, &l2->z) && stdvec_eq(&l1->args, &l2->args);
}

const stdvec_t *literal_z(const literal_t *l)
{
    return &l->z;
}

bool literal_sign(const literal_t *l)
{
    return l->sign;
}

pred_t literal_pred(const literal_t *l)
{
    return l->pred;
}

const stdvec_t *literal_args(const literal_t *l)
{
    return &l->args;
}

