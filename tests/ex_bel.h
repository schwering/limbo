// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The example from the KR paper on ESL.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <assert.h>
#include <stdio.h>
#include "../src/belief.h"
#include "../src/memory.h"
#include "../src/util.h"

static const stdname_t LV  = 1;
static const stdname_t SL  = 2;
static const stdname_t SR1 = 3;

static const pred_t R1 = 0;
static const pred_t L1 = 1;
static const pred_t L2 = 2;

static const var_t a = -1;

static bool is_action(const stdname_t n)
{
    return n == LV || n == SL || n == SR1;
}

static void print_stdname(stdname_t n)
{
    if (n == 1)       printf("lv");
    else if (n == 2)  printf("sL");
    else if (n == 3)  printf("sR1");
    else              printf("#%ld", n);
}

static void print_pred(pred_t p)
{
    if (p == SF)    printf("SF");
    else            printf("d%d", p);
}

static void print_z(const stdvec_t *z)
{
    printf("[");
    for (int i = 0; i < stdvec_size(z); ++i) {
        if (i > 0) printf(",");
        print_stdname(stdvec_get(z, i));
    }
    printf("]");
}

static void print_literal(const literal_t *l)
{
    if (stdvec_size(literal_z(l)) > 0) {
        print_z(literal_z(l));
    }
    if (!literal_sign(l)) {
        printf("~");
    }
    print_pred(literal_pred(l));
    if (stdvec_size(literal_args(l)) > 0) {
        printf("(");
        for (int i = 0; i < stdvec_size(literal_args(l)); ++i) {
            if (i > 0) printf(",");
            print_stdname(stdvec_get(literal_args(l), i));
        }
        printf(")");
    }
}

static void print_clause(const clause_t *c)
{
    printf("[ ");
    for (int i = 0; i < clause_size(c); ++i) {
        //if (i > 0) printf(" v ");
        if (i > 0) printf(", ");
        print_literal(clause_get(c, i));
    }
    //printf("\n");
    printf(" ]\n");
}

static void print_setup(const setup_t *setup)
{
    printf("Setup:\n");
    printf("---------------\n");
    for (int i = 0; i < setup_size(setup); ++i) {
        print_clause(setup_get(setup, i));
    }
    printf("---------------\n");
}

static void print_pel(const pelset_t *pel)
{
    printf("PEL:\n");
    printf("---------------\n");
    for (int i = 0; i < pelset_size(pel); ++i) {
        print_literal(pelset_get(pel, i));
        printf("\n");
    }
    printf("---------------\n");
}

#define DCLAUSE(cond, clause) \
    box_univ_clauses_append(&dynamic_bat, box_univ_clause_init(cond, clause))

#define SCLAUSE(cond, clause) \
    univ_clauses_append(&static_bat, univ_clause_init(cond, clause))

#define SBELIEF(cond, neg_phi, psi) \
    belief_conds_append(&belief_conds, belief_cond_init(cond, neg_phi, psi));

#define DECL_ALL_CLAUSES \
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)),\
            C(N(Z(a), R1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)),\
            C(P(Z(a), R1, A()), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), NEQ(a, LV)),\
            C(N(Z(a), R1, A()), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), NEQ(a, LV)),\
            C(P(Z(a), R1, A()), N(Z(), R1, A())));\
    \
    DCLAUSE(TRUE,\
            C(N(Z(a), L1, A()), P(Z(), L1, A())));\
    DCLAUSE(TRUE,\
            C(P(Z(a), L1, A()), N(Z(), L1, A())));\
    \
    DCLAUSE(TRUE,\
            C(N(Z(a), L2, A()), P(Z(), L2, A())));\
    DCLAUSE(TRUE,\
            C(P(Z(a), L2, A()), N(Z(), L2, A())));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(N(Z(), SF, A(a)), P(Z(), L1, A()), P(Z(), L2, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(N(Z(), SF, A(a)), P(Z(), L1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(N(Z(), SF, A(a)), P(Z(), R1, A()), P(Z(), L2, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(N(Z(), SF, A(a)), P(Z(), R1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(P(Z(), SF, A(a)), N(Z(), L1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)),\
            C(P(Z(), SF, A(a)), N(Z(), L2, A()), P(Z(), R1, A())));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)),\
            C(P(Z(), SF, A(a))));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, SR1)),\
            C(N(Z(), SF, A(a)), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SR1)),\
            C(P(Z(), SF, A(a)), N(Z(), R1, A())));\
    \
    SBELIEF(TRUE,\
            C(),\
            C(N(Z(), L1, A())));\
    SBELIEF(TRUE,\
            C(),\
            C(P(Z(), R1, A())));\
    \
    SBELIEF(TRUE,\
            C(N(Z(), L1, A())),\
            C(P(Z(), R1, A())));\
    \
    SBELIEF(TRUE,\
            C(P(Z(), R1, A())),\
            C(N(Z(), L1, A())));

