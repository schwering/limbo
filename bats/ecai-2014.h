#include "common.h"

static const var_t A = -1;

static const stdname_t LV = 3;
static const stdname_t SL = 2;
static const stdname_t SR1 = 1;
static const stdname_t MAX_STD_NAME = 3;

static const pred_t L1 = 4;
static const pred_t L2 = 3;
static const pred_t POSS = 2;
static const pred_t R1 = 1;

static const char *stdname_to_string(stdname_t val) {
    if (false) return "never occurs"; // never occurs
    else if (val == LV) return "LV";
    else if (val == SL) return "SL";
    else if (val == SR1) return "SR1";
    static char buf[16];
    sprintf(buf, "#%ld", val);
    return buf;
}

static const char *pred_to_string(pred_t val) {
    if (false) return "never occurs"; // never occurs
    else if (val == L1) return "L1";
    else if (val == L2) return "L2";
    else if (val == POSS) return "POSS";
    else if (val == R1) return "R1";
    else if (val == SF) return "SF";
    static char buf[16];
    sprintf(buf, "P%d", val);
    return buf;
}

static stdname_t string_to_stdname(const char *str) {
    if (false) return -1; // never occurs;
    else if (!strcmp(str, "LV")) return LV;
    else if (!strcmp(str, "SL")) return SL;
    else if (!strcmp(str, "SR1")) return SR1;
    else return -1;
}

static pred_t string_to_pred(const char *str) {
    if (false) return -1; // never occurs;
    else if (!strcmp(str, "L1")) return L1;
    else if (!strcmp(str, "L2")) return L2;
    else if (!strcmp(str, "POSS")) return POSS;
    else if (!strcmp(str, "R1")) return R1;
    else if (!strcmp(str, "SF")) return SF;
    else return -1;
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

