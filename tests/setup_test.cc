// vim:filetype=cpp:textwidth=80:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2014 schwering@kbsg.rwth-aachen.de

#include <gtest/gtest.h>
#include <./kr2014.h>
#include <./clause.h>
#include <iostream>

using namespace esbl;
using namespace bats;

TEST(setup, entailment_static) {
  KR2014 bat;
  bat.InitSetup();
  bat.setup().GuaranteeConsistency(3);
  //esbl::Setup s = bat.setup().GroundBoxes({{bat.forward, bat.sonar}});
  esbl::Setup& s = bat.setup();
  EXPECT_TRUE(s.Entails({Literal({}, false, bat.d0, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, false, bat.d1, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d0, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d1, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d2, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d2, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, true, bat.d3, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d3, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, true, bat.d2, {}),
                         Literal({}, true, bat.d3, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({}, false, bat.d2, {}),
                          Literal({}, false, bat.d3, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({}, true, bat.d1, {}),
                         Literal({}, true, bat.d2, {}),
                         Literal({}, true, bat.d3, {})}, 0));
}

TEST(setup, entailment_dynamic) {
  KR2014 bat;
  bat.InitSetup();
  bat.setup().GuaranteeConsistency(3);
  //esbl::Setup s = bat.setup().GroundBoxes({{bat.forward, bat.sonar}});
  esbl::Setup& s = bat.setup();
  EXPECT_TRUE(s.Entails({Literal({bat.forward}, false, bat.d0, {})}, 0));
  EXPECT_FALSE(s.Entails({Literal({bat.forward}, true, bat.d0, {})}, 0));
  s.AddSensingResult({}, bat.forward, true);
  EXPECT_FALSE(s.Entails({Literal({bat.forward}, true, bat.d1, {}),
                          Literal({bat.forward}, true, bat.d2, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal({bat.forward}, true, bat.d1, {}),
                         Literal({bat.forward}, true, bat.d2, {})}, 1));
  s.AddSensingResult({bat.forward}, bat.sonar, true);
  const TermSeq z = {bat.forward, bat.sonar};
  EXPECT_TRUE(s.Entails({Literal(z, false, bat.d0, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal(z, false, bat.d0, {})}, 1));
  EXPECT_TRUE(s.Entails({Literal(z, true, bat.d1, {})}, 0));
  EXPECT_TRUE(s.Entails({Literal(z, true, bat.d1, {})}, 1));
}

TEST(setup, eventual_completeness) {
  KR2014 bat;
  bat.InitSetup();
}

#if 0
START_TEST(test_entailment)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    init_bat(&dynamic_bat, &static_bat, NULL);

    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(forward);
    const stdvec_t s_vec = stdvec_singleton(sonar);
    const stdvec_t fs_vec = stdvec_concat(&f_vec, &s_vec);
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_singleton(&fs_vec);
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    const setup_t setup = setup_union(&static_setup, &dynamic_setup);
    const literal_t D0 = literal_init(&empty_vec, true, d0, &empty_vec);
    const literal_t D1 = literal_init(&empty_vec, true, d1, &empty_vec);
    const literal_t D2 = literal_init(&empty_vec, true, d2, &empty_vec);
    const literal_t D3 = literal_init(&empty_vec, true, d3, &empty_vec);
    const literal_t D4 = literal_init(&empty_vec, true, d4, &empty_vec);
    const literal_t ND0 = literal_flip(&D0);
    const literal_t ND1 = literal_flip(&D1);
    const literal_t ND2 = literal_flip(&D2);
    const literal_t ND3 = literal_flip(&D3);
    const literal_t ND4 = literal_flip(&D4);
    const literal_t FD1 = literal_init(&f_vec, true, d1, &empty_vec);
    const literal_t FD2 = literal_init(&f_vec, true, d2, &empty_vec);

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
    clause_add(&d1d2, &D1);
    clause_add(&d1d2, &D2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d1d2));

    clause_t d2d3 = clause_init();
    clause_add(&d2d3, &D1);
    clause_add(&d2d3, &D2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &d2d3));

    clause_t fd1fd2 = clause_init();
    clause_add(&fd1fd2, &FD1);
    clause_add(&fd1fd2, &FD2);
    setup_up = setup_lazy_copy(&setup);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split D0
    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &d0);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &nd0);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split d1
    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &d1);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &nd1);
    ck_assert(!setup_subsumes(&setup_up, &fd1fd2));

    // split d2
    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &d2);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &nd2);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    // split d3
    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &d3);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &nd3);
    //setup_propagate_units(&setup_up);
    print_setup(&setup_up);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));

    // inconsistent split
    setup_up = setup_lazy_copy(&setup);
    clauses_add(&setup_up.clauses, &d4);
    clauses_add(&setup_up.clauses, &nd4);
    ck_assert(setup_subsumes(&setup_up, &fd1fd2));
}
END_TEST

