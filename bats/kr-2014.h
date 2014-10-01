#include "common.h"

static const var_t A = -1;

static const stdname_t forward = 2;
static const stdname_t sonar = 1;
static const stdname_t MAX_STD_NAME = 2;

static const pred_t POSS = 6;
static const pred_t d0 = 4;
static const pred_t d1 = 3;
static const pred_t d2 = 2;
static const pred_t d3 = 1;
static const pred_t d4 = 0;

static void print_stdname(stdname_t name) {
    if (false) printf("never occurs");
    else if (name == forward) printf("forward");
    else if (name == sonar) printf("sonar");
    else printf("#%ld", name);
}

static void print_pred(pred_t name) {
    if (false) printf("never occurs");
    else if (name == POSS) printf("POSS");
    else if (name == SF) printf("SF");
    else if (name == d0) printf("d0");
    else if (name == d1) printf("d1");
    else if (name == d2) printf("d2");
    else if (name == d3) printf("d3");
    else if (name == d4) printf("d4");
    else printf("%d", name);
}

static bool is_action(stdname_t name) {
    return name > MAX_STD_NAME || name == sonar || name == forward;
}

#define DECL_ALL_CLAUSES(dynamic_bat, static_bat, belief_conds)\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),AND(NEQ(A,sonar),TRUE))),C(N(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,sonar),TRUE)),C(N(Z(),POSS,A(A)),N(Z(),d0,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(P(Z(),d0,A()),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sonar),TRUE)),C(P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),AND(NEQ(A,sonar),TRUE))),C(N(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(),SF,A(A)),P(Z(),d0,A()),P(Z(),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sonar),TRUE)),C(N(Z(),d0,A()),P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sonar),TRUE)),C(N(Z(),d1,A()),P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(A),d0,A()),P(Z(),d0,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),d0,A()),P(Z(),d1,A()),P(Z(),d0,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(),d1,A()),P(Z(A),d0,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(),d0,A()),P(Z(A),d0,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),AND(EQ(A,forward),TRUE))),C(N(Z(A),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(A),d1,A()),P(Z(),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(A),d1,A()),P(Z(),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),d1,A()),P(Z(),d2,A()),P(Z(),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(),d2,A()),P(Z(A),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(),d1,A()),P(Z(A),d1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),AND(EQ(A,forward),TRUE))),C(N(Z(A),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(A),d2,A()),P(Z(),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(A),d2,A()),P(Z(),d3,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),d2,A()),P(Z(),d3,A()),P(Z(),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(),d3,A()),P(Z(A),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(),d2,A()),P(Z(A),d2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),AND(EQ(A,forward),TRUE))),C(N(Z(A),d3,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(A),d3,A()),P(Z(),d3,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(A),d3,A()),P(Z(),d4,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),d3,A()),P(Z(),d4,A()),P(Z(),d3,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,forward),TRUE)),C(N(Z(),d4,A()),P(Z(A),d3,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,forward),TRUE)),C(N(Z(),d3,A()),P(Z(A),d3,A()))));\
    univ_clauses_append(static_bat,univ_clause_init(TRUE,C(N(Z(),d0,A()))));\
    univ_clauses_append(static_bat,univ_clause_init(TRUE,C(N(Z(),d1,A()))));\
    univ_clauses_append(static_bat,univ_clause_init(TRUE,C(P(Z(),d2,A()),P(Z(),d3,A()))));

