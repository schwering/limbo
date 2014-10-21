// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _EWFF_H_
#define _EWFF_H_

#include "term.h"

enum ewff_type { EWFF_EQ, EWFF_SORT, EWFF_NEG, EWFF_OR };

typedef struct ewff ewff_t;

struct ewff {
    enum ewff_type type;
    union {
        struct { term_t t1; term_t t2; } eq;
        struct { term_t t; bool (*is_sort)(stdname_t n); } sort;
        struct { const ewff_t *e; } neg;
        struct { const ewff_t *e1; const ewff_t *e2; } or;
    } u;
};

const ewff_t *ewff_true(void);
const ewff_t *ewff_false(void);
const ewff_t *ewff_eq(term_t t1, term_t t2);
const ewff_t *ewff_neq(term_t t1, term_t t2);
const ewff_t *ewff_sort(term_t t, bool (*is_sort)(stdname_t n));
const ewff_t *ewff_neg(const ewff_t *e1);
const ewff_t *ewff_or(const ewff_t *e1, const ewff_t *e2);
const ewff_t *ewff_and(const ewff_t *e1, const ewff_t *e2);

int ewff_cmp(const ewff_t *e1, const ewff_t *e2);

bool ewff_eval(const ewff_t *e, const varmap_t *varmap);

void ewff_ground(
        const ewff_t *e,
        const varset_t *vars,
        const stdset_t *hplus,
        void (*ground)(const varmap_t *));

void ewff_collect_vars(const ewff_t *e, varset_t *vars);
void ewff_collect_names(const ewff_t *e, stdset_t *names);

#endif

