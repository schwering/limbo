#include "bat-common.h"

static const var_t A = -6;
static const var_t A1 = -5;
static const var_t A2 = -4;
static const var_t X = -3;
static const var_t X1 = -2;
static const var_t X2 = -1;

static const stdname_t counter = 20;
static const stdname_t cup = 19;
static const stdname_t drop_o1 = 18;
static const stdname_t drop_o2 = 17;
static const stdname_t drop_o3 = 16;
static const stdname_t goto_counter = 15;
static const stdname_t goto_table = 14;
static const stdname_t mug = 13;
static const stdname_t o1 = 12;
static const stdname_t o2 = 11;
static const stdname_t o3 = 10;
static const stdname_t pickup_o1 = 9;
static const stdname_t pickup_o2 = 8;
static const stdname_t pickup_o3 = 7;
static const stdname_t sense_counter = 6;
static const stdname_t sense_o1 = 5;
static const stdname_t sense_o2 = 4;
static const stdname_t sense_o3 = 3;
static const stdname_t sense_table = 2;
static const stdname_t table = 1;
static const stdname_t MAX_STD_NAME = 20;

static const pred_t POSS = 4;
static const pred_t at = 2;
static const pred_t loc = 1;
static const pred_t type = 0;

static void print_stdname(stdname_t name) {
    if (false) printf("never occurs");
    else if (name == counter) printf("counter");
    else if (name == cup) printf("cup");
    else if (name == drop_o1) printf("drop_o1");
    else if (name == drop_o2) printf("drop_o2");
    else if (name == drop_o3) printf("drop_o3");
    else if (name == goto_counter) printf("goto_counter");
    else if (name == goto_table) printf("goto_table");
    else if (name == mug) printf("mug");
    else if (name == o1) printf("o1");
    else if (name == o2) printf("o2");
    else if (name == o3) printf("o3");
    else if (name == pickup_o1) printf("pickup_o1");
    else if (name == pickup_o2) printf("pickup_o2");
    else if (name == pickup_o3) printf("pickup_o3");
    else if (name == sense_counter) printf("sense_counter");
    else if (name == sense_o1) printf("sense_o1");
    else if (name == sense_o2) printf("sense_o2");
    else if (name == sense_o3) printf("sense_o3");
    else if (name == sense_table) printf("sense_table");
    else if (name == table) printf("table");
    else printf("#%ld", name);
}

static void print_pred(pred_t name) {
    if (false) printf("never occurs");
    else if (name == POSS) printf("POSS");
    else if (name == SR) printf("SR");
    else if (name == at) printf("at");
    else if (name == loc) printf("loc");
    else if (name == type) printf("type");
    else printf("%d", name);
}

static bool is_action(stdname_t name) {
    return name > MAX_STD_NAME || name == sense_o3 || name == sense_o2 || name == sense_o1 || name == sense_table || name == sense_counter || name == drop_o3 || name == drop_o2 || name == drop_o1 || name == pickup_o3 || name == pickup_o2 || name == pickup_o1;
}

static bool is_location(stdname_t name) {
    return name > MAX_STD_NAME || name == table || name == counter;
}

static bool is_object(stdname_t name) {
    return name > MAX_STD_NAME || name == o1 || name == o2 || name == o1;
}

static bool is_type(stdname_t name) {
    return name > MAX_STD_NAME || name == mug || name == cup;
}