START_TEST(test_eventual_completeness)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();

    const stdvec_t empty_vec = stdvec_init();
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_init();
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    const setup_t setup = setup_union(&static_setup, &dynamic_setup);

    print_setup(&setup);

    ({
        setup_t s = setup_lazy_copy(&setup);

        const literal_t A = literal_init(&empty_vec, true, d0, &empty_vec);
        const literal_t negA = literal_flip(&A);

        clause_t c = clause_init();
        clause_add(&c, &A);
        clause_add(&c, &negA);

        print_clause(&c);

        ck_assert(!setup_entails(&s, &c, 0));
        ck_assert(setup_entails(&s, &c, 1));
    });

    ({
        setup_t s = setup_lazy_copy(&setup);

        const stdvec_t args = stdvec_singleton(forward);
        const literal_t SFa = literal_init(&empty_vec, true, SF, &args);
        const literal_t negSFa = literal_flip(&SFa);

        clause_t c = clause_init();
        clause_add(&c, &SFa);
        clause_add(&c, &negSFa);

        print_clause(&c);

        ck_assert(!setup_entails(&s, &c, 0));
        // EDIT: the following works with the current treatment of, i.e., the
        // following comment is outdated and the "wanted" test actually holds.
        // OUTDATED:
        // This is because of our special treatment of SF literals,
        // which does not include SF literals in the PEL.
        // This could be fixed by including all SF literals from the clause in
        // clause_collect_pel_with_sf(), but then all SF literals in the query
        // would be split regardsless of k (i.e., SF(f) v ~SF(f) would be
        // entailed even for k = 0).
        // Also see documentation in setup.h for details.
        //ck_assert(!setup_entails(&s, &c, 1));
        // WANTED:
        ck_assert(setup_entails(&s, &c, 1));
    });
}
END_TEST

START_TEST(test_inconsistency)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();

    const stdvec_t empty_vec = stdvec_init();
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_init();
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    const setup_t setup = setup_union(&static_setup, &dynamic_setup);

    print_setup(&setup);

    for (int i = -3; i <= 3; ++i) {
        setup_t s = setup_lazy_copy(&setup);

        const literal_t A = literal_init(&empty_vec, true, d0, &empty_vec);
        const literal_t negA = literal_flip(&A);

        const literal_t B = literal_init(&empty_vec, true, d1, &empty_vec);
        const literal_t negB = literal_flip(&B);

        clause_t c1 = clause_init();
        clause_add(&c1, &A);
        clause_add(&c1, &negB);

        clause_t c2 = clause_init();
        clause_add(&c2, &negA);
        clause_add(&c2, &B);

        clause_t c3 = clause_init();
        clause_add(&c3, &A);
        clause_add(&c3, &B);

        clause_t c4 = clause_init();
        clause_add(&c4, &negA);
        clause_add(&c4, &negB);

        clauses_add(&s.clauses, &c1);
        clauses_add(&s.clauses, &c2);
        clauses_add(&s.clauses, &c3);
        clauses_add(&s.clauses, &c4);

        clause_t empty_clause = clause_init();
        ck_assert(setup_inconsistent(&s, 0) == false);

        if (i < 0) {
            for (int k = -i; k >= 0; --k) {
                ck_assert(setup_inconsistent(&s, k) == (k > 0));
                ck_assert(setup_entails(&s, &empty_clause, k) == (k > 0));
                printf("%sconsistent k=%d\n", (k>0 ? "in" : ""), k);
            }
        } else {
            for (int k = 0; k <= i; ++k) {
                ck_assert(setup_inconsistent(&s, k) == (k > 0));
                ck_assert(setup_entails(&s, &empty_clause, k) == (k > 0));
                printf("%sconsistent k=%d\n", (k>0 ? "in" : ""), k);
            }
        }
    };
}
END_TEST

