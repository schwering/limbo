// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "literal.h"
#include <stdlib.h>
#include <string.h>

literal_t literal_init(const stdvec_t *z, bool sign, pred_t pred,
        const stdvec_t *args)
{
    literal_t l;
    if (z == NULL) {
        l.z = stdvec_init();
    } else {
        l.z = stdvec_copy(z);
    }
    l.sign = sign;
    l.pred = pred;
    l.args = stdvec_copy(args);
    return l;
}

void literal_append(literal_t *l1, const literal_t *l2, stdname_t n)
{
    l1->z = stdvec_copy_append(&l2->z, n);
    l1->sign = l2->sign;
    l1->pred = l2->pred;
    memcpy(&l1->args, &l2->args, sizeof(l1->args));
}

void literal_flip(literal_t *l1, const literal_t *l2)
{
    memcpy(l1, l2, sizeof(*l1));
    l1->sign = !l1->sign;
}

void literal_free(literal_t *l1)
{
    stdvec_free(&l1->z);
    stdvec_free(&l1->args);
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

