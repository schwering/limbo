// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The example BAT from Morri & Co's AIJ paper on belief revision.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <assert.h>
#include <stdio.h>
#include "../src/belief.h"
#include "../src/memory.h"
#include "../src/query.h"
#include "../src/util.h"

static const stdname_t LV  = 1;
static const stdname_t SL  = 2;
static const stdname_t SR1 = 3;

static const pred_t POSS = 11111;
static const pred_t R1 = 0;
static const pred_t L1 = 1;
static const pred_t L2 = 2;

static const var_t a = -1;
static const var_t A0 = -1;

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
    if (p == SF)     printf("SF");
    else if (p == 0) printf("R1");
    else if (p == 1) printf("L1");
    else if (p == 2) printf("L2");
    else             printf("%d", p);
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

/*
#define DECL_ALL_CLAUSES \
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)), C(N(Z(a), R1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)), C(P(Z(a), R1, A()), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), NEQ(a, LV)), C(N(Z(a), R1, A()), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), NEQ(a, LV)), C(P(Z(a), R1, A()), N(Z(), R1, A())));\
    \
    DCLAUSE(SORT(a, action), C(N(Z(a), L1, A()), P(Z(), L1, A())));\
    DCLAUSE(SORT(a, action), C(P(Z(a), L1, A()), N(Z(), L1, A())));\
    \
    DCLAUSE(SORT(a, action), C(N(Z(a), L2, A()), P(Z(), L2, A())));\
    DCLAUSE(SORT(a, action), C(P(Z(a), L2, A()), N(Z(), L2, A())));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(N(Z(), SF, A(a)), P(Z(), L1, A()), P(Z(), L2, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(N(Z(), SF, A(a)), P(Z(), L1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(N(Z(), SF, A(a)), P(Z(), R1, A()), P(Z(), L2, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(N(Z(), SF, A(a)), P(Z(), R1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(P(Z(), SF, A(a)), N(Z(), L1, A()), N(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SL)), C(P(Z(), SF, A(a)), N(Z(), L2, A()), P(Z(), R1, A())));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, LV)), C(P(Z(), SF, A(a))));\
    \
    DCLAUSE(AND(SORT(a, action), EQ(a, SR1)), C(N(Z(), SF, A(a)), P(Z(), R1, A())));\
    DCLAUSE(AND(SORT(a, action), EQ(a, SR1)), C(P(Z(), SF, A(a)), N(Z(), R1, A())));\
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
            C(N(Z(), L2, A())));
*/

#define DECL_ALL_CLAUSES(dynamic_bat, static_bat, belief_conds) \
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(P(Z(),POSS,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,SL),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE))))),C(N(Z(),SF,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,SL),AND(NEQ(A0,LV),TRUE)))),C(N(Z(),SF,A(A0)),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE)))),C(N(Z(),SF,A(A0)),P(Z(),L2,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),L2,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE)))),C(N(Z(),SF,A(A0)),N(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),TRUE))),C(N(Z(),SF,A(A0)),N(Z(),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE)))),C(N(Z(),SF,A(A0)),P(Z(),L1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),L1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),L1,A()),P(Z(),L2,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(),SF,A(A0)),P(Z(),L1,A()),P(Z(),L2,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),L1,A()),N(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(),SF,A(A0)),P(Z(),L1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE)))),C(N(Z(),SF,A(A0)),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,SL),AND(NEQ(A0,LV),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),R1,A()),P(Z(),L2,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(),SF,A(A0)),P(Z(),R1,A()),P(Z(),L2,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),AND(NEQ(A0,SR1),TRUE))),C(N(Z(),SF,A(A0)),P(Z(),R1,A()),N(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(),SF,A(A0)),P(Z(),R1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,SL),TRUE)),C(N(Z(),L1,A()),N(Z(),R1,A()),P(Z(),SF,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,SL),TRUE)),C(N(Z(),L2,A()),P(Z(),R1,A()),P(Z(),SF,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,LV),TRUE)),C(P(Z(),SF,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,SR1),TRUE)),C(N(Z(),R1,A()),P(Z(),SF,A(A0)))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(N(Z(A0),R1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,LV),TRUE)),C(N(Z(A0),R1,A()),N(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(A0),R1,A()),P(Z(),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),AND(EQ(A0,LV),TRUE))),C(N(Z(A0),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(EQ(A0,LV),TRUE)),C(P(Z(),R1,A()),P(Z(A0),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),AND(NEQ(A0,LV),TRUE)),C(N(Z(),R1,A()),P(Z(A0),R1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(N(Z(A0),L1,A()),P(Z(),L1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(N(Z(),L1,A()),P(Z(A0),L1,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(N(Z(A0),L2,A()),P(Z(),L2,A()))));\
box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A0,action),TRUE),C(N(Z(),L2,A()),P(Z(A0),L2,A()))));\
belief_conds_append(belief_conds,belief_cond_init(TRUE,C(),C(N(Z(),L1,A()))));\
belief_conds_append(belief_conds,belief_cond_init(TRUE,C(),C(P(Z(),R1,A()))));\
belief_conds_append(belief_conds,belief_cond_init(TRUE,C(N(Z(),L1,A())),C(P(Z(),R1,A()))));\
belief_conds_append(belief_conds,belief_cond_init(TRUE,C(P(Z(),R1,A())),C(N(Z(),L2,A()))));

