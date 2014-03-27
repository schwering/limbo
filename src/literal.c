// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "literal.h"
#include <stdlib.h>
#include <string.h>

literal_t literal_init(const stdvec_t *z, bool sign, pred_t pred,
        const stdvec_t *args)
{
    return (literal_t) {
        .z = (z == NULL) ? stdvec_init() : *z,
        .sign = sign,
        .pred = pred,
        .args = stdvec_copy(args)
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

literal_t literal_flip(const literal_t *l)
{
    return (literal_t) {
        .z = l->z,
        .sign = !l->sign,
        .pred = l->pred,
        .args = l->args
    };
}

void literal_free(literal_t *l)
{
    stdvec_free(&l->z);
    stdvec_free(&l->args);
}

int literal_cmp(const literal_t *l1, const literal_t *l2)
{
    int cmp;
    if ((cmp = l1->pred - l2->pred) != 0) {
        return cmp;
    } else if ((cmp = stdvec_cmp(&l1->z, &l2->z)) != 0) {
        return cmp;
    } else if ((cmp = stdvec_cmp(&l1->args, &l2->args)) != 0) {
        return cmp;
    } else if ((cmp = (int) l1->pred - (int) l2->pred) != 0) {
        return cmp;
    } else {
        return 0;
    }
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

