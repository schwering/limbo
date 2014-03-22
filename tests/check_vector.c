// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/vector.h"

START_TEST (test_vector_create)
{
    vector_t vec1; vector_init(&vec1);
    vector_t vec2; vector_init(&vec2);
    ck_assert_int_eq(vector_eq(&vec1, &vec2, NULL), true);
    vector_free(vec1);
    vector_free(vec2);
}
END_TEST

Suite *vector_suite(void)
{
  Suite *s = suite_create ("Vector");

  // Core test case
  TCase *tc_core = tcase_create ("Core");
  tcase_add_test (tc_core, test_vector_create);
  suite_add_tcase (s, tc_core);

  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = vector_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

