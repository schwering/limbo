// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
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

#if 0
static const clause_t *c1(const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n != FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, SF, &a_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(1), &empty_vec)));
    return c;
}

static const clause_t *c2(const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n != FORWARD && n != SONAR;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, SF, &a_vec))));
}

static const clause_t *c3(const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, true, SF, &a_vec))));
}

static const clause_t *c4(const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == SONAR;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, SF, &a_vec)));
    return c;
}

static const clause_t *c5(const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == SONAR;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(1), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, SF, &a_vec)));
    return c;
}

static const clause_t *gen_c6(int i, const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(i+1), &empty_vec)));
    clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
    return c;
}

static const clause_t *gen_c7(int i, const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n != FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(i), &empty_vec)));
    clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
    return c;
}

static const clause_t *gen_c8(int i, const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n != FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&a_vec, false, D(i), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(i), &empty_vec)));
    return c;
}

static const clause_t *gen_c9(int i, const varmap_t *map)
{
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&a_vec, true, D(i), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, false, D(i+1), &empty_vec)));
    return c;
}

static const clause_t *c10(const varmap_t *map)
{
    assert(varmap_size(map) >= 0); // suppress unused map
    const bool cond = true;
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(0), &empty_vec))));
}

static const clause_t *c11(const varmap_t *map)
{
    assert(varmap_size(map) >= 0); // suppress unused map
    const bool cond = true;
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(1), &empty_vec))));
}

static const clause_t *c12(const varmap_t *map)
{
    assert(varmap_size(map) >= 0); // suppress unused map
    const bool cond = true;
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, true, D(2), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(3), &empty_vec)));
    return c;
}

static const clause_t *c13(const varmap_t *map)
{
    // missing in the paper
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n);
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&a_vec, true, D(0), &empty_vec)));
    return c;
}

static const clause_t *c14(const varmap_t *map)
{
    // missing in the paper
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n != FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&a_vec, false, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(0), &empty_vec)));
    return c;
}

static const clause_t *c15(const varmap_t *map)
{
    // missing in the paper
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&empty_vec, false, D(1), &empty_vec)));
    clause_add(c, NEW(literal_init(&a_vec, true, D(0), &empty_vec)));
    return c;
}

static const clause_t *c16(const varmap_t *map)
{
    // missing in the paper
    const stdname_t n = varmap_lookup(map, a);
    const bool cond = is_action(n) && n == FORWARD;
    const stdvec_t a_vec = stdvec_from_array(&n, 1);
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    clause_t *c = NEW(clause_init());
    clause_add(c, NEW(literal_init(&a_vec, false, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(0), &empty_vec)));
    clause_add(c, NEW(literal_init(&empty_vec, true, D(1), &empty_vec)));
    return c;
}

// instantiate the generic clauses for some i > 0

#define MAKE_UNIV_CLAUSES(i)\
    static const clause_t *c6##i(const varmap_t *map) { return gen_c6(i, map); }\
    static const clause_t *c7##i(const varmap_t *map) { return gen_c7(i, map); }\
    static const clause_t *c8##i(const varmap_t *map) { return gen_c8(i, map); }\
    static const clause_t *c9##i(const varmap_t *map) { return gen_c9(i, map); }

MAKE_UNIV_CLAUSES(1);
MAKE_UNIV_CLAUSES(2);
MAKE_UNIV_CLAUSES(3);
MAKE_UNIV_CLAUSES(4);
//MAKE_UNIV_CLAUSES(5);
//MAKE_UNIV_CLAUSES(6);
//MAKE_UNIV_CLAUSES(7);
//MAKE_UNIV_CLAUSES(8);
//MAKE_UNIV_CLAUSES(9);
#endif

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

