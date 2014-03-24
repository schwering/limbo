// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/set.h"

static int compar_int(const void *p, const void *q)
{
    return (long int) p - (long int) q;
}

START_TEST(test_set_insert)
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

Suite *set_suite(void)
{
  Suite *s = suite_create("Set");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_set_insert);
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


