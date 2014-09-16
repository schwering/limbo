// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bel.h"

START_TEST(test_morri_example)
{
    univ_clauses_t static_bat = univ_clauses_init();
    belief_conds_t belief_conds = belief_conds_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES;
    const int k = 2;

    const stdvec_t query_z = ({
        stdvec_t z = stdvec_init();
        stdvec_append(&z, SL);
        stdvec_append(&z, SR1);
        stdvec_append(&z, LV);
        stdvec_append(&z, SL);
        z;
    });
    const stdvecset_t query_zs = stdvecset_singleton(&query_z);
    const stdset_t hplus = ({
        stdset_t ns = stdset_init();
        stdset_add(&ns, LV);
        stdset_add(&ns, SL);
        stdset_add(&ns, SR1);
        const int n_vars = 0;
        stdset_t hplus = bbat_hplus(&static_bat, &belief_conds, &dynamic_bat, &ns, n_vars);
        stdset_add_all(&hplus, &ns);
        hplus;
    });
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    const setup_t static_and_dynamic_setup = setup_union(&static_setup, &dynamic_setup);
    bsetup_t setups = bsetup_init_beliefs(&static_and_dynamic_setup, &belief_conds, &hplus, k);
    pelsets_t pels = bsetup_pels(&setups);
    int pl;

    ck_assert_int_eq(bsetup_size(&setups), 3);

    // Property 1
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(N(Z(), L1, A())), k, &pl));
    ck_assert(pl == 0);

    // Property 2
    bsetup_add_sensing_results(&setups, SF(P(Z(), SF, A(SL))));
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(P(Z(SL), L1, A())), k, &pl));
    ck_assert_int_eq(pl, 1);
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(P(Z(SL), R1, A())), k, &pl));
    ck_assert_int_eq(pl, 1);

    // Property 3
    bsetup_add_sensing_results(&setups, SF(N(Z(SL), SF, A(SR1))));
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(N(Z(SL, SR1), R1, A())), k, &pl));
    ck_assert_int_eq(pl, 2);

    // Property 5
    ck_assert(!bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(P(Z(), L1, A())), k, &pl));
    ck_assert(!bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(N(Z(), L1, A())), k, &pl));

    // Property 6
    bsetup_add_sensing_results(&setups, SF(P(Z(SL, SR1), SF, A(LV))));
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(P(Z(SL, SR1, LV), R1, A())), k, &pl));
    ck_assert_int_eq(pl, 2);

    // Property 7
    bsetup_add_sensing_results(&setups, SF(P(Z(SL, SR1, LV), SF, A(SL))));
    ck_assert(bsetup_with_splits_and_sf_subsumes(&setups, &pels, C(P(Z(SL, SR1, LV, SL), L1, A())), k, &pl));
    ck_assert_int_eq(pl, 2);
}
END_TEST

START_TEST(test_morri_example_with_context)
{
    univ_clauses_t static_bat = univ_clauses_init();
    belief_conds_t belief_conds = belief_conds_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES;
    const int k = 2;
    context_t ctx1 = bcontext_init(&static_bat, &belief_conds, &dynamic_bat, k, Z(), SF());

    ck_assert_int_eq(bsetup_size(&ctx1.u.b.setups), 3);

    // Property 1
    const query_t *phi1 = Q(N(Z(), L1, A()));
    ck_assert(query_entailed(&ctx1, false, phi1, k));

    // Property 2
    const query_t *phi2 = query_and(Q(P(Z(), L1, A())), Q(P(Z(), R1, A())));
    context_t ctx2 = context_copy(&ctx1);
    //context_add_actions(&ctx2, Z(SL), SF(P(Z(), SF, A(SL))));
    CONTEXT_ADD_ACTIONS(&ctx2, {SL,true});
    ck_assert(query_entailed(&ctx2, false, phi2, k));
    ck_assert(!query_entailed(&ctx1, false, phi2, k)); // sensing really really is required

    // Property 3
    const query_t *phi3 = Q(N(Z(), R1, A()));
    context_t ctx3 = context_copy(&ctx2);
    //context_add_actions(&ctx3, Z(SR1), SF(N(Z(SL), SF, A(SR1))));
    CONTEXT_ADD_ACTIONS(&ctx3, {SR1,false});
    ck_assert(query_entailed(&ctx3, false, phi3, k));
    ck_assert(!query_entailed(&ctx2, false, phi3, k)); // sensing really really is required

    // Property 5
    const query_t *phi5a = Q(P(Z(), L1, A()));
    const query_t *phi5b = Q(N(Z(), L1, A()));
    ck_assert(!query_entailed(&ctx3, false, phi5a, k));
    ck_assert(!query_entailed(&ctx3, false, phi5b, k));

    // Property 6
    const query_t *phi6 = Q(P(Z(), R1, A()));
    context_t ctx4 = context_copy(&ctx3);
    //context_add_actions(&ctx4, Z(LV), SF(P(Z(SL, SR1), SF, A(LV))));
    CONTEXT_ADD_ACTIONS(&ctx4, {LV,true});
    ck_assert(query_entailed(&ctx4, false, phi6, k));
    ck_assert(!query_entailed(&ctx3, false, phi6, k)); // sensing really really is required

    // Property 7
    const query_t *phi7 = Q(P(Z(), L1, A()));
    context_t ctx5 = context_copy(&ctx4);
    //context_add_actions(&ctx5, Z(SL), SF(P(Z(SL, SR1, LV), SF, A(SL))));
    CONTEXT_ADD_ACTIONS(&ctx5, {SL,true});
    ck_assert(query_entailed(&ctx5, false, phi7, k));
    ck_assert(query_entailed(&ctx4, false, phi6, k)); // sensing really really is required
}
END_TEST

