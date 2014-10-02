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

static const char *stdname_to_string(stdname_t val) {
    if (false) return "never occurs"; // never occurs
    else if (val == forward) return "forward";
    else if (val == sonar) return "sonar";
    static char buf[16];
    sprintf(buf, "#%ld", val);
    return buf;
}

static const char *pred_to_string(pred_t val) {
    if (false) return "never occurs"; // never occurs
    else if (val == POSS) return "POSS";
    else if (val == SF) return "SF";
    else if (val == d0) return "d0";
    else if (val == d1) return "d1";
    else if (val == d2) return "d2";
    else if (val == d3) return "d3";
    else if (val == d4) return "d4";
    static char buf[16];
    sprintf(buf, "P%d", val);
    return buf;
}

static stdname_t string_to_stdname(const char *str) {
    if (false) return -1; // never occurs;
    else if (!strcmp(str, "forward")) return forward;
    else if (!strcmp(str, "sonar")) return sonar;
    else return -1;
}

static pred_t string_to_pred(const char *str) {
    if (false) return -1; // never occurs;
    else if (!strcmp(str, "POSS")) return POSS;
    else if (!strcmp(str, "SF")) return SF;
    else if (!strcmp(str, "d0")) return d0;
    else if (!strcmp(str, "d1")) return d1;
    else if (!strcmp(str, "d2")) return d2;
    else if (!strcmp(str, "d3")) return d3;
    else if (!strcmp(str, "d4")) return d4;
    else return -1;
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

