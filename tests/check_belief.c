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
    const int k = 0;

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
    const ranked_setups_t ranked_setups = setup_init_beliefs(&static_setup, &belief_conds, &hplus, k);
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

