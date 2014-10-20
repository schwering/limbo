// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include <kr2014.h>

// legacy function from query.c which is still useful for testing
static bool query_entailed_by_bat(
        const univ_clauses_t *static_bat,
        const box_univ_clauses_t *dynamic_bat,
        const stdvec_t *situation,
        const bitmap_t *sensings,
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
    phi = query_ennf(situation, phi, &hplus);
    phi = query_simplify(sensings, phi, &truth_value);
    if (phi == NULL) {
        return truth_value;
    }
    setup_t setup = ({
        const stdvecset_t zs = query_ennf_zs(phi);
        const setup_t static_setup = setup_init_static(static_bat, &hplus);
        const setup_t dynamic_setup = setup_init_dynamic(dynamic_bat, &hplus, &zs);
        const setup_t s = setup_union(&static_setup, &dynamic_setup);
        setup_add_sensing_results(&s, sensings);
        s;
    });
    cnf_t cnf = query_cnf(phi);
    truth_value = true;
    for (int i = 0; i < cnf_size(&cnf) && truth_value; ++i) {
        const clause_t *c = cnf_get(&cnf, i);
        truth_value = setup_entails(&setup, c, k);
    }
    return truth_value;
#endif
    context_t ctx = kcontext_init(static_bat, dynamic_bat);
    for (int i = 0; i < stdvec_size(situation); ++i) {
        const stdname_t n = stdvec_get(situation, i);
        const bool r = bitmap_get(sensings, i);
        context_add_action(&ctx, n, r);
    }
    return query_entailed(&ctx, false, phi, k);
}


