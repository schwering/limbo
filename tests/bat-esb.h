#include "bat-common.h"

static const var_t A = -1;

static const stdname_t LV = 3;
static const stdname_t SL = 2;
static const stdname_t SR1 = 1;
static const stdname_t MAX_STD_NAME = 3;

static const pred_t L1 = 4;
static const pred_t L2 = 3;
static const pred_t POSS = 2;
static const pred_t R1 = 1;

static void print_stdname(stdname_t name) {
    if (false) printf("never occurs");
    else if (name == LV) printf("LV");
    else if (name == SL) printf("SL");
    else if (name == SR1) printf("SR1");
    else printf("#%ld", name);
}

static void print_pred(pred_t name) {
    if (false) printf("never occurs");
    else if (name == L1) printf("L1");
    else if (name == L2) printf("L2");
    else if (name == POSS) printf("POSS");
    else if (name == R1) printf("R1");
    else if (name == SF) printf("SF");
    else printf("%d", name);
}

static bool is_action(stdname_t name) {
    return name > MAX_STD_NAME || name == SR1 || name == SL || name == LV;
}

#define DECL_ALL_CLAUSES(dynamic_bat, static_bat, belief_conds)\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,SL),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE))))),C(N(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,SL),AND(NEQ(A,LV),TRUE)))),C(N(Z(),SF,A(A)),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE)))),C(N(Z(),SF,A(A)),P(Z(),L2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),TRUE))),C(N(Z(),SF,A(A)),P(Z(),L2,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE)))),C(N(Z(),SF,A(A)),N(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),TRUE))),C(N(Z(),SF,A(A)),N(Z(),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE)))),C(N(Z(),SF,A(A)),P(Z(),L1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),TRUE))),C(N(Z(),SF,A(A)),P(Z(),L1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE))),C(N(Z(),SF,A(A)),P(Z(),L1,A()),P(Z(),L2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(),SF,A(A)),P(Z(),L1,A()),P(Z(),L2,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE))),C(N(Z(),SF,A(A)),P(Z(),L1,A()),N(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(),SF,A(A)),P(Z(),L1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE)))),C(N(Z(),SF,A(A)),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,SL),AND(NEQ(A,LV),TRUE))),C(N(Z(),SF,A(A)),P(Z(),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE))),C(N(Z(),SF,A(A)),P(Z(),R1,A()),P(Z(),L2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(),SF,A(A)),P(Z(),R1,A()),P(Z(),L2,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),AND(NEQ(A,SR1),TRUE))),C(N(Z(),SF,A(A)),P(Z(),R1,A()),N(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(),SF,A(A)),P(Z(),R1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,SL),TRUE)),C(N(Z(),L1,A()),N(Z(),R1,A()),P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,SL),TRUE)),C(N(Z(),L2,A()),P(Z(),R1,A()),P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,LV),TRUE)),C(P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,SR1),TRUE)),C(N(Z(),R1,A()),P(Z(),SF,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),R1,A()),N(Z(),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,LV),TRUE)),C(N(Z(A),R1,A()),N(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(A),R1,A()),P(Z(),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),AND(EQ(A,LV),TRUE))),C(N(Z(A),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,LV),TRUE)),C(P(Z(),R1,A()),P(Z(A),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(NEQ(A,LV),TRUE)),C(N(Z(),R1,A()),P(Z(A),R1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),L1,A()),P(Z(),L1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(),L1,A()),P(Z(A),L1,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(A),L2,A()),P(Z(),L2,A()))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),TRUE),C(N(Z(),L2,A()),P(Z(A),L2,A()))));\
    belief_conds_append(belief_conds,belief_cond_init(TRUE,C(),C(N(Z(),L1,A()))));\
    belief_conds_append(belief_conds,belief_cond_init(TRUE,C(),C(P(Z(),R1,A()))));\
    belief_conds_append(belief_conds,belief_cond_init(TRUE,C(N(Z(),L1,A())),C(P(Z(),R1,A()))));\
    belief_conds_append(belief_conds,belief_cond_init(TRUE,C(P(Z(),R1,A())),C(N(Z(),L2,A()))));