#define DECL_ALL_CLAUSES(dynamic_bat, static_bat, belief_conds)\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sense_counter),AND(EQ(X1,counter),TRUE))),C(N(Z(),POSS,A(A)),P(Z(),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sense_counter),AND(EQ(X1,counter),TRUE))),C(N(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sense_table),AND(EQ(X1,table),TRUE))),C(N(Z(),POSS,A(A)),P(Z(),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(EQ(A,sense_table),AND(EQ(X1,table),TRUE))),C(N(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o1),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o1),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o1),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o2),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o2),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o2),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o3),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,sense_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o3),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,pickup_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),POSS,A(A)),N(Z(),loc,A(X1)),P(Z(),at,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o3),TRUE))),C(P(Z(),loc,A(X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,drop_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),at,A(X2,X1)),P(Z(),POSS,A(A)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(EQ(A,sense_counter),AND(EQ(X2,counter),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(EQ(A,sense_counter),AND(EQ(X2,counter),TRUE)))),C(N(Z(),at,A(X1,X2)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(EQ(A,sense_table),AND(EQ(X2,table),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(EQ(A,sense_table),AND(EQ(X2,table),TRUE)))),C(N(Z(),at,A(X1,X2)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(NEQ(X1,cup),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(EQ(X1,cup),AND(EQ(X2,o1),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(NEQ(X1,mug),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(EQ(X2,o1),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o1),AND(EQ(X1,mug),AND(EQ(X2,o1),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(NEQ(X1,cup),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(EQ(X1,cup),AND(EQ(X2,o2),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(NEQ(X1,mug),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(EQ(X2,o2),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o2),AND(EQ(X1,mug),AND(EQ(X2,o2),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(NEQ(X1,cup),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(EQ(X1,cup),AND(EQ(X2,o3),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(NEQ(X1,mug),TRUE)))),C(N(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(EQ(X2,o3),TRUE)))),C(N(Z(),SR,A(A,X1)),P(Z(),type,A(X2,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,type),AND(EQ(A,sense_o3),AND(EQ(X1,mug),AND(EQ(X2,o3),TRUE))))),C(N(Z(),type,A(X2,X1)),P(Z(),SR,A(A,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,type),TRUE))),C(N(Z(A),type,A(X1,X2)),P(Z(),type,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(X1,type),AND(SORT(A2,action),TRUE))),C(N(Z(),type,A(X,X1)),P(Z(A2),type,A(X,X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(NEQ(A,goto_table),AND(NEQ(A,goto_counter),TRUE)))),C(N(Z(A),loc,A(X1)),P(Z(),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(NEQ(A,goto_table),AND(NEQ(X1,counter),AND(EQ(A,goto_table),TRUE))))),C(N(Z(A),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(NEQ(A,goto_table),AND(NEQ(X1,counter),TRUE)))),C(N(Z(A),loc,A(X1)),P(Z(),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,location),AND(SORT(A1,action),AND(NEQ(X,table),AND(NEQ(A1,goto_counter),AND(EQ(A1,goto_counter),TRUE))))),C(N(Z(A1),loc,A(X)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,location),AND(SORT(A1,action),AND(NEQ(X,table),AND(NEQ(A1,goto_counter),TRUE)))),C(N(Z(A1),loc,A(X)),P(Z(),loc,A(X)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,location),AND(SORT(A1,action),AND(NEQ(X,table),AND(NEQ(X,counter),AND(EQ(A1,goto_table),TRUE))))),C(N(Z(A1),loc,A(X)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,location),AND(SORT(A1,action),AND(NEQ(X,table),AND(NEQ(X,counter),AND(EQ(A1,goto_counter),TRUE))))),C(N(Z(A1),loc,A(X)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,location),AND(SORT(A1,action),AND(NEQ(X,table),AND(NEQ(X,counter),TRUE)))),C(N(Z(A1),loc,A(X)),P(Z(),loc,A(X)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,goto_table),AND(EQ(X1,table),TRUE)))),C(P(Z(A),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(EQ(A,goto_counter),AND(EQ(X1,counter),TRUE)))),C(P(Z(A),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,location),AND(NEQ(A,goto_table),AND(NEQ(A,goto_counter),TRUE)))),C(N(Z(),loc,A(X1)),P(Z(A),loc,A(X1)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(A,drop_o2),AND(NEQ(A,drop_o3),AND(NEQ(X1,o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE))))))))),C(N(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(A,drop_o2),AND(NEQ(A,drop_o3),TRUE)))))),C(N(Z(A),at,A(X1,X2)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(A,drop_o2),AND(NEQ(X1,o3),AND(NEQ(X1,o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE))))))))),C(N(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(A,drop_o2),AND(NEQ(X1,o3),TRUE)))))),C(N(Z(A),at,A(X1,X2)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(X1,o2),AND(NEQ(A,drop_o3),AND(NEQ(X1,o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE))))))))),C(N(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(X1,o2),AND(NEQ(A,drop_o3),TRUE)))))),C(N(Z(A),at,A(X1,X2)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),AND(NEQ(X1,o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE))))))))),C(N(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,drop_o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE)))))),C(N(Z(A),at,A(X1,X2)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(A1,drop_o2),AND(NEQ(A1,drop_o3),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(A1,drop_o2),AND(NEQ(A1,drop_o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(A1,drop_o2),AND(NEQ(X,o3),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(A1,drop_o2),AND(NEQ(X,o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(A1,drop_o3),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(A1,drop_o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),AND(EQ(A1,pickup_o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),AND(NEQ(X,o1),AND(EQ(A1,pickup_o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(EQ(A1,pickup_o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE))))))))),C(N(Z(A1),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),at,A(X,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(EQ(A,pickup_o1),AND(NEQ(X1,o2),AND(NEQ(X1,o3),TRUE)))))),C(N(Z(A),at,A(X1,X2)),P(Z(),loc,A(X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(EQ(A1,pickup_o2),AND(NEQ(X,o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),loc,A(X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(EQ(A1,pickup_o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),loc,A(X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(X,object),AND(SORT(A1,action),AND(SORT(X2,location),AND(NEQ(X,o1),AND(NEQ(X,o2),AND(NEQ(X,o3),TRUE)))))),C(N(Z(A1),at,A(X,X2)),P(Z(),loc,A(X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),TRUE))),C(N(Z(A),at,A(X1,X2)),P(Z(),loc,A(X2)),P(Z(),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(EQ(A,drop_o1),AND(EQ(X1,o1),TRUE))))),C(N(Z(),loc,A(X2)),P(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(EQ(A,drop_o2),AND(EQ(X1,o2),TRUE))))),C(N(Z(),loc,A(X2)),P(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(EQ(A,drop_o3),AND(EQ(X1,o3),TRUE))))),C(N(Z(),loc,A(X2)),P(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,pickup_o1),AND(EQ(X1,o1),TRUE))))),C(N(Z(),at,A(X1,X2)),P(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,pickup_o2),AND(EQ(X1,o2),TRUE))))),C(N(Z(),at,A(X1,X2)),P(Z(A),at,A(X1,X2)))));\
    box_univ_clauses_append(dynamic_bat,box_univ_clause_init(AND(SORT(A,action),AND(SORT(X1,object),AND(SORT(X2,location),AND(NEQ(A,pickup_o3),AND(EQ(X1,o3),TRUE))))),C(N(Z(),at,A(X1,X2)),P(Z(A),at,A(X1,X2)))));\
    univ_clauses_append(static_bat,univ_clause_init(AND(EQ(X,table),TRUE),C(P(Z(),loc,A(X)))));

