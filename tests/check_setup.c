// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bat.h"

START_TEST(test_grounding)
{
    univ_clauses_t static_bat;
    box_univ_clauses_t dynamic_bat;
    make_bat(&static_bat, &dynamic_bat);
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
    const splitset_t split = splitset_init();
    setup_t setup_up = setup_lazy_copy(&setup);
    setup_propagate_units(&setup_up, &split);
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
    univ_clauses_t static_bat;
    box_univ_clauses_t dynamic_bat;
    make_bat(&static_bat, &dynamic_bat);
    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const stdvec_t fs_vec = stdvec_concat(&f_vec, &s_vec);
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_singleton(&fs_vec);
    const setup_t setup = setup_init_static_and_dynamic(&static_bat, &dynamic_bat, &hplus, &query_zs);
    const literal_t d0 = literal_init(&empty_vec, true, D(0), &empty_vec);
    const literal_t d1 = literal_init(&empty_vec, true, D(1), &empty_vec);
    const literal_t d2 = literal_init(&empty_vec, true, D(2), &empty_vec);
    const literal_t d3 = literal_init(&empty_vec, true, D(3), &empty_vec);
    const literal_t d4 = literal_init(&empty_vec, true, D(4), &empty_vec);
    const literal_t nd0 = literal_flip(&d0);
    const literal_t nd1 = literal_flip(&d1);
    const literal_t nd2 = literal_flip(&d2);
    const literal_t nd3 = literal_flip(&d3);
    const literal_t nd4 = literal_flip(&d4);
    const literal_t fd1 = literal_init(&f_vec, true, D(1), &empty_vec);
    const literal_t fd2 = literal_init(&f_vec, true, D(2), &empty_vec);

    splitset_t split = splitset_init();

    clause_t d0d1 = clause_init();
    clause_add(&d0d1, &d0);
    clause_add(&d0d1, &d1);
    ck_assert(!setup_subsumes(&setup, &split, &d0d1));

    clause_t d0d2 = clause_init();
    clause_add(&d0d1, &d0);
    ck_assert(!setup_subsumes(&setup, &split, &d0d2));

    clause_t d1d2 = clause_init();
    clause_add(&d0d1, &d1);
    clause_add(&d0d1, &d2);
    ck_assert(!setup_subsumes(&setup, &split, &d1d2));

    clause_t d2d3 = clause_init();
    clause_add(&d0d1, &d1);
    clause_add(&d0d1, &d2);
    ck_assert(!setup_subsumes(&setup, &split, &d2d3));

    clause_t fd1fd2 = clause_init();
    clause_add(&fd1fd2, &fd1);
    clause_add(&fd1fd2, &fd2);
    ck_assert(!setup_subsumes(&setup, &split, &fd1fd2));

    // split d0
    splitset_clear(&split);
    splitset_add(&split, &d0);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    splitset_clear(&split);
    splitset_add(&split, &nd0);
    ck_assert(!setup_subsumes(&setup, &split, &fd1fd2));

    // split d1
    splitset_clear(&split);
    splitset_add(&split, &d1);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    splitset_clear(&split);
    splitset_add(&split, &nd1);
    ck_assert(!setup_subsumes(&setup, &split, &fd1fd2));

    // split d2
    splitset_clear(&split);
    splitset_add(&split, &d2);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    splitset_clear(&split);
    splitset_add(&split, &nd2);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    // split d3
    splitset_clear(&split);
    splitset_add(&split, &d3);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    splitset_clear(&split);
    splitset_add(&split, &nd3);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));

    // inconsistent split
    splitset_clear(&split);
    splitset_add(&split, &d4);
    splitset_add(&split, &nd4);
    ck_assert(setup_subsumes(&setup, &split, &fd1fd2));
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