START_TEST(test_example_12)
{
    univ_clauses_t static_bat = univ_clauses_init();
    belief_conds_t belief_conds = belief_conds_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    const int k = 1;
    const literal_t *A = P(Z(), 0, A());
    const literal_t *B = P(Z(), 1, A());
    const literal_t *negA = N(Z(), 0, A());
    const literal_t *negB = N(Z(), 1, A());
    const literal_t *negC = N(Z(), 2, A());
    SBELIEF(TRUE, C(negA), C(B));
    SBELIEF(TRUE, C(negC), C(A));
    SBELIEF(TRUE, C(negC), C(negB));

    const stdvec_t query_z = stdvec_init();
    const stdvecset_t query_zs = stdvecset_singleton(&query_z);
    const stdset_t hplus = stdset_init();
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    const setup_t static_and_dynamic_setup = setup_union(&static_setup, &dynamic_setup);
    bsetup_t setups = bsetup_init_beliefs(&static_and_dynamic_setup, &belief_conds, &hplus, k);
    pelsets_t pels = bsetup_pels(&setups);

    ck_assert_int_eq(bsetup_size(&setups), 3);

    ck_assert(setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 0), pelsets_get(&pels, 0), C(negA, B), k));
    ck_assert(setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 0), pelsets_get(&pels, 0), C(negC), k));
    ck_assert(setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 1), pelsets_get(&pels, 1), C(negC, A), k));
    ck_assert(setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 1), pelsets_get(&pels, 1), C(negC, negB), k));
    ck_assert(!setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 1), pelsets_get(&pels, 1), C(A), k));
    ck_assert(!setup_with_splits_and_sf_subsumes(bsetup_get_unsafe(&setups, 1), pelsets_get(&pels, 1), C(negB), k));
}
END_TEST

START_TEST(test_inconsistency)
{
    univ_clauses_t static_bat = univ_clauses_init();
    belief_conds_t belief_conds = belief_conds_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    const literal_t *A = P(Z(), 0, A());
    const literal_t *B = P(Z(), 1, A());
    const literal_t *negA = N(Z(), 0, A());
    const literal_t *negB = N(Z(), 1, A());
    SBELIEF(TRUE, C(), C(   A,    B));
    SBELIEF(TRUE, C(), C(   A, negB));
    SBELIEF(TRUE, C(), C(negA,    B));
    SBELIEF(TRUE, C(), C(negA, negB));

    context_t ctx1 = bcontext_init(&static_bat, &belief_conds, &dynamic_bat, 0, Z(), SF());

    ck_assert_int_eq(bsetup_size(&ctx1.u.b.setups), 2);

    //print_setup(bsetup_get(&ctx1.u.b.setups, 0));
    //print_pel(pelsets_get(&ctx1.u.b.pels, 0));
    //print_setup(bsetup_get(&ctx1.u.b.setups, 1));
    //print_pel(pelsets_get(&ctx1.u.b.pels, 1));

    ck_assert(setup_subsumes(bsetup_get_unsafe(&ctx1.u.b.setups, 0), C(A, B)));
    ck_assert(!setup_subsumes(bsetup_get_unsafe(&ctx1.u.b.setups, 1), C(A, B)));

    ck_assert(query_entailed(&ctx1, false, query_or(Q(A), Q(B)), 0));
    ck_assert(!query_entailed(&ctx1, false, query_or(Q(A), Q(B)), 1));
    ck_assert(query_entailed(&ctx1, false, query_or(Q(A), Q(B)), 0));
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Belief");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_morri_example);
    tcase_add_test(tc_core, test_morri_example_with_context);
    tcase_add_test(tc_core, test_example_12);
    tcase_add_test(tc_core, test_inconsistency);
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

