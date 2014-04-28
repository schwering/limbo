// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bat.h"

// legacy function from query.c which is still useful for testing
static bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *context_z,
        const splitset_t *context_sf,
        const query_t *phi,
        const int k)
{
#if 0
    const stdset_t hplus = ({
        const stdset_t ns = query_names(phi);
        const int n_vars = query_n_vars(phi);
        stdset_t hplus = bat_hplus(static_bat, dynamic_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    bool truth_value;
    phi = query_ennf(context_z, phi, &hplus);
    phi = query_simplify(context_sf, phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }
    setup_t setup = ({
        const stdvecset_t zs = query_ennf_zs(phi);
        setup_t s = setup_init_static_and_dynamic(static_bat, dynamic_bat,
            &hplus, &zs);
        setup_add_sensing_results(&s, context_sf);
        s;
    });
    pelset_t pel = setup_pel(&setup);
    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        truth_value = setup_with_splits_and_sf_subsumes(&setup, &pel, c, k);
    }
    return truth_value;
#endif
    context_t ctx = kcontext_init(static_bat, dynamic_bat, context_z, context_sf);
    return query_entailed(&ctx, false, phi, k);
}


START_TEST(test_bat_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES(&static_bat, &dynamic_bat);

    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(FORWARD);
    const stdvec_t s_vec = stdvec_singleton(SONAR);
    const literal_t sensing_forward = literal_init(&empty_vec, true, SF, &f_vec);
    const literal_t sensing_sonar = literal_init(&f_vec, true, SF, &s_vec);
    stdvec_t context_z = stdvec_init_with_size(0);
    splitset_t context_sf = splitset_init_with_size(0);

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    const query_t *phi1 =
        query_neg(
            query_or(
                Q(P(Z(), D(0), A())),
                Q(P(Z(), D(1), A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi1, 0));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    const query_t *phi2 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(1), A())),
                Q(P(Z(), D(2), A()))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi2, 0));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    phi2 =
        query_or(
            Q(P(Z(), D(1), A())),
            Q(P(Z(), D(2), A())));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi2, 0));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    ck_assert_int_eq(stdvec_size(&context_z), 1);
    ck_assert_int_eq(splitset_size(&context_sf), 1);
    const query_t *phi3 =
        query_or(
            Q(P(Z(), D(1), A())),
            Q(P(Z(), D(2), A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi3, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    phi3 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(1), A())),
                Q(P(Z(), D(2), A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi3, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    splitset_add(&context_sf, &sensing_sonar);
    const query_t *phi4 =
        query_or(
            Q(P(Z(), D(0), A())),
            Q(P(Z(), D(1), A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi4, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    phi4 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    Q(P(Z(), D(0), A())),
                    Q(P(Z(), D(1), A())))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi4, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    splitset_add(&context_sf, &sensing_sonar);
    const query_t *phi5 =
        query_or(
            Q(P(Z(), D(0), A())),
            Q(P(Z(), D(1), A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi5, 1));

    stdvec_clear(&context_z);
    splitset_clear(&context_sf);
    phi5 =
        query_act(FORWARD,
            query_act(SONAR,
                query_or(
                    Q(P(Z(), D(0), A())),
                    Q(P(Z(), D(1), A())))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi5, 1));

    stdvec_clear(&context_z);
    stdvec_append(&context_z, FORWARD);
    stdvec_append(&context_z, SONAR);
    splitset_clear(&context_sf);
    splitset_add(&context_sf, &sensing_forward);
    splitset_add(&context_sf, &sensing_sonar);
    const query_t *phi6 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(0), A())),
                Q(P(Z(), D(1), A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &context_z, &context_sf, phi6, 1));
}
END_TEST

START_TEST(test_setup_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES(&static_bat, &dynamic_bat);

    context_t ctx = kcontext_init(&static_bat, &dynamic_bat, Z(), SF());

    //printf("Q0\n");
    const query_t *phi0 =
        query_and(
            Q(N(Z(), D(0), A())),
            Q(N(Z(), D(1), A())));
    ck_assert(query_entailed(&ctx, false, phi0, 0));

    //printf("Q1\n");
    const query_t *phi1 =
        query_neg(
            query_or(
                Q(P(Z(), D(0), A())),
                Q(P(Z(), D(1), A()))));
    ck_assert(query_entailed(&ctx, false, phi1, 0));

    //printf("Q2\n");
    const query_t *phi3 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(1), A())),
                Q(P(Z(), D(2), A()))));
    ck_assert(query_entailed(&ctx, false, phi3, 1));

    //printf("Q3\n");
    const query_t *phi2 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(1), A())),
                Q(P(Z(), D(2), A()))));
    ck_assert(!query_entailed(&ctx, false, phi2, 0));

    CONTEXT_ADD_ACTIONS(&ctx, {FORWARD,true}, {SONAR,true});

    //printf("Q4\n");
    const query_t *phi4 =
        query_or(
            Q(P(Z(), D(0), A())),
            Q(P(Z(), D(1), A())));
    ck_assert(query_entailed(&ctx, false, phi4, 1));

    //printf("Q5\n");
    const query_t *phi5 = Q(P(Z(), D(0), A()));
    ck_assert(!query_entailed(&ctx, false, phi5, 1));

    //printf("Q6\n");
    const query_t *phi6 = Q(P(Z(), D(1), A()));
    ck_assert(query_entailed(&ctx, false, phi6, 1));

    //printf("Q7\n");
    const query_t *phi7 =
        query_act(SONAR,
            query_or(
                Q(P(Z(), D(0), A())),
                Q(P(Z(), D(1), A()))));
    ck_assert(query_entailed(&ctx, false, phi7, 1));

    //printf("Q8\n");
    const query_t *phi8 =
        query_act(SONAR,
            query_act(SONAR,
                query_or(
                    Q(P(Z(), D(0), A())),
                    Q(P(Z(), D(1), A())))));
    ck_assert(query_entailed(&ctx, false, phi8, 1));

    //printf("Q9\n");
    const query_t *phi9 =
        query_act(FORWARD,
            query_or(
                Q(P(Z(), D(0), A())),
                Q(P(Z(), D(1), A()))));
    ck_assert(query_entailed(&ctx, false, phi9, 1));

    //printf("Q10\n");
    const query_t *phi10 =
        query_act(FORWARD,
            query_act(FORWARD,
                Q(P(Z(), D(0), A()))));
    ck_assert(query_entailed(&ctx, false, phi10, 1));
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

