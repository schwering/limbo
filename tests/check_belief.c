// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "ex_bel.h"

START_TEST(test_ranking)
{
    univ_clauses_t static_bat = univ_clauses_init();
    belief_conds_t belief_conds = belief_conds_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    DECL_ALL_CLAUSES;
    const int k = 1;

    const stdvec_t query_z = ({
        stdvec_t z = stdvec_init();
        stdvec_append(&z, SL);
        //stdvec_append(&z, SR1);
        //stdvec_append(&z, LV);
        //stdvec_append(&z, SL);
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
    bsetup_t bsetup = bsetup_init_beliefs(&static_and_dynamic_setup, &belief_conds, &hplus, k);
    int pl;

    // split R1 in bsetup1 and bsetup2
    bsetup_t bsetup1 = bsetup_deep_copy(&bsetup);
    bsetup_t bsetup2 = bsetup_deep_copy(&bsetup);
    for (int i = 0; i < bsetup_size(&bsetup1); ++i) {
        setup_t *setup1 = bsetup_get_unsafe(&bsetup1, i);
        setup_t *setup2 = bsetup_get_unsafe(&bsetup2, i);
        setup_add(setup1, C(P(Z(), R1, A())));
        setup_add(setup2, C(N(Z(), R1, A())));
    }

    printf("plausibility levels: %d\n", bsetup_size(&bsetup));

    ck_assert(bsetup_subsumes(&bsetup, C(N(Z(), L1, A())), &pl));
    ck_assert(pl == 0);

    bsetup_add_sensing_results(&bsetup, SF(P(Z(), SF, A(SL))));
    bsetup_add_sensing_results(&bsetup1, SF(P(Z(), SF, A(SL))));
    bsetup_add_sensing_results(&bsetup2, SF(P(Z(), SF, A(SL))));
    bsetup_subsumes(&bsetup1, C(P(Z(), L1, A())), &pl);
    bsetup_subsumes(&bsetup2, C(P(Z(), L1, A())), &pl);
    print_setup(bsetup_get_unsafe(&bsetup1, 1));
    print_setup(bsetup_get_unsafe(&bsetup2, 1));
    ck_assert(bsetup_subsumes(&bsetup1, C(P(Z(), L1, A())), &pl));
    ck_assert(pl == 1);
    ck_assert(bsetup_subsumes(&bsetup2, C(P(Z(), L1, A())), &pl));
    ck_assert(pl == 1);
    ck_assert(bsetup_subsumes(&bsetup1, C(P(Z(), R1, A())), &pl));
    ck_assert(pl == 1);
    ck_assert(bsetup_subsumes(&bsetup2, C(P(Z(), R1, A())), &pl));
    ck_assert(pl == 1);

    bsetup_add_sensing_results(&bsetup, SF(N(Z(SL), SF, A(SR1))));
    ck_assert(bsetup_subsumes(&bsetup, C(P(Z(), L1, A())), &pl));
    ck_assert(pl == 1);
    ck_assert(bsetup_subsumes(&bsetup, C(P(Z(), R1, A())), &pl));
    ck_assert(pl == 1);
    ck_assert(!bsetup_subsumes(&bsetup, C(P(Z(), L1, A())), &pl));
    ck_assert(!bsetup_subsumes(&bsetup, C(N(Z(), L1, A())), &pl));

    bsetup_add_sensing_results(&bsetup, SF(P(Z(SL, SR1), SF, A(LV))));
    ck_assert(bsetup_subsumes(&bsetup, C(P(Z(), R1, A())), &pl));
    ck_assert(pl == 2);

    bsetup_add_sensing_results(&bsetup, SF(P(Z(SL, SR1, LV), SF, A(SL))));
    ck_assert(bsetup_subsumes(&bsetup, C(P(Z(), L1, A())), &pl));
    ck_assert(pl == 2);
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Belief");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_ranking);
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

