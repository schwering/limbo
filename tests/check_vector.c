// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include <check.h>
#include <stdlib.h>
#include "../src/vector.h"

static int compar_long_int(long int p, long int q)
{
    return p - q;
}

VECTOR_DECL(ivec, long int);
VECTOR_IMPL(ivec, long int, compar_long_int);

START_TEST(test_vector_insert)
{
    ivec_t vec1 = ivec_init();
    ivec_t vec2 = ivec_init();
    ck_assert(ivec_eq(&vec1, &vec2));
    ck_assert_int_eq(ivec_size(&vec1), 0);
    ivec_prepend(&vec1, 3);
    ivec_prepend(&vec1, 2);
    ivec_prepend(&vec1, 1);
    ck_assert_int_eq(ivec_get(&vec1, 0), 1);
    ck_assert_int_eq(ivec_get(&vec1, 1), 2);
    ck_assert_int_eq(ivec_get(&vec1, 2), 3);
    ck_assert_int_eq(ivec_size(&vec1), 3);
    ck_assert(!ivec_eq(&vec1, &vec2));
    ivec_append(&vec2, 1);
    ivec_append(&vec2, 2);
    ivec_append(&vec2, 3);
    ck_assert_int_eq(ivec_get(&vec2, 0), 1);
    ck_assert_int_eq(ivec_get(&vec2, 1), 2);
    ck_assert_int_eq(ivec_get(&vec2, 2), 3);
    ck_assert_int_eq(ivec_size(&vec2), 3);
    ck_assert(ivec_eq(&vec1, &vec2));
    while (ivec_size(&vec1) > 0) {
        ivec_remove(&vec1, 0);
    }
    for (int i = 0; i < 150; ++i) {
        ivec_append(&vec1, i);
    }
    for (int i = 1500; i >= 150; --i) {
        ivec_insert(&vec1, 150, i);
    }
    ck_assert_int_eq(ivec_size(&vec1), 1501);
    for (int i = 0; i <= 1500; ++i) {
        ck_assert_int_eq(ivec_get(&vec1, i), i);
    }
    ivec_cleanup(&vec1);
    ivec_cleanup(&vec2);
}
END_TEST

START_TEST(test_vector_insert_all)
{
    ivec_t vec1 = ivec_init();
    ivec_t vec2 = ivec_init();
    for (int i = 0; i < 10; ++i) {
        ivec_append(&vec1, i);
    }
    for (int i = 10; i < 20; ++i) {
        ivec_append(&vec2, i);
    }
    for (int i = 20; i < 30; ++i) {
        ivec_append(&vec1, i);
    }
    ck_assert_int_eq(ivec_size(&vec1), 20);
    ck_assert_int_eq(ivec_size(&vec2), 10);
    ivec_insert_all(&vec1, 10, &vec2);
    ck_assert_int_eq(ivec_size(&vec1), 30);
    for (int i = 1; i < ivec_size(&vec1); ++i) {
        ck_assert_int_eq(ivec_get(&vec1, i-1) + 1, ivec_get(&vec1, i));
    }
    ivec_cleanup(&vec2);
    vec2 = ivec_copy(&vec1);
    ck_assert(ivec_eq(&vec1, &vec2));
    ivec_cleanup(&vec2);
    vec2 = ivec_copy_range(&vec1, 0, 10);
    ck_assert_int_eq(ivec_size(&vec2), 10);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(ivec_get(&vec2, i), i);
    }
    ivec_clear(&vec1);
    ck_assert_int_eq(ivec_size(&vec1), 0);
    ivec_append_all(&vec1, &vec2);
    ck_assert_int_eq(ivec_size(&vec1), 10);
    ivec_append_all(&vec1, &vec2);
    ck_assert_int_eq(ivec_size(&vec1), 20);
    ivec_append_all(&vec1, &vec2);
    ck_assert_int_eq(ivec_size(&vec1), 30);
    for (int i = 0; i < 10; ++i) {
        ck_assert_int_eq(ivec_get(&vec1, i), i);
        ck_assert_int_eq(ivec_get(&vec1, i + 10), i);
        ck_assert_int_eq(ivec_get(&vec1, i + 20), i);
    }
    ivec_cleanup(&vec1);
    ivec_cleanup(&vec2);
}
END_TEST

