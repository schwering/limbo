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
}
END_TEST

Suite *set_suite(void)
{
  Suite *s = suite_create ("Vector");
  TCase *tc_core = tcase_create ("Core");
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


