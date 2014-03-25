// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/vector.h"

static int compar_int(const void *p, const void *q)
{
    return (long int) p - (long int) q;
}

START_TEST(test_vector_insert)
{
    vector_t vec1; vector_init(&vec1);
    vector_t vec2; vector_init(&vec2);
    ck_assert(vector_eq(&vec1, &vec2, NULL));
    ck_assert(vector_eq(&vec1, &vec2, compar_int));
    ck_assert_int_eq(vector_size(&vec1), 0);
    vector_prepend(&vec1, (void *) 3);
    vector_prepend(&vec1, (void *) 2);
    vector_prepend(&vec1, (void *) 1);
    ck_assert_int_eq((long int) vector_get(&vec1, 0), 1);
    ck_assert_int_eq((long int) vector_get(&vec1, 1), 2);
    ck_assert_int_eq((long int) vector_get(&vec1, 2), 3);
    ck_assert_int_eq(vector_size(&vec1), 3);
    ck_assert(!vector_eq(&vec1, &vec2, NULL));
    ck_assert(!vector_eq(&vec1, &vec2, compar_int));
    vector_append(&vec2, (void *) 1);
    vector_append(&vec2, (void *) 2);
    vector_append(&vec2, (void *) 3);
    ck_assert_int_eq((long int) vector_get(&vec2, 0), 1);
    ck_assert_int_eq((long int) vector_get(&vec2, 1), 2);
    ck_assert_int_eq((long int) vector_get(&vec2, 2), 3);
    ck_assert_int_eq(vector_size(&vec2), 3);
    ck_assert(vector_eq(&vec1, &vec2, NULL));
    ck_assert(vector_eq(&vec1, &vec2, compar_int));
    while (vector_size(&vec1) > 0) {
        vector_remove(&vec1, 0);
    }
    for (int i = 0; i < 150; ++i) {
        vector_append(&vec1, (void *) (long int) i);
    }
    for (int i = 1500; i >= 150; --i) {
        vector_insert(&vec1, 150, (void *) (long int) i);
    }
    ck_assert_int_eq(vector_size(&vec1), 1501);
    for (int i = 0; i <= 1500; ++i) {
        ck_assert_int_eq((long int) vector_get(&vec1, i), i);
    }
    vector_free(&vec1);
    vector_free(&vec2);
}
END_TEST

START_TEST(test_vector_insert_all)
{
    vector_t vec1; vector_init(&vec1);
    vector_t vec2; vector_init(&vec2);
    for (int i = 0; i < 10; ++i) {
        vector_append(&vec1, (const void *) (long int) i);
    }
    for (int i = 10; i < 20; ++i) {
        vector_append(&vec2, (const void *) (long int) i);
    }
    for (int i = 20; i < 30; ++i) {
        vector_append(&vec1, (const void *) (long int) i);
    }
    ck_assert_int_eq(vector_size(&vec1), 20);
    ck_assert_int_eq(vector_size(&vec2), 10);
    vector_insert_all(&vec1, 10, &vec2);
    ck_assert_int_eq(vector_size(&vec1), 30);
    for (int i = 1; i < vector_size(&vec1); ++i) {
        ck_assert_int_eq((long int) vector_get(&vec1, i-1) + 1, (long int) vector_get(&vec1, i));
    }
    vector_free(&vec2);
    vector_copy(&vec2, &vec1);
    ck_assert(vector_eq(&vec1, &vec2, compar_int));
    vector_free(&vec2);
    vector_copy_range(&vec2, &vec1, 0, 10);
    ck_assert_int_eq(vector_size(&vec2), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq((long int) vector_get(&vec2, i), i);
    }
    vector_clear(&vec1);
    ck_assert_int_eq(vector_size(&vec1), 0);
    vector_append_all(&vec1, &vec2);
    ck_assert_int_eq(vector_size(&vec1), 10);
    vector_append_all(&vec1, &vec2);
    ck_assert_int_eq(vector_size(&vec1), 20);
    vector_append_all(&vec1, &vec2);
    ck_assert_int_eq(vector_size(&vec1), 30);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq((long int) vector_get(&vec1, i), i);
        ck_assert_int_eq((long int) vector_get(&vec1, i + 10), i);
        ck_assert_int_eq((long int) vector_get(&vec1, i + 20), i);
    }
    vector_free(&vec1);
    vector_free(&vec2);
}
END_TEST

Suite *vector_suite(void)
{
  Suite *s = suite_create("Vector");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_vector_insert);
  tcase_add_test(tc_core, test_vector_insert_all);
  suite_add_tcase(s, tc_core);
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