START_TEST(test_bat_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    init_bat(&dynamic_bat, &static_bat, NULL);

    stdvec_t situation = stdvec_init_with_size(0);
    bitmap_t sensings = bitmap_init_with_size(0);

    stdvec_clear(&situation);
    bitmap_clear(&sensings);
    const query_t *phi1 =
        query_neg(
            query_or(
                Q(P(Z(), d0, A())),
                Q(P(Z(), d1, A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi1, 0));

    stdvec_clear(&situation);
    bitmap_clear(&sensings);
    const query_t *phi2 =
        query_act(forward,
            query_or(
                Q(P(Z(), d1, A())),
                Q(P(Z(), d2, A()))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi2, 0));

    stdvec_clear(&situation);
    stdvec_append(&situation, forward);
    bitmap_clear(&sensings);
    bitmap_append(&sensings, true);
    phi2 =
        query_or(
            Q(P(Z(), d1, A())),
            Q(P(Z(), d2, A())));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi2, 0));

    stdvec_clear(&situation);
    stdvec_append(&situation, forward);
    bitmap_clear(&sensings);
    bitmap_append(&sensings, true);
    ck_assert_int_eq(stdvec_size(&situation), 1);
    ck_assert_int_eq(bitmap_size(&sensings), 1);
    const query_t *phi3 =
        query_or(
            Q(P(Z(), d1, A())),
            Q(P(Z(), d2, A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi3, 1));

    stdvec_clear(&situation);
    bitmap_clear(&sensings);
    phi3 =
        query_act(forward,
            query_or(
                Q(P(Z(), d1, A())),
                Q(P(Z(), d2, A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi3, 1));

    stdvec_clear(&situation);
    stdvec_append(&situation, forward);
    stdvec_append(&situation, sonar);
    bitmap_clear(&sensings);
    bitmap_append(&sensings, true);
    bitmap_append(&sensings, true);
    const query_t *phi4 =
        query_or(
            Q(P(Z(), d0, A())),
            Q(P(Z(), d1, A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi4, 1));

    stdvec_clear(&situation);
    bitmap_clear(&sensings);
    phi4 =
        query_act(forward,
            query_act(sonar,
                query_or(
                    Q(P(Z(), d0, A())),
                    Q(P(Z(), d1, A())))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi4, 1));

    stdvec_clear(&situation);
    stdvec_append(&situation, forward);
    stdvec_append(&situation, sonar);
    bitmap_clear(&sensings);
    bitmap_append(&sensings, true);
    bitmap_append(&sensings, true);
    const query_t *phi5 =
        query_or(
            Q(P(Z(), d0, A())),
            Q(P(Z(), d1, A())));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi5, 1));

    stdvec_clear(&situation);
    bitmap_clear(&sensings);
    phi5 =
        query_act(forward,
            query_act(sonar,
                query_or(
                    Q(P(Z(), d0, A())),
                    Q(P(Z(), d1, A())))));
    ck_assert(!query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi5, 1));

    stdvec_clear(&situation);
    stdvec_append(&situation, forward);
    stdvec_append(&situation, sonar);
    bitmap_clear(&sensings);
    bitmap_append(&sensings, true);
    bitmap_append(&sensings, true);
    const query_t *phi6 =
        query_act(forward,
            query_or(
                Q(P(Z(), d0, A())),
                Q(P(Z(), d1, A()))));
    ck_assert(query_entailed_by_bat(&static_bat, &dynamic_bat, &situation, &sensings, phi6, 1));
}
END_TEST

START_TEST(test_setup_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    init_bat(&dynamic_bat, &static_bat, NULL);

    context_t ctx = kcontext_init(&static_bat, &dynamic_bat);

    //printf("Q0\n");
    const query_t *phi0 =
        query_and(
            Q(N(Z(), d0, A())),
            Q(N(Z(), d1, A())));
    ck_assert(query_entailed(&ctx, false, phi0, 0));

    //printf("Q1\n");
    const query_t *phi1 =
        query_neg(
            query_or(
                Q(P(Z(), d0, A())),
                Q(P(Z(), d1, A()))));
    ck_assert(query_entailed(&ctx, false, phi1, 0));

    //printf("Q2\n");
    const query_t *phi3 =
        query_act(forward,
            query_or(
                Q(P(Z(), d1, A())),
                Q(P(Z(), d2, A()))));
    ck_assert(query_entailed(&ctx, false, phi3, 1));

    //printf("Q3\n");
    const query_t *phi2 =
        query_act(forward,
            query_or(
                Q(P(Z(), d1, A())),
                Q(P(Z(), d2, A()))));
    ck_assert(!query_entailed(&ctx, false, phi2, 0));

    context_add_action(&ctx, forward, true);
    context_add_action(&ctx, sonar, true);

    //printf("Q4\n");
    const query_t *phi4 =
        query_or(
            Q(P(Z(), d0, A())),
            Q(P(Z(), d1, A())));
    ck_assert(query_entailed(&ctx, false, phi4, 1));

    //printf("Q5\n");
    const query_t *phi5 = Q(P(Z(), d0, A()));
    ck_assert(!query_entailed(&ctx, false, phi5, 1));

    //printf("Q6\n");
    const query_t *phi6 = Q(P(Z(), d1, A()));
    ck_assert(query_entailed(&ctx, false, phi6, 1));

    //printf("Q7\n");
    const query_t *phi7 =
        query_act(sonar,
            query_or(
                Q(P(Z(), d0, A())),
                Q(P(Z(), d1, A()))));
    ck_assert(query_entailed(&ctx, false, phi7, 1));

    //printf("Q8\n");
    const query_t *phi8 =
        query_act(sonar,
            query_act(sonar,
                query_or(
                    Q(P(Z(), d0, A())),
                    Q(P(Z(), d1, A())))));
    ck_assert(query_entailed(&ctx, false, phi8, 1));

    //printf("Q9\n");
    const query_t *phi9 =
        query_act(forward,
            query_or(
                Q(P(Z(), d0, A())),
                Q(P(Z(), d1, A()))));
    ck_assert(query_entailed(&ctx, false, phi9, 1));

    //printf("Q10\n");
    const query_t *phi10 =
        query_act(forward,
            query_act(forward,
                Q(P(Z(), d0, A()))));
    ck_assert(query_entailed(&ctx, false, phi10, 1));
}
END_TEST

START_TEST(test_eventual_completeness_tautology)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    const literal_t *A = P(Z(), 0, A());
    const literal_t *B = P(Z(), 1, A());
    const literal_t *negA = N(Z(), 0, A());
    const literal_t *negB = N(Z(), 1, A());

    context_t ctx1 = kcontext_init(&static_bat, &dynamic_bat);

    // tests tautology p v q v (~p ^ ~q)
    ck_assert(!query_entailed(&ctx1, false, query_or(query_or(Q(A), Q(B)), query_and(Q(negA), Q(negB))), 0));
    ck_assert(query_entailed(&ctx1, false, query_or(query_or(Q(A), Q(B)), query_and(Q(negA), Q(negB))), 1));

    ck_assert(!query_entailed(&ctx1, false, query_or(query_or(Q(A), Q(B)), query_neg(query_or(Q(A), Q(B)))), 0));
    ck_assert(query_entailed(&ctx1, false, query_or(query_or(Q(A), Q(B)), query_neg(query_or(Q(A), Q(B)))), 1));

    ck_assert(!query_entailed(&ctx1, false, query_or(Q(A), query_or(Q(B), query_neg(query_or(Q(A), Q(B))))), 0));
    ck_assert(query_entailed(&ctx1, false, query_or(Q(A), query_or(Q(B), query_neg(query_or(Q(A), Q(B))))), 1));

    // tests tautology (E x) (P(x) v ~P(x))
    var_t x = -1;
    const literal_t *p = P(Z(), 0, A(x));
    ck_assert(!query_entailed(&ctx1, false, query_exists(x, query_or(Q(p), query_neg(Q(p)))), 0));
    ck_assert(query_entailed(&ctx1, false, query_exists(x, query_or(Q(p), query_neg(Q(p)))), 1));
}
END_TEST

