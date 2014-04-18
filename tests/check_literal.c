// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/literal.h"
#include "../src/util.h"
#include "../src/memory.h"

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

START_TEST(test_substitution)
{
    const var_t x = -123;
    const stdname_t n = 567;
    const stdname_t m = 568;
    const pred_t p = 890;
    literal_t *l1;
    literal_t *l2;
    literal_t l3;

    varmap_t varmap = varmap_init();
    varmap_add(&varmap, x, n);

    l1 = P(Z(x,x), p, Z(x,x));
    l2 = P(Z(n,n), p, Z(n,n));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x,x), p, Z(x,m));
    l2 = P(Z(n,n), p, Z(n,m));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x,x), p, Z(m,m));
    l2 = P(Z(n,n), p, Z(m,m));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x), p, Z(m,m));
    l2 = P(Z(n), p, Z(m,m));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(), p, Z(m,m));
    l2 = P(Z(), p, Z(m,m));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(), p, Z(x,x));
    l2 = P(Z(), p, Z(n,n));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x,x), p, Z(m));
    l2 = P(Z(n,n), p, Z(m));
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x,x), p, Z());
    l2 = P(Z(n,n), p, Z());
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(x), p, Z());
    l2 = P(Z(n), p, Z());
    l3 = literal_substitute(l1, &varmap);
    ck_assert(!literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));

    l1 = P(Z(), p, Z());
    l2 = P(Z(), p, Z());
    l3 = literal_substitute(l1, &varmap);
    ck_assert(literal_eq(l1, l2));
    ck_assert(literal_eq(l2, &l3));
}
END_TEST

Suite *literal_suite(void)
{
  Suite *s = suite_create("Vector");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_literal);
  tcase_add_test(tc_core, test_substitution);
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

