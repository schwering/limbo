// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bat.h"

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

    query_t *phi1 =
        query_neg(
            query_or(
                query_lit(empty_vec, true, D(0), empty_vec),
                query_lit(empty_vec, true, D(1), empty_vec)));
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi1, 0));

    query_t *phi2 =
        query_act(FORWARD,
            query_or(
                query_lit(empty_vec, true, D(1), empty_vec),
                query_lit(empty_vec, true, D(2), empty_vec)));
    ck_assert(!query_test(&dynamic_bat, &static_bat, &sensing_results, phi2, 0));

    query_t *phi3 =
        query_act(FORWARD,
            query_or(
                query_lit(empty_vec, true, D(1), empty_vec),
                query_lit(empty_vec, true, D(2), empty_vec)));
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi3, 1));

    query_t *phi4 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    query_lit(empty_vec, true, D(0), empty_vec),
                    query_lit(empty_vec, true, D(1), empty_vec))));
    const literal_t sensing_sonar = literal_init(&f_vec, true, SF, &s_vec);
    litset_add(&sensing_results, &sensing_sonar);
    ck_assert(query_test(&dynamic_bat, &static_bat, &sensing_results, phi4, 1));

    query_t *phi5 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    query_lit(empty_vec, true, D(0), empty_vec),
                    query_lit(empty_vec, true, D(1), empty_vec))));
    litset_clear(&sensing_results);
    ck_assert(!query_test(&dynamic_bat, &static_bat, &sensing_results, phi5, 1));
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc_core = tcase_create("Core");
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

