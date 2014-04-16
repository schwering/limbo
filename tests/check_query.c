// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bat.h"

START_TEST(test_bat_entailment)
{
    univ_clauses_t static_bat;
    box_univ_clauses_t dynamic_bat;
    make_bat(&static_bat, &dynamic_bat);
    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const literal_t sensing_forward = literal_init(&empty_vec, true, SF, &f_vec);
    const literal_t sensing_sonar = literal_init(&f_vec, true, SF, &s_vec);
    stdvec_t context_z = stdvec_init_with_size(0);
    splitset_t context_sf = splitset_init_with_size(0);

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    const query_t *phi1 =
        query_neg(
            query_or(
                query_atom(D(0), empty_vec),
                query_atom(D(1), empty_vec)));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi1, 0));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    const query_t *phi2 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), empty_vec),
                query_atom(D(2), empty_vec)));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi2, 0));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    phi2 =
        query_or(
            query_atom(D(1), empty_vec),
            query_atom(D(2), empty_vec));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi2, 0));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    const query_t *phi3 =
        query_or(
            query_atom(D(1), empty_vec),
            query_atom(D(2), empty_vec));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi3, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    phi3 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), empty_vec),
                query_atom(D(2), empty_vec)));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi3, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    splitset_add(&context_sf, &sensing_sonar);
    const query_t *phi4 =
        query_or(
            query_atom(D(0), empty_vec),
            query_atom(D(1), empty_vec));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi4, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    splitset_add(&context_sf, &sensing_sonar);
    phi4 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    query_atom(D(0), empty_vec),
                    query_atom(D(1), empty_vec))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi4, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    const query_t *phi5 =
        query_or(
            query_atom(D(0), empty_vec),
            query_atom(D(1), empty_vec));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi5, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    phi5 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    query_atom(D(0), empty_vec),
                    query_atom(D(1), empty_vec))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi5, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_sonar);
    const query_t *phi6 =
        query_act(FORWARD,
            query_or(
                query_atom(D(0), empty_vec),
                query_atom(D(1), empty_vec)));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi6, 1));
}
END_TEST

START_TEST(test_setup_entailment)
{
    univ_clauses_t static_bat;
    box_univ_clauses_t dynamic_bat;
    make_bat(&static_bat, &dynamic_bat);
    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const literal_t sensing_forward = literal_init(&empty_vec, true, SF, &f_vec);
    const literal_t sensing_sonar = literal_init(&f_vec, true, SF, &s_vec);

    stdvec_t context_z_1 = stdvec_init_with_size(0);
    splitset_t context_sf_1 = splitset_init_with_size(0);
    context_t ctx1 = context_init(&static_bat, &dynamic_bat, &context_z_1, &context_sf_1);

    //printf("Q0\n");
    const query_t *phi0 =
        query_and(
            query_lit(empty_vec, false, D(0), empty_vec),
            query_lit(empty_vec, false, D(1), empty_vec));
    ck_assert(query_entailed_by_setup(&ctx1, false, phi0, 0));

    //printf("Q1\n");
    const query_t *phi1 =
        query_neg(
            query_or(
                query_atom(D(0), empty_vec),
                query_atom(D(1), empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx1, false, phi1, 0));

    //printf("Q2\n");
    const query_t *phi3 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), empty_vec),
                query_atom(D(2), empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx1, false, phi3, 1));

    //printf("Q3\n");
    const query_t *phi2 =
        query_act(FORWARD,
            query_or(
                query_atom(D(1), empty_vec),
                query_atom(D(2), empty_vec)));
    ck_assert(!query_entailed_by_setup(&ctx1, false, phi2, 0));

    stdvec_t context_z_2 = stdvec_init_with_size(0);
    splitset_t context_sf_2 = splitset_init_with_size(0);
    stdvec_append(&context_z_2, FORWARD);
    stdvec_append(&context_z_2, SONAR);
    splitset_add(&context_sf_2, &sensing_forward);
    splitset_add(&context_sf_2, &sensing_sonar);
    context_t ctx2 = context_copy_with_new_actions(&ctx1, &context_z_2, &context_sf_2);

    //printf("Q4\n");
    const query_t *phi4 =
        query_or(
            query_atom(D(0), empty_vec),
            query_atom(D(1), empty_vec));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi4, 1));

    //printf("Q5\n");
    const query_t *phi5 = query_atom(D(0), empty_vec);
    ck_assert(!query_entailed_by_setup(&ctx2, false, phi5, 1));

    //printf("Q6\n");
    const query_t *phi6 = query_atom(D(1), empty_vec);
    ck_assert(query_entailed_by_setup(&ctx2, false, phi6, 1));

    //printf("Q7\n");
    const query_t *phi7 =
        query_act(SONAR,
            query_or(
                query_atom(D(0), empty_vec),
                query_atom(D(1), empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi7, 1));

    //printf("Q8\n");
    const query_t *phi8 =
        query_act(SONAR,
            query_act(SONAR,
                query_or(
                    query_atom(D(0), empty_vec),
                    query_atom(D(1), empty_vec))));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi8, 1));

    //printf("Q9\n");
    const query_t *phi9 =
        query_act(FORWARD,
            query_or(
                query_atom(D(0), empty_vec),
                query_atom(D(1), empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi9, 1));

    //printf("Q10\n");
    const query_t *phi10 =
        query_act(FORWARD,
            query_act(FORWARD,
                query_atom(D(0), empty_vec)));
    ck_assert(query_entailed_by_setup(&ctx2, false, phi10, 1));
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_bat_entailment);
    tcase_add_test(tc_core, test_setup_entailment);
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

