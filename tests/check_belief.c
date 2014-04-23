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
    pelset_t pel;
    int pl;

    ck_assert_int_eq(bsetup_size(&setups), 3);

    // Property 1
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(N(Z(), L1, A())), k, &pl));
    ck_assert(pl == 0);

    // Property 2
    bsetup_add_sensing_results(&setups, SF(P(Z(), SF, A(SL))));
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(P(Z(SL), L1, A())), k, &pl));
    ck_assert(pl == 1);
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(P(Z(SL), R1, A())), k, &pl));
    ck_assert(pl == 1);

    // Property 3
    bsetup_add_sensing_results(&setups, SF(N(Z(SL), SF, A(SR1))));
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(N(Z(SL, SR1), R1, A())), k, &pl));
    ck_assert(pl == 2);

    // Property 5
    pel = bsetup_pel(&setups);
    ck_assert(!bsetup_with_splits_subsumes(&setups, &pel, C(P(Z(), L1, A())), k, &pl));
    pel = bsetup_pel(&setups);
    ck_assert(!bsetup_with_splits_subsumes(&setups, &pel, C(N(Z(), L1, A())), k, &pl));

    // Property 6
    bsetup_add_sensing_results(&setups, SF(P(Z(SL, SR1), SF, A(LV))));
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(P(Z(SL, SR1, LV), R1, A())), k, &pl));
    ck_assert(pl == 2);

    // Property 7
    bsetup_add_sensing_results(&setups, SF(P(Z(SL, SR1, LV), SF, A(SL))));
    pel = bsetup_pel(&setups);
    ck_assert(bsetup_with_splits_subsumes(&setups, &pel, C(P(Z(SL, SR1, LV, SL), L1, A())), k, &pl));
    ck_assert(pl == 2);
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Belief");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_morri_example);
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

