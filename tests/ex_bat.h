// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * The example from the KR paper on ESL.
 *
 * schwering@kbsg.rwth-aachen.de
 */
#include <assert.h>
#include <stdio.h>
#include "../src/query.h"
#include "../src/setup.h"
#include "../src/memory.h"
#include "../src/util.h"

static const stdname_t FORWARD = 1;
static const stdname_t SONAR   = 2;

#define D(i) ((pred_t) i)

static const var_t a = -12345;

static bool is_action(const stdname_t n)
{
    return n == FORWARD || n == SONAR;
}

static void print_stdname(stdname_t n)
{
    if (n == 1)       printf("f");
    else if (n == 2)  printf("s");
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

#define DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix, cond, clause) \
    const ewff_t *cond_##suffix = cond;\
    const clause_t *clause_##suffix = clause;\
    clause_type_prefix##s_add(clauses_set, clause_type_prefix##_init(cond_##suffix, clause_##suffix));

#define GEN_DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix, cond, clause) \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_1, cond, ({ const int i = 1; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_2, cond, ({ const int i = 2; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_3, cond, ({ const int i = 3; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_4, cond, ({ const int i = 4; clause; }));

#define DECL_ALL_CLAUSES(static_bat, dynamic_bat) \
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 1,\
            AND(SORT(a, action), NEQ(a, FORWARD)),\
            C(N(Z(), SF, A(a)), P(Z(), D(0), A()), P(Z(), D(1), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 2,\
            AND(SORT(a, action), AND(NEQ(a, FORWARD), NEQ(a, SONAR))),\
            C(N(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 3,\
            AND(SORT(a, action), EQ(a, FORWARD)),\
            C(P(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 4,\
            AND(SORT(a, action), EQ(a, SONAR)),\
            C(N(Z(), D(0), A()), P(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 5,\
            AND(SORT(a, action), EQ(a, SONAR)),\
            C(N(Z(), D(1), A()), P(Z(), SF, A(a))));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 6,\
            AND(SORT(a, action), EQ(a, FORWARD)),\
            C(N(Z(), D(i+1), A()), P(Z(a), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 7,\
            AND(SORT(a, action), NEQ(a, FORWARD)),\
            C(N(Z(), D(i), A()), P(Z(a), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 8,\
            AND(SORT(a, action), NEQ(a, FORWARD)),\
            C(N(Z(a), D(i), A()), P(Z(), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 9,\
            AND(SORT(a, action), EQ(a, FORWARD)),\
            C(P(Z(a), D(i), A()), N(Z(), D(i+1), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 10,\
            TRUE,\
            C(N(Z(), D(0), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 11,\
            TRUE,\
            C(N(Z(), D(1), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 12,\
            TRUE,\
            C(P(Z(), D(2), A()), P(Z(), D(3), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 13,\
            SORT(a, action),\
            C(N(Z(), D(0), A()), P(Z(a), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 14,\
            AND(SORT(a, action), NEQ(a, FORWARD)),\
            C(N(Z(a), D(0), A()), P(Z(), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 15,\
            AND(SORT(a, action), EQ(a, FORWARD)),\
            C(N(Z(), D(1), A()), P(Z(a), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 16,\
            AND(SORT(a, action), EQ(a, FORWARD)),\
            C(N(Z(a), D(0), A()), P(Z(), D(0), A()), P(Z(), D(1), A())));

