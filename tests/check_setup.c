// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/setup.h"

static const stdname_t FORWARD = 1;
static const stdname_t SONAR   = 2;

//static const pred_t SF = -1;
#define D(i) ((pred_t) i)

static const var_t a = 12345;

#define NEW(expr)    ({ typeof(expr) *ptr = malloc(sizeof(expr)); *ptr = expr; ptr; })

static bool is_action(const stdname_t n)
{
    return n == FORWARD || n == SONAR;
}

static const clause_t *c1(const varmap_t *map)
{
    ck_assert(!varmap_contains(map, 0));
    ck_assert(varmap_contains(map, a));
    ck_assert(!varmap_contains(map, a-1));
    ck_assert(!varmap_contains(map, a+1));
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
    const bool cond = true;
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(0), &empty_vec))));
}

static const clause_t *c11(const varmap_t *map)
{
    const bool cond = true;
    const stdvec_t empty_vec = stdvec_init();
    if (!cond) {
        return NULL;
    }
    return NEW(clause_singleton(NEW(literal_init(&empty_vec, false, D(1), &empty_vec))));
}

static const clause_t *c12(const varmap_t *map)
{
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
//MAKE_UNIV_CLAUSES(4);
//MAKE_UNIV_CLAUSES(5);
//MAKE_UNIV_CLAUSES(6);
//MAKE_UNIV_CLAUSES(7);
//MAKE_UNIV_CLAUSES(8);
//MAKE_UNIV_CLAUSES(9);

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

static void print_literal(const literal_t *l)
{
    if (stdvec_size(literal_z(l)) > 0) {
        printf("[");
        for (int i = 0; i < stdvec_size(literal_z(l)); ++i) {
            if (i > 0) printf(",");
            print_stdname(stdvec_get(literal_z(l), i));
        }
        printf("]");
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

static void make_bat(box_univ_clauses_t *dynamic_bat, univ_clauses_t *static_bat)
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
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c71 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c72 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c73 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c81 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c82 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c83 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c91 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c92 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c93 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c13 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c14 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c15 } })));
        box_univ_clauses_append(&cs, NEW(((box_univ_clause_t) { .c = (univ_clause_t) { .names = names, .vars = vars, .univ_clause = &c16 } })));
        cs;
    });
}

START_TEST(test_grounding)
{
    box_univ_clauses_t dynamic_bat;
    univ_clauses_t static_bat;
    make_bat(&dynamic_bat, &static_bat);
    const stdvec_t query_z = ({
        stdvec_t z = stdvec_init();
        stdvec_append(&z, FORWARD);
        stdvec_append(&z, SONAR);
        z;
    });
    const stdvecset_t query_zs = stdvecset_singleton(&query_z);
    const stdset_t hplus = ({
        stdset_t ns = stdset_init();
        stdset_add(&ns, FORWARD);
        stdset_add(&ns, SONAR);
        const int n_vars = 0;
        stdset_t hplus = bat_hplus(&dynamic_bat, &static_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    const setup_t setup = setup_ground_clauses(&dynamic_bat, &static_bat, &hplus, &query_zs);
    print_setup(&setup);
    const pelset_t pel = setup_pel(&setup);
    print_pel(&pel);
    const litset_t split = litset_init();
    const setup_t setup_up = setup_propagate_units(&setup, &split);
    print_setup(&setup_up);

    ck_assert(!setup_contains(&setup_up, clause_empty()));
    for (int i = 0; i < setup_size(&setup); ++i) {
        const clause_t *c = setup_get(&setup, i);
        bool subsumed = false;
        for (int j = 0; j < setup_size(&setup_up); ++j) {
            const clause_t *d = setup_get(&setup_up, j);
            if (clause_contains_all(c, d)) {
                subsumed = true;
                break;
            }
        }
        ck_assert(subsumed);
    }
}
END_TEST

START_TEST(test_entailment)
{
    box_univ_clauses_t dynamic_bat;
    univ_clauses_t static_bat;
    make_bat(&dynamic_bat, &static_bat);
    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const literal_t sensing_forward = literal_init(&empty_vec, true, SF, &f_vec);
    litset_t sensing_results = litset_singleton(&sensing_forward);

    query_t *phi1 = NEW(((query_t) {
        .type = NEG,
        .u.neg = (query_unary_t) {
            .phi = NEW(((query_t) {
                .type = OR,
                .u.or = (query_binary_t) {
                    .phi1 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(0), &empty_vec)
                    })),
                    .phi2 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(1), &empty_vec)
                    }))
                }
            }))
        }
    }));
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi1, 0));

    query_t *phi2 = NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = FORWARD,
            .phi = NEW(((query_t) {
                .type = OR,
                .u.or = (query_binary_t) {
                    .phi1 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(1), &empty_vec)
                    })),
                    .phi2 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(2), &empty_vec)
                    }))
                }
            }))
        }
    }));
    ck_assert(!query_test(&dynamic_bat, &static_bat, &sensing_results, phi2, 0));

    query_t *phi3 = NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = FORWARD,
            .phi = NEW(((query_t) {
                .type = OR,
                .u.or = (query_binary_t) {
                    .phi1 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(1), &empty_vec)
                    })),
                    .phi2 = NEW(((query_t) {
                        .type = LIT,
                        .u.lit = literal_init(&empty_vec, true, D(2), &empty_vec)
                    }))
                }
            }))
        }
    }));
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi3, 1));

    query_t *phi4 = NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = FORWARD,
            .phi = NEW(((query_t) {
                .type = ACT,
                .u.act = (query_action_t) {
                    .n = SONAR,
                    .phi = NEW(((query_t) {
                        .type = OR,
                        .u.or = (query_binary_t) {
                            .phi1 = NEW(((query_t) {
                                .type = LIT,
                                .u.lit = literal_init(&empty_vec, true, D(0), &empty_vec)
                            })),
                            .phi2 = NEW(((query_t) {
                                .type = LIT,
                                .u.lit = literal_init(&empty_vec, true, D(1), &empty_vec)
                            }))
                        }
                    }))
                }
            }))
        }
    }));
    const literal_t sensing_sonar = literal_init(&f_vec, true, SF, &s_vec);
    litset_add(&sensing_results, &sensing_sonar);
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi4, 1));

    query_t *phi5 = NEW(((query_t) {
        .type = ACT,
        .u.act = (query_action_t) {
            .n = FORWARD,
            .phi = NEW(((query_t) {
                .type = ACT,
                .u.act = (query_action_t) {
                    .n = SONAR,
                    .phi = NEW(((query_t) {
                        .type = OR,
                        .u.or = (query_binary_t) {
                            .phi1 = NEW(((query_t) {
                                .type = LIT,
                                .u.lit = literal_init(&empty_vec, true, D(0), &empty_vec)
                            })),
                            .phi2 = NEW(((query_t) {
                                .type = LIT,
                                .u.lit = literal_init(&empty_vec, true, D(1), &empty_vec)
                            }))
                        }
                    }))
                }
            }))
        }
    }));
    litset_clear(&sensing_results);
    ck_assert(!query_test(&dynamic_bat, &static_bat, &sensing_results, phi5, 1));
}
END_TEST

Suite *clause_suite(void)
{
  Suite *s = suite_create("Setup");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_grounding);
  tcase_add_test(tc_core, test_entailment);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = clause_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