START_TEST(test_inconsistency_sf)
{
    univ_clauses_t static_bat = univ_clauses_init();
    box_univ_clauses_t dynamic_bat = box_univ_clauses_init();
    init_bat(&dynamic_bat, &static_bat, NULL);

    const stdvec_t empty_vec = stdvec_init();
    const stdvec_t f_vec = stdvec_singleton(forward);
    const stdvec_t s_vec = stdvec_singleton(sonar);
    const stdvec_t fs_vec = stdvec_concat(&f_vec, &s_vec);
    const stdvec_t fsf_vec = stdvec_concat(&fs_vec, &f_vec);
    const stdvec_t fsfs_vec = stdvec_concat(&fsf_vec, &s_vec);
    const stdset_t ns = stdset_init();
    const stdset_t hplus = bat_hplus(&static_bat, &dynamic_bat, &ns, 0);
    const stdvecset_t query_zs = stdvecset_singleton(&fsfs_vec);
    const setup_t static_setup = setup_init_static(&static_bat, &hplus);
    const setup_t dynamic_setup = setup_init_dynamic(&dynamic_bat, &hplus, &query_zs);
    setup_t setup = setup_union(&static_setup, &dynamic_setup);

    ck_assert(!setup_inconsistent(&setup, 0));
    ck_assert(!setup_inconsistent(&setup, 1));

    setup_add_sensing_result(&setup, &empty_vec, forward, true);
    setup_add_sensing_result(&setup, &f_vec, sonar, true);

    ({
        setup_t copy = setup_copy(&setup);
        setup_add_sensing_result(&copy, &f_vec, sonar, false);
        ck_assert(setup_inconsistent(&copy, 0));
        ck_assert(setup_inconsistent(&copy, 1));
    });

    ck_assert(!setup_inconsistent(&setup, 0));
    ck_assert(!setup_inconsistent(&setup, 1));

    setup_add_sensing_result(&setup, &fs_vec, forward, true);

    ck_assert(!setup_inconsistent(&setup, 0));
    ck_assert(!setup_inconsistent(&setup, 1));

    setup_add_sensing_result(&setup, &fsf_vec, sonar, false);

    const literal_t D0 = literal_init(&fsfs_vec, true, d0, &empty_vec);
    const literal_t D1 = literal_init(&fsfs_vec, true, d1, &empty_vec);
    clause_t d0d1 = clause_init();
    clause_add(&d0d1, &D0);
    clause_add(&d0d1, &D1);
    ck_assert(setup_entails(&setup, &d0d1, 1));

    ck_assert(setup_inconsistent(&setup, 0));
    ck_assert(setup_inconsistent(&setup, 1));
    ck_assert(setup_inconsistent(&setup, 2));
    ck_assert(setup_inconsistent(&setup, 4));
}
END_TEST

Suite *clause_suite(void)
{
    Suite *s = suite_create("Setup");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_ewff);
    tcase_add_test(tc_core, test_grounding);
    tcase_add_test(tc_core, test_entailment);
    tcase_add_test(tc_core, test_eventual_completeness);
    tcase_add_test(tc_core, test_inconsistency);
    tcase_add_test(tc_core, test_inconsistency_sf);
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
#endif

