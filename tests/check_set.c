// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/set.h"

static int compar_int(const void *p, const void *q)
{
    return (long int) p - (long int) q;
}

START_TEST(test_set_add)
{
    set_t set; set_init(&set, compar_int);
    for (int i = 0; i < 10; ++i) {
        set_add(&set, (const void *) (long int) i);
        set_add(&set, (const void *) (long int) (i + 10));
        set_add(&set, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set), 20);
    for (int i = 0; i < 20; ++i) {
        ck_assert_int_eq(set_find(&set, (const void *) (long int) i), i);
        ck_assert(set_contains(&set, (const void *) (long int) i));
    }
    for (int i = 20; i < 30; ++i) {
        ck_assert_int_eq(set_find(&set, (const void *) (long int) -i), -1);
        ck_assert(!set_contains(&set, (const void *) (long int) -i));
    }
    for (int i = 5; i < 15; ++i) {
        ck_assert(set_contains(&set, (const void *) (long int) i));
        set_remove(&set, (const void *) (long int) i);
        ck_assert(!set_contains(&set, (const void *) (long int) i));
    }
    ck_assert_int_eq(set_size(&set), 10);
    for (int i = 5; i < 15; ++i) {
        ck_assert(!set_contains(&set, (const void *) (long int) i));
        set_add(&set, (const void *) (long int) i);
        ck_assert(set_contains(&set, (const void *) (long int) i));
    }
    for (int i = 15; i > 5; --i) {
        ck_assert(set_contains(&set, (const void *) (long int) i));
        set_remove(&set, (const void *) (long int) i);
        ck_assert(!set_contains(&set, (const void *) (long int) i));
    }
    ck_assert_int_eq(set_size(&set), 10);
    for (int i = 15; i > 5; --i) {
        ck_assert(!set_contains(&set, (const void *) (long int) i));
        set_add(&set, (const void *) (long int) i);
        ck_assert(set_contains(&set, (const void *) (long int) i));
    }
    ck_assert_int_eq(set_size(&set), 20);
    set_free(&set);
}
END_TEST

START_TEST(test_set_singleton)
{
    set_t set; set_singleton(&set, compar_int, (const void *) (long int) 5);
    ck_assert_int_eq(set_size(&set), 1);
    ck_assert(set_contains(&set, (const void *) (long int) 5));
    ck_assert(!set_contains(&set, (const void *) (long int) 4));
    ck_assert(!set_contains(&set, (const void *) (long int) 6));
    set_free(&set);
}
END_TEST

START_TEST(test_set_union)
{
    set_t set1; set_init(&set1, compar_int);
    set_t set2; set_init(&set2, compar_int);
    set_t set;
    // left same as right
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_union(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 10);
    ck_assert(set_eq(&set, &set1));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 20);
    set_union(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 20);
    ck_assert(set_eq(&set, &set2));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_union(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 20);
    ck_assert(set_eq(&set, &set1));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 20);
    set_union(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 30);
    for (int i = 0; i < 30; ++i) {
        ck_assert_int_eq((long int) set_get(&set, i), i);
    }
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_union(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 20);
    for (int i = 0; i < 20; ++i) {
        ck_assert_int_eq((long int) set_get(&set, i), i);
    }
    set_free(&set);
    set_free(&set1);
    set_free(&set2);
}
END_TEST

START_TEST(test_set_intersection)
{
    set_t set1; set_init(&set1, compar_int);
    set_t set2; set_init(&set2, compar_int);
    set_t set;
    // left same as right
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 0; i < 10; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_intersection(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 10);
    ck_assert(set_eq(&set, &set1));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left subset of right
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 0; i < 20; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 20);
    set_intersection(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 10);
    ck_assert(set_eq(&set, &set1));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left superset of right
    for (int i = 0; i < 20; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 20);
    for (int i = 0; i < 10; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_intersection(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 10);
    ck_assert(set_eq(&set, &set2));
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left and right intersect
    for (int i = 0; i < 20; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 20);
    for (int i = 10; i < 30; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 20);
    set_intersection(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 10);
    for (int i = 10; i < 20; ++i) {
        ck_assert_int_eq((long int) set_get(&set, i), i);
    }
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    // left and right don't intersect
    for (int i = 0; i < 10; ++i) {
        set_add(&set1, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set1), 10);
    for (int i = 10; i < 20; ++i) {
        set_add(&set2, (const void *) (long int) i);
    }
    ck_assert_int_eq(set_size(&set2), 10);
    set_intersection(&set, &set1, &set2);
    ck_assert_int_eq(set_size(&set), 0);
    set_free(&set);
    set_clear(&set1);
    set_clear(&set2);
    set_free(&set);
    set_free(&set1);
    set_free(&set2);
}
END_TEST

Suite *set_suite(void)
{
  Suite *s = suite_create("Set");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_set_add);
  tcase_add_test(tc_core, test_set_singleton);
  tcase_add_test(tc_core, test_set_union);
  tcase_add_test(tc_core, test_set_intersection);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = set_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