#if 0
static void make_bat(univ_clauses_t *static_bat, box_univ_clauses_t *dynamic_bat)
{
    *static_bat = ({
        univ_clauses_t cs = univ_clauses_init();
        univ_clauses_append(&cs, NEW(((univ_clause_t) { .names = stdset_init(), .vars = varset_init(), .univ_clause = &c10 })));
        univ_clauses_append(&cs, NEW(((univ_clause_t) { .names = stdset_init(), .vars = varset_init(), .univ_clause = &c11 })));
        univ_clauses_append(&cs, NEW(((univ_clause_t) { .names = stdset_init(), .vars = varset_init(), .univ_clause = &c12 })));
        cs;
    });
    *dynamic_bat = ({
        const varset_t vars = varset_singleton(a);
        stdset_t names = stdset_init();
        stdset_add(&names, FORWARD);
        stdset_add(&names, SONAR);
        box_univ_clauses_t cs = box_univ_clauses_init();
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c1  } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c2  } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c3  } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c4  } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c5  } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c61 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c62 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c63 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c64 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c71 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c72 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c73 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c74 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c81 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c82 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c83 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c84 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c91 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c92 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c93 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c94 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c13 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c14 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c15 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c16 } })));
        cs;
    });
}
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

#define DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix, cond, clause) \
    DECL_CLAUSE(suffix, ({ UNUSED(check_arg); cond; }), clause);\
    clause_type_prefix##s_add(clauses_set, clause_type_prefix##_init(&check_##suffix, NULL, clause_##suffix));

#define GEN_DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix, cond, clause) \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_1, cond, ({ const int i = 1; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_2, cond, ({ const int i = 2; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_3, cond, ({ const int i = 3; clause; })); \
    DECL_AND_ADD_CLAUSE(clause_type_prefix, clauses_set, suffix##_4, cond, ({ const int i = 4; clause; }));

#define DECL_ALL_CLAUSES(static_bat, dynamic_bat) \
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 1,\
            is_action(V(a)) && V(a) != FORWARD,\
            C(N(Z(), SF, A(a)), P(Z(), D(0), A()), P(Z(), D(1), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 2,\
            is_action(V(a)) && V(a) != FORWARD && V(a) != SONAR,\
            C(N(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 3,\
            is_action(V(a)) && V(a) == FORWARD,\
            C(P(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 4,\
            is_action(V(a)) && V(a) == SONAR,\
            C(N(Z(), D(0), A()), P(Z(), SF, A(a))));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 5,\
            is_action(V(a)) && V(a) == SONAR,\
            C(N(Z(), D(1), A()), P(Z(), SF, A(a))));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 6,\
            is_action(V(a)) && V(a) == FORWARD,\
            C(N(Z(), D(i+1), A()), P(Z(a), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 7,\
            is_action(V(a)) && V(a) != FORWARD,\
            C(N(Z(), D(i), A()), P(Z(a), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 8,\
            is_action(V(a)) && V(a) != FORWARD,\
            C(N(Z(a), D(i), A()), P(Z(), D(i), A())));\
    GEN_DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 9,\
            is_action(V(a)) && V(a) == FORWARD,\
            C(P(Z(a), D(i), A()), N(Z(), D(i+1), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 10,\
            ({ UNUSED(varmap); true; }),\
            C(N(Z(), D(0), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 11,\
            ({ UNUSED(varmap); true; }),\
            C(N(Z(), D(1), A())));\
    DECL_AND_ADD_CLAUSE(univ_clause, static_bat, 12,\
            ({ UNUSED(varmap); true; }),\
            C(P(Z(), D(2), A()), P(Z(), D(3), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 13,\
            is_action(V(a)),\
            C(N(Z(), D(0), A()), P(Z(a), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 14,\
            is_action(V(a)) && V(a) != FORWARD,\
            C(N(Z(a), D(0), A()), P(Z(), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 15,\
            is_action(V(a)) && V(a) == FORWARD,\
            C(N(Z(), D(1), A()), P(Z(a), D(0), A())));\
    DECL_AND_ADD_CLAUSE(box_univ_clause, dynamic_bat, 16,\
            is_action(V(a)) && V(a) == FORWARD,\
            C(N(Z(a), D(0), A()), P(Z(), D(0), A()), P(Z(), D(1), A())));

