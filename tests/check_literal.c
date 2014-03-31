// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/literal.h"

START_TEST(test_literal)
{
    const stdname_t kak1[2] = {1,2};
    const stdname_t kak2[3] = {1,2,3};

    const pred_t P = 1;
    const pred_t Q = 2;
    const stdvec_t z1 = stdvec_from_array(kak1, 2);
    const stdvec_t z2 = stdvec_from_array(kak2, 3);
    const stdvec_t args1 = stdvec_from_array(kak1, 2);
    const stdvec_t args2 = stdvec_from_array(kak2, 3);
    const literal_t p = literal_init(&z1, true, P, &args1);
    literal_t q;

    q = literal_flip(&p);
    ck_assert(!literal_eq(&p, &q));

    q = literal_flip(&q);
    ck_assert(literal_eq(&p, &q));

    q = literal_init(&z1, false, P, &args1);
    ck_assert(!literal_eq(&p, &q));

    q = literal_flip(&q);
    ck_assert(literal_eq(&p, &q));

    q = literal_init(&z2, true, P, &args1);
    ck_assert(!stdvec_eq(literal_z(&p), literal_z(&q)));
    ck_assert(!literal_eq(&p, &q));

    q = literal_init(&z1, true, P, &args2);
    ck_assert(!literal_eq(&p, &q));

    q = literal_init(&z1, true, Q, &args1);
    ck_assert(!literal_eq(&p, &q));
}
END_TEST

Suite *literal_suite(void)
{
  Suite *s = suite_create("Vector");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_literal);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = literal_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

