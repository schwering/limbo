// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * Extended literals are literals with an action sequence `z'.
 * A literal is a predicate symbol `pred' with vector of arguments `args'
 * which has an either positive or negative `sign'.
 *
 * literal_append() creates a new literal with a new action appended to its
 * action sequence.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _LITERAL_H_
#define _LITERAL_H_

#include "term.h"

typedef int pred_t;
typedef struct {
    stdvec_t z;
    bool sign;
    pred_t pred;
    stdvec_t args;
} literal_t;

extern const pred_t SF;

literal_t literal_init(const stdvec_t *z, bool sign, pred_t pred,
        const stdvec_t *args);
literal_t literal_prepend(stdname_t n, const literal_t *l);
literal_t literal_prepend_all(const stdvec_t *z, const literal_t *l);
literal_t literal_append(const literal_t *l, stdname_t n);
literal_t literal_append_all(const literal_t *l, const stdvec_t *z);
literal_t literal_flip(const literal_t *l);
literal_t literal_substitute(const literal_t *l, const varmap_t *map);
void literal_cleanup(literal_t *l);

bool literal_is_ground(const literal_t *l);
void literal_collect_vars(const literal_t *l, varset_t *vars);
void literal_collect_names(const literal_t *l, stdset_t *names);

int literal_cmp_flipped(const literal_t *l1, const literal_t *l2);
int literal_cmp(const literal_t *l1, const literal_t *l2);
bool literal_eq(const literal_t *l1, const literal_t *l2);

const stdvec_t *literal_z(const literal_t *l);
bool literal_sign(const literal_t *l);
pred_t literal_pred(const literal_t *l);
const stdvec_t *literal_args(const literal_t *l);

#endif

