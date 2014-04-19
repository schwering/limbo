// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bat.h"

START_TEST(test_grounding)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES(&static_bat, &dynamic_bat);

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
        stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    const setup_t setup = setup_init_static_and_dynamic(&static_bat, &dynamic_bat, &hplus, &query_zs);
    print_setup(&setup);
    const pelset_t pel = setup_pel(&setup);
    print_pel(&pel);
    setup_t setup_up = setup_lazy_copy(&setup);
    setup_propagate_units(&setup_up);
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

    const stdvec_t empty_vec = stdvec_init_with_size(0);
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const literal_t neg_sf = literal_init(&empty_vec, false, SF, &f_vec);
    const splitset_t sensing_results = splitset_singleton(&neg_sf);
    setup_add_sensing_results(&setup_up, &sensing_results);
    print_setup(&setup_up);
    setup_propagate_units(&setup_up);
    print_setup(&setup_up);
}
END_TEST

START_TEST(test_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES(&static_bat, &dynamic_bat);

    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const stdvec_t fs_vec = stdvec_concat(&f_vec, &s_vec);
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_init();//singleton(&fs_vec);
    context_t ctx1 = context_init(&static_bat, &dynamic_bat, Z(), SF());
    print_setup(&ctx1.setup);
    const setup_t setup = setup_init_static_and_dynamic(&static_bat, &dynamic_bat, &hplus, &query_zs);
    print_setup(&setup);
    return;
    const literal_t D0 = literal_init(&empty_vec, true, D(0), &empty_vec);
    const literal_t D1 = literal_init(&empty_vec, true, D(1), &empty_vec);
    const literal_t D2 = literal_init(&empty_vec, true, D(2), &empty_vec);
    const literal_t D3 = literal_init(&empty_vec, true, D(3), &empty_vec);
    const literal_t D4 = literal_init(&empty_vec, true, D(4), &empty_vec);
    const literal_t ND0 = literal_flip(&D0);
    const literal_t ND1 = literal_flip(&D1);
    const literal_t ND2 = literal_flip(&D2);
    const literal_t ND3 = literal_flip(&D3);
    const literal_t ND4 = literal_flip(&D4);
    const literal_t FD1 = literal_init(&f_vec, true, D(1), &empty_vec);
    const literal_t FD2 = literal_init(&f_vec, true, D(2), &empty_vec);

    clause_t d0 = clause_singleton(&D0);
    clause_t d1 = clause_singleton(&D1);
    clause_t d2 = clause_singleton(&D2);
    clause_t d3 = clause_singleton(&D3);
    clause_t d4 = clause_singleton(&D4);
    clause_t nd0 = clause_singleton(&ND0);
    clause_t nd1 = clause_singleton(&ND1);
    clause_t nd2 = clause_singleton(&ND2);
    clause_t nd3 = clause_singleton(&ND3);
    clause_t nd4 = clause_singleton(&ND4);

    setup_t setup_up;

    clause_t d0d1 = clause_init();
    clause_add(&d0d1, &D0);
    clause_add(&d0d1, &D1);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d0d1));

    clause_t d0d2 = clause_init();
    clause_add(&d0d1, &D0);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d0d2));

    clause_t d1d2 = clause_init();
    clause_add(&d0d1, &D1);
    clause_add(&d0d1, &D2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d1d2));

    clause_t d2d3 = clause_init();
    clause_add(&d0d1, &D1);
    clause_add(&d0d1, &D2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d2d3));

    clause_t fd1fd2 = clause_init();
    clause_add(&fd1fd2, &FD1);
    clause_add(&fd1fd2, &FD2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split D0
    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &d0);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &nd0);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split d1
    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &d1);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &nd1);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split d2
    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &d2);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &nd2);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    // split d3
    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &d3);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &nd3);
    //setup_propagate_units(&setup_up);
    print_setup(&setup_up);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    // inconsistent split
    setup_up = setup_lazy_copy(&setup);
    setup_add(&setup_up, &d4);
    setup_add(&setup_up, &nd4);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));
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