START_TEST(test_eventual_completeness_entailments)
{
    ({
        univ_clauses_t static_bat = univ_clauses_init();
        box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
        const literal_t *P1 = P(Z(), 0, A(1));
        const literal_t *P2 = P(Z(), 0, A(2));
        univ_clauses_append(&static_bat,univ_clause_init(TRUE,C(P1, P2)));

        context_t ctx1 = kcontext_init(&static_bat, &dynamic_bat);

        // tests entailment P(#1) v P(#2) |= (E x) P(x)
        var_t x = -1;
        const literal_t *P = P(Z(), 0, A(x));
        ck_assert(query_entailed(&ctx1, false, query_exists(x, Q(P)), 0));
        ck_assert(query_entailed(&ctx1, false, query_exists(x, Q(P)), 1));
        ck_assert(query_entailed(&ctx1, false, query_exists(x, Q(P)), 2));
    });

    ({
        univ_clauses_t static_bat = univ_clauses_init();
        box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
        const literal_t *P1 = P(Z(), 0, A(1));
        const literal_t *P2 = P(Z(), 0, A(2));
        const literal_t *Q3 = P(Z(), 1, A(2));
        const literal_t *negQ3 = N(Z(), 1, A(2));
        univ_clauses_append(&static_bat,univ_clause_init(TRUE,C(P1, P2, Q3)));
        univ_clauses_append(&static_bat,univ_clause_init(TRUE,C(P1, P2, negQ3)));

        context_t ctx1 = kcontext_init(&static_bat, &dynamic_bat);

        // tests entailment
        // (P(#1) v P(#2) v Q(#3)) ^ (P(#1) v P(#2) v ~Q(#3)) |= (E x) P(x)
        var_t x = -1;
        const literal_t *P = P(Z(), 0, A(x));
        ck_assert(!query_entailed(&ctx1, false, query_exists(x, Q(P)), 0));
        ck_assert(query_entailed(&ctx1, false, query_exists(x, Q(P)), 1));
        ck_assert(query_entailed(&ctx1, false, query_exists(x, Q(P)), 2));
    });
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_bat_entailment);
    tcase_add_test(tc_core, test_setup_entailment);
    tcase_add_test(tc_core, test_eventual_completeness_tautology);
    tcase_add_test(tc_core, test_eventual_completeness_entailments);
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