START_TEST(test_vector_remove_range)
{
    ivec_t vec = ivec_init();
    for (int from = 0; from < 10; ++from) {
        for (int to = from; to <= 10; ++to) {
            for (int i = 0; i < 10; ++i) {
                ivec_append(&vec, i);
            }
            ck_assert_int_eq(ivec_size(&vec), 10);
            ivec_remove_range(&vec, from, to);
            ck_assert_int_eq(ivec_size(&vec), 10 - (to - from));
            for (int i = 0; i < ivec_size(&vec); ++i) {
                if (i < from) {
                    ck_assert_int_eq(ivec_get(&vec, i), i);
                } else {
                    ck_assert_int_eq(ivec_get(&vec, i), i+(to-from));
                }
            }
            ivec_remove_range(&vec, 0, ivec_size(&vec));
            ck_assert_int_eq(ivec_size(&vec), 0);
            ivec_cleanup(&vec);
        }
    }
}
END_TEST

START_TEST(test_vector_remove_all)
{
    ivec_t vec = ivec_init();
    for (int i = 0; i < 10; ++i) {
        ivec_append(&vec, i);
    }
    ck_assert_int_eq(ivec_size(&vec), 10);
    ivec_remove_all(&vec, (int[]) {1,3,5,7,9}, 5);
    ck_assert_int_eq(ivec_size(&vec), 5);
    for (int i = 0; i < ivec_size(&vec); ++i) {
        ck_assert_int_eq(ivec_get(&vec, i), 2*i);
    }
    ivec_remove_all(&vec, (int[]) {1,2,3}, 3);
    ck_assert_int_eq(ivec_size(&vec), 2);
    ck_assert_int_eq(ivec_get(&vec, 0), 0);
    ck_assert_int_eq(ivec_get(&vec, 1), 8);
    ivec_remove_all(&vec, (int[]) {0,1}, 2);
    ck_assert_int_eq(ivec_size(&vec), 0);
    for (int i = 0; i < 10; ++i) {
        ivec_append(&vec, i);
    }
    ck_assert_int_eq(ivec_size(&vec), 10);
    ivec_remove_all(&vec, (int[]) {0,2,4,6,8}, 5);
    ck_assert_int_eq(ivec_size(&vec), 5);
    for (int i = 0; i < ivec_size(&vec); ++i) {
        ck_assert_int_eq(ivec_get(&vec, i), 2*i+1);
    }
    ivec_remove_all(&vec, (int[]) {}, 0);
    ck_assert_int_eq(ivec_size(&vec), 5);
    ivec_remove_all(&vec, (int[]) {0,1}, 2);
    ck_assert_int_eq(ivec_size(&vec), 3);
    ck_assert_int_eq(ivec_get(&vec, 0), 5);
    ck_assert_int_eq(ivec_get(&vec, 1), 7);
    ck_assert_int_eq(ivec_get(&vec, 2), 9);
    ivec_remove_all(&vec, (int[]) {1,2}, 2);
    ck_assert_int_eq(ivec_size(&vec), 1);
    ck_assert_int_eq(ivec_get(&vec, 0), 5);
    ivec_remove_all(&vec, (int[]) {0}, 1);
    ck_assert_int_eq(ivec_size(&vec), 0);
    ivec_cleanup(&vec);
}
END_TEST

START_TEST(test_vector_cmp)
{
    ivec_t vec1 = ivec_init();
    ivec_t vec2 = ivec_init();
    for (int i = 0; i < 10; ++i) {
        ivec_append(&vec1, i);
        ivec_append(&vec2, i);
    }
    ck_assert_int_eq(ivec_size(&vec1), 10);
    ck_assert_int_eq(ivec_size(&vec2), 10);
    ck_assert(ivec_eq(&vec1, &vec2));
    ck_assert(ivec_is_prefix(&vec1, &vec2));
    ivec_remove(&vec1, ivec_size(&vec1) - 1);
    ck_assert(!ivec_eq(&vec1, &vec2));
    ck_assert(ivec_is_prefix(&vec1, &vec2));
    ck_assert(ivec_cmp(&vec1, &vec2) < 0);
    ivec_append(&vec1, 0);
    ck_assert(!ivec_eq(&vec1, &vec2));
    ck_assert(!ivec_is_prefix(&vec1, &vec2));
    ck_assert(ivec_cmp(&vec1, &vec2) < 0);
    ivec_cleanup(&vec1);
    ivec_cleanup(&vec2);
}
END_TEST

Suite *ivec_suite(void)
{
  Suite *s = suite_create("Vector");
  TCase *tc_core = tcase_create("Core");
  tcase_add_test(tc_core, test_vector_insert);
  tcase_add_test(tc_core, test_vector_insert_all);
  tcase_add_test(tc_core, test_vector_remove_range);
  tcase_add_test(tc_core, test_vector_remove_all);
  tcase_add_test(tc_core, test_vector_cmp);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void)
{
    int number_failed;
    Suite *s = ivec_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

